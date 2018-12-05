#pragma once

#include <mutex>
#include <condition_variable>

namespace bee {
    class semaphore {
    public:
        void signal() {
            std::unique_lock<std::mutex> lk(mutex);
            if (ok) {
                return;
            }
            ok = true;
            lk.unlock();
            condition.notify_one();
        }
        void wait() {
            std::unique_lock<std::mutex> lk(mutex);
            condition.wait(lk, [this] { return ok; });
            ok = false;
        }
		template <class Clock, class Duration>
        bool timed_wait(const std::chrono::time_point<Clock, Duration>& timeout) {
            std::unique_lock<std::mutex> lk(mutex);
            if (condition.wait_until(lk, timeout, [this] { return ok; })) {
                ok = false;
                return true;
            }
            return false;
        }
    private:
        std::mutex mutex;
        std::condition_variable condition;
        bool ok = false;
    };
}
