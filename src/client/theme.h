#pragma once

#include <imgui.h>
#include <string>

namespace proj {

class Theme {
public:
    Theme();

    // utility functions
    static ImColor parse_color(int rgb_hex);
    static ImFont* load_font(const std::string& short_name, const std::string& filename, float font_size);

    // set ImGui::Style colors
    void set_style();

    // background color, use with glClear for example
    ImColor background;

    // font, use with ImGui::PushFont/PopFont
    ImFont* font;
};

} // namespace proj
