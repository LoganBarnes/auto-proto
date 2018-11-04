#pragma once

#include "util/generic_guard.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace imgui {

class ScopedFont {
public:
    explicit ScopedFont(ImFont* font) : guard_(util::make_guard(&ImGui::PushFont, &ImGui::PopFont, font)) {}

private:
    util::GenericGuard<decltype(&ImGui::PushFont), decltype(&ImGui::PopFont), ImFont*&> guard_;
};

class ScopedDisable {
public:
    explicit ScopedDisable(bool disable)
        : guard_(util::make_guard(&ScopedDisable::push, &ScopedDisable::pop, disable)) {}

private:
    static void push(bool disable) {
        if (disable) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
    }

    static void pop(bool disable) {
        if (disable) {
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
        }
    }
    util::GenericGuard<decltype(&ScopedDisable::push), decltype(&ScopedDisable::pop), bool&> guard_;
};

class ScopedIndent {
public:
    explicit ScopedIndent(float indent_w = 0.f)
        : guard_(util::make_guard(&ImGui::Indent, &ImGui::Unindent, indent_w)) {}

private:
    util::GenericGuard<decltype(&ImGui::Indent), decltype(&ImGui::Unindent), float&> guard_;
};

class ScopedID {
public:
    explicit ScopedID(const char* str_id);
    explicit ScopedID(const char* str_id_begin, const char* str_id_end);
    explicit ScopedID(const void* ptr_id);
    explicit ScopedID(int int_id);
    ~ScopedID();
};

} // namespace imgui
