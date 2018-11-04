#pragma once

#include <util/atomic_data.h>
#include <util/blocking_deque.h>
#include <util/semaphore.h>

#include <grpcpp/server_context.h>

#include <atomic>

namespace svr {

template <typename T>
class StreamHandler {
public:
    void handle_client(grpc::ServerContext* context, grpc::ServerWriter<T>* writer);

    void send_data(T data);

    void attempt_shutdown();

private:
    util::AtomicData<unsigned> num_streaming_clients_;
    util::BlockingQueue<T> queue_;

    util::Semaphore pop_block_;
    util::Semaphore loop_block_;

    std::atomic_bool attempt_shutdown_ = false;
};

template <typename T>
void StreamHandler<T>::handle_client(grpc::ServerContext* context, grpc::ServerWriter<T>* writer) {
    num_streaming_clients_.use_safely([](unsigned& clients) { ++clients; });

    bool broken_stream = false;

    T data_to_send;

    while (not broken_stream and not attempt_shutdown_.load() and not context->IsCancelled()) {
        // wait for a max of 5 seconds before checking that the stream is still valid
        if (queue_.wait_for(5000u)) {
            data_to_send = queue_.front_copy();

            pop_block_.notify();
            broken_stream = not writer->Write(data_to_send);
            loop_block_.wait();
        }
    }

    num_streaming_clients_.use_safely([](unsigned& clients) { --clients; });
}

template <typename T>
void StreamHandler<T>::send_data(T data) {
    num_streaming_clients_.use_safely([&](unsigned clients) {
        queue_.push_back(std::move(data));

        for (auto i = 0u; i < clients; ++i) {
            pop_block_.wait();
        }

        queue_.pop_front();

        for (auto i = 0u; i < clients; ++i) {
            loop_block_.notify();
        }
    });
}

template <typename T>
void StreamHandler<T>::attempt_shutdown() {
    num_streaming_clients_.use_safely([this](unsigned clients) {
        queue_.push_back(T());

        for (auto i = 0u; i < clients; ++i) {
            pop_block_.wait();
        }

        queue_.pop_front();
        attempt_shutdown_.store(true);

        for (auto i = 0u; i < clients; ++i) {
            loop_block_.notify();
        }
    });
}

} // namespace svr
