#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

namespace util {

// Pretty similar to https://stackoverflow.com/a/12805690
template <typename T>
class BlockingQueue {
public:
    void push_back(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        condition_.notify_one();
    }

    T pop_front() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [=] { return not queue_.empty(); });
        T rc(std::move(queue_.front()));
        queue_.pop();
        return rc;
    }

    bool wait_for(unsigned max_wait_time_millis) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, std::chrono::milliseconds(max_wait_time_millis), [=] { return not queue_.empty(); });
        return not queue_.empty();
    }

    typename std::deque<T>::size_type non_blocking_size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool non_blocking_empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    T front_copy() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [=] { return not queue_.empty(); });
        return queue_.front();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
};

} // namespace util
