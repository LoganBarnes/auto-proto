#pragma once

#include <mutex>
#include <condition_variable>

namespace util {

class Semaphore {
public:
    explicit Semaphore(unsigned count = 0);
    void notify();
    void wait();
    bool try_wait();

private:
    std::mutex lock_;
    std::condition_variable condition_;
    unsigned count_;
};

} // namespace util
