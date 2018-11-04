#include "semaphore.h"

namespace util {

Semaphore::Semaphore(unsigned count) : count_(count) {}

void Semaphore::notify() {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    ++count_;
    condition_.notify_one();
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> unlockable_lock(lock_);
    condition_.wait(unlockable_lock, [this] { return count_ > 0; });
    --count_;
}

bool Semaphore::try_wait() {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    if (count_) {
        --count_;
        return true;
    }
    return false;
}

} // namespace util
