#include <bee/thread/atomic_sync.h>

#if defined(_WIN32)

#    include <Windows.h>

static_assert(sizeof(bee::atomic_sync::value_type) == 1);

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val) noexcept {
    ::WaitOnAddress((PVOID)ptr, (PVOID)&val, sizeof(value_type), INFINITE);
}

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val, int timeout) noexcept {
    ::WaitOnAddress((PVOID)ptr, (PVOID)&val, sizeof(value_type), timeout);
}

void bee::atomic_sync::wake(const value_type* ptr, bool all) noexcept {
    if (all) {
        ::WakeByAddressAll((PVOID)ptr);
    }
    else {
        ::WakeByAddressSingle((PVOID)ptr);
    }
}

#elif defined(__linux__)

#    include <linux/futex.h>
#    include <sys/syscall.h>
#    include <unistd.h>

#    include <climits>

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val) noexcept {
    ::syscall(SYS_futex, ptr, FUTEX_WAIT_PRIVATE, val, nullptr, 0, 0);
}

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val, int timeout) noexcept {
    struct FutexTimespec {
        long tv_sec;
        long tv_nsec;
    };
    FutexTimespec ts;
    ts.tv_sec  = (long)(timeout / 1000);
    ts.tv_nsec = (long)(timeout % 1000 * 1000 * 1000);
    ::syscall(SYS_futex, ptr, FUTEX_WAIT_PRIVATE, val, &ts, 0, 0);
}

void bee::atomic_sync::wake(const value_type* ptr, bool all) noexcept {
    ::syscall(SYS_futex, ptr, FUTEX_WAKE_PRIVATE, all ? INT_MAX : 1, 0, 0, 0);
}

#elif defined(BEE_USE_ULOCK)

#    include <errno.h>

extern "C" int __ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout);
extern "C" int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

constexpr uint32_t UL_COMPARE_AND_WAIT = 1;
constexpr uint32_t ULF_WAKE_ALL        = 0x00000100;
constexpr uint32_t ULF_NO_ERRNO        = 0x01000000;

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val) noexcept {
    for (;;) {
        int rc = ::__ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, const_cast<value_type*>(ptr), val, 0);
        if (rc == -EINTR) {
            continue;
        }
        return;
    }
}

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val, int timeout) noexcept {
    ::__ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, const_cast<value_type*>(ptr), val, timeout * 1000);
}

void bee::atomic_sync::wake(const value_type* ptr, bool all) noexcept {
    ::__ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | (all ? ULF_WAKE_ALL : 0), const_cast<value_type*>(ptr), 0);
}
#else

#    include <bee/thread/simplethread.h>
#    include <bee/thread/spinlock.h>

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val) noexcept {
    ++ctx;
    if (ctx < 64) {
        cpu_relax();
    }
    else if (ctx < 72) {
        thread_yield();
    }
    else {
        thread_sleep(10);
    }
}

void bee::atomic_sync::wait(int& ctx, const value_type* ptr, value_type val, int timeout) noexcept {
    atomic_sync::wait(ctx, ptr, val);
}

void bee::atomic_sync::wake(const value_type* ptr, bool all) noexcept {
}

#endif
