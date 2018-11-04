#pragma once

#include "util/message_util.h"
#include "util/blocking_deque.h"

#include <proj/proj_config.h>

#ifdef PROJ_ARM_SERVER_ONLY
#include "util/util.h"
#endif

#include <google/protobuf/dynamic_message.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <proj/annotations.pb.h>
#include <fstream>
#include <tuple>

namespace svr {

using ComputeFunc = void (*)(google::protobuf::Message*);

class ServerTree {
public:
    /**
     * @tparam T is the message type
     * @return true if output successfully added, false if output already exists
     */
    template <typename T,
              typename = typename std::enable_if<std::is_base_of<google::protobuf::Message, T>::value>::type>
    bool add_output(std::shared_ptr<util::BlockingQueue<T>> queue);

    template <typename T, typename Func, typename... Args>
    void register_function(Func func, Args... args);

    bool update_source(const google::protobuf::Message& message);

    std::string graphvis_string() const;

private:
    struct Computer {
        virtual ~Computer() = 0;
        virtual void compute(google::protobuf::Message*) = 0;
    };

    template <typename T, typename Func, typename... Args>
    struct TypedComputer : public Computer {
        explicit TypedComputer(Func func, Args... args)
            : func_(func), arguments_(std::make_tuple(std::forward<Args>(args)..., static_cast<T*>(nullptr))) {}
        ~TypedComputer() override = default;

        void compute(google::protobuf::Message* message) override {
            std::get<sizeof...(Args)>(arguments_) = dynamic_cast<T*>(message);
#ifdef PROJ_ARM_SERVER_ONLY
            util::apply(func_, arguments_);
#else
            std::apply(func_, arguments_);
#endif
        }

    private:
        Func func_;
        std::tuple<typename std::decay<Args>::type..., T*> arguments_;
    };

    using NodeKey = const google::protobuf::Descriptor*;

    struct ServerNode {
        std::unique_ptr<google::protobuf::Message> message; // stored data
        std::unordered_map<int, NodeKey> inputs;
        std::unordered_map<NodeKey, int> outputs;

        bool valid = false;
        std::string debug_name = {};

        explicit ServerNode(std::unique_ptr<google::protobuf::Message> msg);
    };

    struct Sink {
        virtual ~Sink() = 0;
        virtual google::protobuf::Message* get_data() = 0;
        virtual void send_data() const = 0;
    };

    template <typename D>
    struct SinkData : Sink {
        D data;
        std::shared_ptr<util::BlockingQueue<D>> queue;

        explicit SinkData(std::shared_ptr<util::BlockingQueue<D>> q) : queue(std::move(q)) {}
        ~SinkData() override = default;

        google::protobuf::Message* get_data() override { return &data; }

        void send_data() const override {
            std::cout << "Sending data" << std::endl;
            queue->push_back(data);
        }
    };

    std::unordered_map<NodeKey, std::unique_ptr<ServerNode>> nodes_;
    std::unordered_set<NodeKey> sources_;
    std::unordered_map<NodeKey, std::unique_ptr<Sink>> sinks_;
    std::unordered_map<NodeKey, std::unique_ptr<Computer>> compute_functions_;

    NodeKey get_key(const google::protobuf::Descriptor* desc);
    NodeKey get_key(const google::protobuf::Message& message);
    NodeKey get_key(google::protobuf::Message* message);

    NodeKey build_node(google::protobuf::Message* message);

    std::unordered_set<NodeKey> invalidate_node(NodeKey key, int input_index);
    std::unordered_set<NodeKey> update_node(NodeKey key, int input_index);

    std::unordered_set<google::protobuf::Message*> copy_node_to_messages(
        const ServerNode& node, int field_index, const std::unordered_set<google::protobuf::Message*>& parent_msgs);
};

template <typename T, typename>
bool ServerTree::add_output(std::shared_ptr<util::BlockingQueue<T>> queue) {
    auto data = util::make_unique<SinkData<T>>(std::move(queue));

    auto key = get_key(data->get_data());
    if (sinks_.find(key) != sinks_.end()) {
        return false;
    }

    auto sink_key = build_node(data->get_data());
    assert(key == sink_key);
    sinks_.emplace(sink_key, std::move(data));

    {
        std::ofstream graphvis_file("nodes.dot.ps");
        graphvis_file << graphvis_string();
    }

    return true;
}

template <typename T, typename Func, typename... Args>
void ServerTree::register_function(Func func, Args... args) {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value, "Can ony register functions for message types");
    T tmp;
    util::MsgPkg msg_pkg(&tmp);
    assert(msg_pkg.desc->options().GetExtension(proj::proto::node) != proj::proto::Node::NONE);

#ifdef PROJ_ARM_SERVER_ONLY
    auto key = get_key(tmp);
    auto iter = compute_functions_.find(key);
    if (iter != compute_functions_.end()) {
        compute_functions_.erase(key);
    }
    compute_functions_.emplace(key,
                               util::make_unique<TypedComputer<T, Func, Args...>>(func, std::forward<Args>(args)...));
#else
    compute_functions_.insert_or_assign(get_key(tmp),
                                        std::make_unique<TypedComputer<T, Func, Args...>>(func,
                                                                                          std::forward<Args>(args)...));
#endif
}

} // namespace svr
