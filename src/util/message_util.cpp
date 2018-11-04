#include "message_util.h"

#include <proj/annotations.pb.h>

#include <grpcpp/impl/codegen/proto_utils.h>

#include <sstream>

namespace gp = google::protobuf;

namespace util {

namespace {

struct ConstMsgPkg {
    const google::protobuf::Message& msg;
    const google::protobuf::Descriptor* desc;
    const google::protobuf::Reflection* refl;
    const google::protobuf::FieldDescriptor* field = nullptr;

    explicit ConstMsgPkg(const google::protobuf::Message& message)
        : msg(message), desc(msg.GetDescriptor()), refl(msg.GetReflection()) {}

    void set_field_index(int field_index) { field = desc->field(field_index); }
};

bool graphvis_string(::std::ostream& os, gp::Message* message, bool is_sink, bool message_existed) {

    const std::string& node_name = message->GetDescriptor()->name();

    MsgPkg msg_pkg(message);

    bool is_source = true;

    bool valid = true;

    iterate_msg_fields(message, [&](const gp::FieldDescriptor* field, int /*field_index*/) {
        if (not field->is_repeated() and field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {

            bool has_field = message_has_field(*message, field);
            MsgPkg input_pkg(msg_pkg.refl->MutableMessage(message, field));

            if (input_pkg.desc->options().GetExtension(proj::proto::node) != proj::proto::Node::NONE) {

                bool child_valid = graphvis_string(os, input_pkg.msg, false, has_field);

                const std::string& input_name = input_pkg.desc->name();
                std::string color = (has_field ? "" : "[color=gray]");

                is_source = false;
                valid &= (has_field and child_valid);

                os << "\t" << input_name << " -> {" << node_name << "} " << color << "\n";
            }
        }
    });

    proj::proto::Node node_type = msg_pkg.desc->options().GetExtension(proj::proto::node);

    std::string color;
    std::string shape;

    if (is_sink) {
        color = "azure";
        shape = "trapezium";

    } else if (is_source) {
        color = "darkolivegreen";
        shape = "invtrapezium";
        valid = message_existed;

    } else if (node_type == proj::proto::Node::SYNC) {
        color = "burlywood";
        shape = "box";

    } else {
        assert(node_type == proj::proto::Node::ASYNC);
        color = "lightsalmon";
        shape = "octagon";
    }

    color += (valid ? "1" : "4");

    os << "\t" << node_name << " [shape=" << shape << ", style=filled, fillcolor=" << color;
    os << ", label=<" << node_name << "<font point-size=\"10\">";

    util::iterate_msg_fields(message, [&](const gp::FieldDescriptor* field, int /*index*/) {
        if (field->cpp_type() != gp::FieldDescriptor::CPPTYPE_MESSAGE) {
            os << "<br/>" << field->name() << ": ";
            util::print_field(os, *message, field, msg_pkg.refl);
        }
    });
    os << "</font>>]\n";

    return valid;
}

} // namespace

void print_field(std::ostream& os,
                 const gp::Message& msg,
                 const gp::FieldDescriptor* field,
                 const gp::Reflection* refl) {

    if (field->is_repeated()) {
        os << "Repeated fields not implemented";

    } else { // Non-repeated field

        switch (field->cpp_type()) {
        case gp::FieldDescriptor::CPPTYPE_INT32:
            os << refl->GetInt32(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_INT64:
            os << refl->GetInt64(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_UINT32:
            os << refl->GetUInt32(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_UINT64:
            os << refl->GetUInt64(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_DOUBLE:
            os << refl->GetDouble(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_FLOAT:
            os << refl->GetFloat(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_BOOL:
            os << refl->GetBool(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_ENUM:
            os << refl->GetEnumValue(msg, field);
            break;
        case gp::FieldDescriptor::CPPTYPE_STRING:
            os << "\"" << refl->GetString(msg, field) << "\"";
            break;
        case gp::FieldDescriptor::CPPTYPE_MESSAGE:
            const gp::Message& child_msg = refl->GetMessage(msg, field);
            const gp::Reflection* child_refl = child_msg.GetReflection();
            iterate_msg_fields(child_msg, [&](const gp::FieldDescriptor* child_field, int) {
                print_field(os, child_msg, child_field, child_refl);
            });
            break;
        }
    }
}

void copy_field(gp::Message* dst, const gp::Message& src, int field_index) {
    MsgPkg dst_pkg(dst);
    dst_pkg.set_field_index(field_index);

    ConstMsgPkg src_pkg(src);
    src_pkg.set_field_index(field_index);

    assert(dst_pkg.field->cpp_type() == src_pkg.field->cpp_type());
    assert(dst_pkg.field->cpp_type() != gp::FieldDescriptor::CPPTYPE_MESSAGE);

    if (src_pkg.field->is_repeated()) {

        dst_pkg.refl->ClearField(dst, dst_pkg.field);

        int repeated_count = src_pkg.refl->FieldSize(src, src_pkg.field);

        for (int i = 0; i < repeated_count; ++i) {

            switch (src_pkg.field->cpp_type()) {
            case gp::FieldDescriptor::CPPTYPE_INT32:
                dst_pkg.refl->AddInt32(dst, dst_pkg.field, src_pkg.refl->GetRepeatedInt32(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_INT64:
                dst_pkg.refl->AddInt64(dst, dst_pkg.field, src_pkg.refl->GetRepeatedInt64(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_UINT32:
                dst_pkg.refl->AddUInt32(dst, dst_pkg.field, src_pkg.refl->GetRepeatedUInt32(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_UINT64:
                dst_pkg.refl->AddUInt64(dst, dst_pkg.field, src_pkg.refl->GetRepeatedUInt64(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_DOUBLE:
                dst_pkg.refl->AddDouble(dst, dst_pkg.field, src_pkg.refl->GetRepeatedDouble(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_FLOAT:
                dst_pkg.refl->AddFloat(dst, dst_pkg.field, src_pkg.refl->GetRepeatedFloat(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_BOOL:
                dst_pkg.refl->AddBool(dst, dst_pkg.field, src_pkg.refl->GetRepeatedBool(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_ENUM:
                dst_pkg.refl->AddEnumValue(dst,
                                           dst_pkg.field,
                                           src_pkg.refl->GetRepeatedEnumValue(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_STRING:
                dst_pkg.refl->AddString(dst, dst_pkg.field, src_pkg.refl->GetRepeatedString(src, src_pkg.field, i));
                break;
            case gp::FieldDescriptor::CPPTYPE_MESSAGE:
                gp::Message* new_msg = dst_pkg.refl->AddMessage(dst, dst_pkg.field);
                new_msg->CopyFrom(src_pkg.refl->GetRepeatedMessage(src, src_pkg.field, i));
            }
        }
    } else { // Non-repeated field

        switch (src_pkg.field->cpp_type()) {
        case gp::FieldDescriptor::CPPTYPE_INT32:
            dst_pkg.refl->SetInt32(dst, dst_pkg.field, src_pkg.refl->GetInt32(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_INT64:
            dst_pkg.refl->SetInt64(dst, dst_pkg.field, src_pkg.refl->GetInt64(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_UINT32:
            dst_pkg.refl->SetUInt32(dst, dst_pkg.field, src_pkg.refl->GetUInt32(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_UINT64:
            dst_pkg.refl->SetUInt64(dst, dst_pkg.field, src_pkg.refl->GetUInt64(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_DOUBLE:
            dst_pkg.refl->SetDouble(dst, dst_pkg.field, src_pkg.refl->GetDouble(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_FLOAT:
            dst_pkg.refl->SetFloat(dst, dst_pkg.field, src_pkg.refl->GetFloat(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_BOOL:
            dst_pkg.refl->SetBool(dst, dst_pkg.field, src_pkg.refl->GetBool(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_ENUM:
            dst_pkg.refl->SetEnumValue(dst, dst_pkg.field, src_pkg.refl->GetEnumValue(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_STRING:
            dst_pkg.refl->SetString(dst, dst_pkg.field, src_pkg.refl->GetString(src, src_pkg.field));
            break;
        case gp::FieldDescriptor::CPPTYPE_MESSAGE:
            dst_pkg.refl->MutableMessage(dst, dst_pkg.field)->CopyFrom(src_pkg.refl->GetMessage(src, src_pkg.field));
        }
    }
}

bool message_has_field(const gp::Message& msg, const gp::FieldDescriptor* field) {
    const gp::Reflection* refl = msg.GetReflection();
    return ((field->is_repeated() and refl->FieldSize(msg, field) > 0)
            or (not field->is_repeated() and refl->HasField(msg, field)));
}

std::unique_ptr<grpc::ByteBuffer> serialize_to_byte_buffer(const gp::Message& message) {
    auto buffer = std::make_unique<grpc::ByteBuffer>();
    bool own_buffer;
    grpc::SerializationTraits<gp::Message>::Serialize(message, buffer.get(), &own_buffer);
    return buffer;
}

void deserialize_from_byte_buffer(grpc::ByteBuffer* buffer, gp::Message* message) {
    grpc::SerializationTraits<gp::Message>::Deserialize(buffer, message);
}

std::string graphvis_string(const gp::Message& message) {
    if (message.GetDescriptor()->options().GetExtension(proj::proto::node) == proj::proto::Node::NONE) {
        throw std::invalid_argument("Non-node messages cannot produce graphvis strings");
    }

    std::stringstream ss;
    ss << "digraph {\n";
    auto msg_copy = clone_msg(message);
    graphvis_string(ss, msg_copy.get(), /*is_sink=*/true, /*message_existed=*/true);
    ss << "}\n";

    return ss.str();
}

} // namespace util
