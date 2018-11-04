#include "client/proto_gui.h"
#include "client/imgui_util.h"
#include "util/message_util.h"

#include <imgui.h>

#include <limits>

namespace gp = google::protobuf;

namespace proj {

namespace {

template <typename T, typename In, typename... Args>
bool update_field(const std::string& field_name,
                  T (gp::Reflection::*get_func)(const gp::Message&, const gp::FieldDescriptor*, Args...) const,
                  void (gp::Reflection::*set_func)(gp::Message*, const gp::FieldDescriptor*, In) const,
                  util::MsgPkg msg_pkg,
                  const GuiOptions& options,
                  Args... args) {

    T value = (msg_pkg.refl->*get_func)(*msg_pkg.msg, msg_pkg.field, args...);

    bool value_changed = ProtoGui::configure_gui(field_name, &value, options);
    (msg_pkg.refl->*set_func)(msg_pkg.msg, msg_pkg.field, value);

    return value_changed;
}

template <typename Func>
bool display_non_message_primitive(const std::string& name, const GuiOptions& options, const Func& func) {
    imgui::ScopedDisable disable(options.display_only);

    if (options.label_width > 0.f) {
        ImGui::Columns(2, nullptr, false);
        ImGui::Text("%s", name.c_str());
        ImGui::SetColumnWidth(-1, options.label_width);

        ImGui::NextColumn();
        auto hidden_label = "###" + name;
        bool value_changed = func(hidden_label);

        ImGui::Columns(1);
        return value_changed;
    }

    return func(name);
}

} // namespace

/*
 * Primitive types
 */
bool ProtoGui::configure_gui(const std::string& name, bool* update_value, const GuiOptions& options) {
    return display_non_message_primitive(name, options, [&](const std::string& label) {
        return ImGui::Checkbox(label.c_str(), update_value);
    });
}

bool ProtoGui::configure_gui(const std::string& name, float* update_value, const GuiOptions& options) {
    return display_non_message_primitive(name, options, [&](const std::string& label) {
        return ImGui::DragFloat(label.c_str(), update_value);
    });
}

bool ProtoGui::configure_gui(const std::string& name, double* update_value, const GuiOptions& options) {
    auto value = static_cast<float>(*update_value);
    auto value_changed = ProtoGui::configure_gui(name, &value, options);
    *update_value = static_cast<double>(value);
    return value_changed;
}

bool ProtoGui::configure_gui(const std::string& name, int* update_value, const GuiOptions& options) {
    return display_non_message_primitive(name, options, [&](const std::string& label) {
        return ImGui::DragInt(label.c_str(), update_value);
    });
}

bool ProtoGui::configure_gui(const std::string& name, unsigned* update_value, const GuiOptions& options) {
    return display_non_message_primitive(name, options, [&](const std::string& label) {
        auto value = static_cast<int>(*update_value);
        auto value_changed = ImGui::DragInt(label.c_str(), &value, 1.f, 0, std::numeric_limits<int>::max());
        *update_value = static_cast<unsigned>(value);
        return value_changed;
    });
}

bool ProtoGui::configure_gui(const std::string& name, long* update_value, const GuiOptions& options) {
    auto value = static_cast<int>(*update_value);
    auto result = ProtoGui::configure_gui(name, &value, options);
    *update_value = static_cast<long>(value);
    return result;
}

bool ProtoGui::configure_gui(const std::string& name, unsigned long* update_value, const GuiOptions& options) {
    auto value = static_cast<unsigned>(*update_value);
    bool value_changed = ProtoGui::configure_gui(name, &value, options);
    *update_value = static_cast<unsigned long>(value);
    return value_changed;
}

bool ProtoGui::configure_gui(const std::string& name, std::string* value, const GuiOptions& options) {
    return display_non_message_primitive(name, options, [&](const std::string& label) {
        char buf[1024];
        value->copy(buf, value->size());
        buf[value->size()] = '\0';

        auto flags = (options.strings_change_on_enter_only ? ImGuiInputTextFlags_EnterReturnsTrue : 0);

        bool value_changed = ImGui::InputText(label.c_str(), buf, sizeof(buf), flags);
        *value = std::string(buf);

        return value_changed;
    });
}
/*
 * End Scalar Types
 */

/*
 * Enum Type
 */
bool ProtoGui::configure_gui(const std::string& name,
                             const gp::EnumDescriptor* enum_desc,
                             int* value,
                             const GuiOptions& options) {

    return display_non_message_primitive(name, options, [&](const std::string& label) {
        int value_count = enum_desc->value_count();
        std::vector<std::string> str_values(static_cast<unsigned>(value_count));

        for (int i = 0; i < value_count; ++i) {
            str_values[static_cast<unsigned>(i)] = enum_desc->value(i)->name();
        }

        return ImGui::Combo(label.c_str(),
                            value,
                            [](void* data, int current_index, const char** out_str) {
                                auto strings = static_cast<std::string*>(data);
                                *out_str = strings[current_index].c_str();
                                return true;
                            },
                            str_values.data(),
                            value_count);
    });
}
/*
 * End Enum Type
 */

void ProtoGui::activate_field(util::MsgPkg msg_pkg) {

    switch (msg_pkg.field->cpp_type()) {
    case gp::FieldDescriptor::CPPTYPE_DOUBLE:
        msg_pkg.refl->SetDouble(msg_pkg.msg, msg_pkg.field, 0.0);
        break;

    case gp::FieldDescriptor::CPPTYPE_FLOAT:
        msg_pkg.refl->SetFloat(msg_pkg.msg, msg_pkg.field, 0.f);
        break;

    case gp::FieldDescriptor::CPPTYPE_INT32:
        msg_pkg.refl->SetInt32(msg_pkg.msg, msg_pkg.field, 0);
        break;

    case gp::FieldDescriptor::CPPTYPE_INT64:
        msg_pkg.refl->SetInt64(msg_pkg.msg, msg_pkg.field, 0);
        break;

    case gp::FieldDescriptor::CPPTYPE_UINT32:
        msg_pkg.refl->SetUInt32(msg_pkg.msg, msg_pkg.field, 0u);
        break;

    case gp::FieldDescriptor::CPPTYPE_UINT64:
        msg_pkg.refl->SetUInt64(msg_pkg.msg, msg_pkg.field, 0u);
        break;

    case gp::FieldDescriptor::CPPTYPE_BOOL:
        msg_pkg.refl->SetBool(msg_pkg.msg, msg_pkg.field, false);
        break;

    case gp::FieldDescriptor::CPPTYPE_STRING:
        msg_pkg.refl->SetString(msg_pkg.msg, msg_pkg.field, "");
        break;

    case gp::FieldDescriptor::CPPTYPE_MESSAGE:
        msg_pkg.refl->MutableMessage(msg_pkg.msg, msg_pkg.field);
        break;

    case gp::FieldDescriptor::CPPTYPE_ENUM:
        msg_pkg.refl->SetEnumValue(msg_pkg.msg, msg_pkg.field, 0);
        break;
    }
}

/*
 * Field types
 */
bool ProtoGui::configure_primitive_field(const std::string& field_name,
                                         util::MsgPkg msg_pkg,
                                         const GuiOptions& options) {
    assert(msg_pkg.field);

    if (not msg_pkg.refl->HasField(*msg_pkg.msg, msg_pkg.field) and not options.display_empty_fields) {
        return false;
    }

    auto call_update
        = [&](auto get_func, auto set_func) { return update_field(field_name, get_func, set_func, msg_pkg, options); };

    switch (msg_pkg.field->cpp_type()) {

    case gp::FieldDescriptor::CPPTYPE_DOUBLE:
        return call_update(&gp::Reflection::GetDouble, &gp::Reflection::SetDouble);

    case gp::FieldDescriptor::CPPTYPE_FLOAT:
        return call_update(&gp::Reflection::GetFloat, &gp::Reflection::SetFloat);

    case gp::FieldDescriptor::CPPTYPE_INT32:
        return call_update(&gp::Reflection::GetInt32, &gp::Reflection::SetInt32);

    case gp::FieldDescriptor::CPPTYPE_INT64:
        return call_update(&gp::Reflection::GetInt64, &gp::Reflection::SetInt64);

    case gp::FieldDescriptor::CPPTYPE_UINT32:
        return call_update(&gp::Reflection::GetUInt32, &gp::Reflection::SetUInt32);

    case gp::FieldDescriptor::CPPTYPE_UINT64:
        return call_update(&gp::Reflection::GetUInt64, &gp::Reflection::SetUInt64);

    case gp::FieldDescriptor::CPPTYPE_BOOL:
        return call_update(&gp::Reflection::GetBool, &gp::Reflection::SetBool);

    case gp::FieldDescriptor::CPPTYPE_STRING:
        return call_update(&gp::Reflection::GetString, &gp::Reflection::SetString);

    case gp::FieldDescriptor::CPPTYPE_MESSAGE:
        assert(false);
        break;

    case gp::FieldDescriptor::CPPTYPE_ENUM: {
        const auto* enum_desc = msg_pkg.refl->GetEnum(*msg_pkg.msg, msg_pkg.field);
        int value = msg_pkg.refl->GetEnumValue(*msg_pkg.msg, msg_pkg.field);

        if (ProtoGui::configure_gui(field_name, enum_desc->type(), &value)) {
            msg_pkg.refl->SetEnumValue(msg_pkg.msg, msg_pkg.field, value);
            return true;
        }
        return false;
    }

    } // switch

    return false;
}

} // namespace proj
