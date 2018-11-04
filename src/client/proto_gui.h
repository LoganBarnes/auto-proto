#pragma once

#include <util/message_util.h>

namespace proj {

struct GuiOptions {
    bool auto_update = false;
    bool show_types = false;
    bool display_empty_fields = false;
    bool headers_expanded_by_default = false;
    bool strings_change_on_enter_only = true;
    bool display_all_oneof_messsages = false;
    float label_width = 0.f;
    bool display_only = false;
};

class ProtoGui {
public:
    /*
     * Scalar Types
     */
    static bool configure_gui(const std::string& name, bool* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, float* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, double* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, int* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, unsigned* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, long* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, unsigned long* update_value, const GuiOptions& options = {});
    static bool configure_gui(const std::string& name, std::string* value, const GuiOptions& options = {});

    /*
     * Enum Type
     */
    static bool configure_gui(const std::string& name,
                              const google::protobuf::EnumDescriptor* enum_desc,
                              int* value,
                              const GuiOptions& options = {});

    static void activate_field(util::MsgPkg msg_pkg);
    static bool
    configure_primitive_field(const std::string& field_name, util::MsgPkg msg_pkg, const GuiOptions& options);
};

} // namespace proj
