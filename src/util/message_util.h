#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/byte_buffer.h>

namespace util {

struct MsgPkg {
    google::protobuf::Message* msg;
    const google::protobuf::Descriptor* desc;
    const google::protobuf::Reflection* refl;
    const google::protobuf::FieldDescriptor* field = nullptr;

    explicit MsgPkg(google::protobuf::Message* message)
        : msg(message), desc(msg->GetDescriptor()), refl(msg->GetReflection()) {}

    void set_field_index(int field_index) { field = desc->field(field_index); }
};

template <typename Func>
void iterate_msg_fields(google::protobuf::Message* msg, const Func& func) {
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();

    int field_count = desc->field_count();

    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor* field = desc->field(i);
        func(field, i);
    }
}

template <typename Func>
void iterate_msg_fields(const google::protobuf::Message& msg, const Func& func) {
    const google::protobuf::Descriptor* desc = msg.GetDescriptor();

    int field_count = desc->field_count();

    for (int i = 0; i < field_count; ++i) {
        const google::protobuf::FieldDescriptor* field = desc->field(i);
        func(field, i);
    }
}

template <typename Msg, typename = std::enable_if_t<std::is_base_of<google::protobuf::Message, Msg>::value>>
static std::unique_ptr<Msg> clone_msg(const Msg& msg) {
    std::unique_ptr<Msg> p(msg.New());
    p->CopyFrom(msg);
    return p;
}

void print_field(std::ostream& os,
                 const google::protobuf::Message& msg,
                 const google::protobuf::FieldDescriptor* field,
                 const google::protobuf::Reflection* refl);

void copy_field(google::protobuf::Message* dst, const google::protobuf::Message& src, int field_index);

bool message_has_field(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field);

std::unique_ptr<grpc::ByteBuffer> serialize_to_byte_buffer(const google::protobuf::Message& message);
void deserialize_from_byte_buffer(grpc::ByteBuffer* buffer, google::protobuf::Message* message);

std::string graphvis_string(const google::protobuf::Message& message);

} // namespace util
