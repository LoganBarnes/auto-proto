#pragma once

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <functional>
#include <string>

struct GLFWwindow;

namespace proj {

class Gui {
public:
    explicit Gui(const std::string& title = "GUI", int width = 1280, int height = 760, bool resizable = true);
    virtual ~Gui();

    virtual void configure_gui(int w, int h) = 0;

    void run_loop();

    // Keyboard callbacks
    virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    virtual void char_callback(GLFWwindow* window, unsigned codepoint);

    // Mouse callbacks
    virtual void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    virtual void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    virtual void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    // Window callbacks
    virtual void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    virtual void window_focus_callback(GLFWwindow* window, int focus);
    virtual void drop_callback(GLFWwindow* window, int count, const char** paths);

private:
    std::shared_ptr<int> glfw_;
    std::shared_ptr<ImGuiContext> imgui_;
    std::shared_ptr<GLFWwindow> window_;
    std::shared_ptr<bool> imgui_gl_;

    unsigned auto_updates_ = 0u;

    void reset_update_counter();
    void set_callbacks();
};

} // namespace proj
