#pragma once

#include <google/protobuf/message.h>

namespace util {

//void non_clearing_copy(google::protobuf::Message* dst, const google::protobuf::Message& src);

using MsgPath = std::vector<int>;

google::protobuf::Message* get_message(google::protobuf::Message* root, const MsgPath& path);

} // namespace util
