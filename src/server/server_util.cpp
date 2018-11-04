#include "server_util.h"
#include <util/message_util.h>

namespace gp = google::protobuf;

namespace util {

//void non_clearing_copy(google::protobuf::Message* dst, const google::protobuf::Message& src) {
//    iterate_msg_fields(src, [&](const gp::FieldDescriptor* src_field, int index) {
//        if (src_field->cpp_type() == gp::FieldDescriptor::CPPTYPE_MESSAGE) {
//
//            const gp::Reflection* src_refl = src.GetReflection();
//
//            const gp::Descriptor* dst_desc = dst->GetDescriptor();
//            const gp::Reflection* dst_refl = dst->GetReflection();
//            const gp::FieldDescriptor* dst_field= dst_desc->field(index);
//
//            if (message_has_field(src, src_field)) {
//                if (src_field->is_repeated()) {
//
//                    dst->
//                    int repeated_count = src_refl->FieldSize(src, src_field);
//
//                    for (int i = 0; i < repeated_count; ++i) {
//
//                    }
//
//                } else {
//                    non_clearing_copy(dst_refl->MutableMessage(dst, dst_field), src_refl->GetMessage(src,src_field));
//                }
//            } else {
//                dst_refl->ClearField(dst, dst_field);
//            }
//        } else {
//            copy_non_message_field(dst, src, index);
//        }
//    });
//}

google::protobuf::Message* get_message(google::protobuf::Message* root, const MsgPath& path) {

    auto path_count = path.size();

    for (auto i = 0u; i < path_count; ++i) {
        const gp::Descriptor* desc = root->GetDescriptor();
        const gp::Reflection* refl = root->GetReflection();

        assert(path.at(i) < desc->field_count());

        const gp::FieldDescriptor* field = desc->field(path[i]);

        if (field->is_repeated()) {
            ++i;
            assert(path.at(i) < refl->FieldSize(*root, field));
            root = refl->MutableRepeatedMessage(root, field, path[i]);
        } else {
            root = refl->MutableMessage(root, field);
        }
    }

    return root;
}

} // namespace util
