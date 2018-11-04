
#include "theme.h"
#include <stdexcept>
#include <proj/proj_config.h>

namespace proj {

Theme::Theme() : background(parse_color(0x1f242c)) {
    ImGui::GetIO().Fonts->AddFontDefault();
    font = load_font("Roboto-Regular", proj::res_path() + "fonts/Roboto-Regular.ttf", 26.f);
}

ImColor Theme::parse_color(int rgb_hex) {
    int r = (rgb_hex & 0xFF0000) >> 16;
    int g = (rgb_hex & 0x00FF00) >> 8;
    int b = (rgb_hex & 0x0000FF) >> 0;
    return {r, g, b};
}

ImFont* Theme::load_font(const std::string& short_name, const std::string& filename, float font_size) {
    ImFontConfig font_cfg;
    strncpy(font_cfg.Name, short_name.c_str(), 35);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font
        = io.Fonts->AddFontFromFileTTF(filename.c_str(), font_size, &font_cfg, io.Fonts->GetGlyphRangesDefault());
    if (not font) {
        throw std::runtime_error("Fail to load font");
    }
    return font;
}

void Theme::set_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.f;
    style.ChildRounding = 6.f;
    style.FrameRounding = 6.f;
    style.GrabRounding = 6.f;
    style.PopupRounding = 6.f;
    style.ScrollbarRounding = 6.f;

    style.FramePadding.x = 7.f;
    style.FramePadding.y = 7.f;

    style.ItemSpacing.x = 6.f;
    style.ItemSpacing.y = 2.f;

    style.ItemInnerSpacing.x = 6.f;
    style.ItemInnerSpacing.y = 2.f;

    style.GrabMinSize = 22.f;
    style.ScrollbarSize = 18.f;
    style.FrameBorderSize = 1.f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.21f, 0.21f, 0.23f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.17f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.17f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.29f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.29f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.29f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.29f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
}

} // namespace proj