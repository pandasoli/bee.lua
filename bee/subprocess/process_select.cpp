#include <bee/subprocess.h>
#include <bee/subprocess/process_select.h>

#if defined(_WIN32)
#    include <Windows.h>
#else
#    include <poll.h>
#endif

namespace bee::subprocess {
    status process_select(const dynarray<process*>& set, int timeout) noexcept {
#if defined(_WIN32)
        if (set.size() >= MAXIMUM_WAIT_OBJECTS) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return status::failed;
        }
        dynarray<HANDLE> fds(set.size());
        DWORD nfds = 0;
        for (const auto& p : set) {
            auto fd     = p->native_handle();
            fds[nfds++] = fd;
        }
        DWORD ret = WaitForMultipleObjects(nfds, fds.data(), FALSE, (timeout < 0) ? INFINITE : static_cast<DWORD>(timeout));
        if (ret == WAIT_TIMEOUT) {
            return status::timeout;
        }
        if (ret < WAIT_OBJECT_0 || ret > (WAIT_OBJECT_0 + nfds - 1)) {
            return status::failed;
        }
        return status::success;
#else
        dynarray<pollfd> fds(set.size());
        nfds_t nfds = 0;
        for (const auto& p : set) {
            auto fd     = p->native_handle();
            fds[nfds++] = { fd, POLLIN | POLLPRI, 0 };
        }
        int ret = poll(fds.data(), nfds, timeout);
        if (ret == -1) {
            return status::failed;
        }
        if (ret == 0) {
            return status::timeout;
        }
        return status::success;
#endif
    }
}
