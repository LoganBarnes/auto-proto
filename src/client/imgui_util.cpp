#include "client/imgui_util.h"

namespace imgui {

ScopedID::ScopedID(const char* str_id) {
    ImGui::PushID(str_id);
}

ScopedID::ScopedID(const char* str_id_begin, const char* str_id_end) {
    ImGui::PushID(str_id_begin, str_id_end);
}

ScopedID::ScopedID(const void* ptr_id) {
    ImGui::PushID(ptr_id);
}

ScopedID::ScopedID(int int_id) {
    ImGui::PushID(int_id);
}

ScopedID::~ScopedID() {
    ImGui::PopID();
}

} // namespace imgui
