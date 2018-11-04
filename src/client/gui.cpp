#include "client/gui.h"

#include <iostream>
#include <chrono>
#include <algorithm>

namespace proj {

Gui::Gui(const std::string& title, int width, int height, bool resizable) {
    // Set the error callback before any other GLFW calls so we get proper error reporting
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "ERROR: (" << error << ") " << description << std::endl;
    });

    glfw_ = std::shared_ptr<int>(new int(glfwInit()), [](auto* p) {
        glfwTerminate();
        delete p;
    });

    if (*glfw_ == 0) {
        throw std::runtime_error("GLFW init failed");
    }

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    imgui_ = std::shared_ptr<ImGuiContext>(ImGui::CreateContext(), ImGui::DestroyContext);

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    if (width == 0 || height == 0) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        width = mode->width;
        height = mode->height;
    }

    if (title.empty()) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // highest on mac :(
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif // __APPLE__

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, resizable);

    window_ = std::shared_ptr<GLFWwindow>(glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr),
                                          [glfw_copy = glfw_](auto* p) {
                                              if (p) {
                                                  glfwDestroyWindow(p);
                                              }
                                          });

    if (window_ == nullptr) {
        throw std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(window_.get());
    glfwSwapInterval(1);

    if (gl3wInit()) {
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    auto init_imgui_gl = [this]() {
        ImGui_ImplGlfw_InitForOpenGL(window_.get(), false);
        ImGui_ImplOpenGL3_Init();
        return new bool(true);
    };

    /// \todo: call ImGui_ImplGlfwGL3_Init on new windows and whenever window focus changes
    if (!imgui_gl_) {
        imgui_gl_ = std::shared_ptr<bool>(init_imgui_gl(), [window_copy = window_](auto* p) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            delete p;
        });
    }

    ImGui::StyleColorsDark();

    set_callbacks();
}

Gui::~Gui() = default;

void Gui::run_loop() {
    do {
        int w, h;
        glfwGetFramebufferSize(window_.get(), &w, &h);
        glViewport(0, 0, w, h);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        configure_gui(w, h);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_.get());

        if (auto_updates_ == 0) {
            glfwWaitEvents();
            reset_update_counter();
        } else {
            --auto_updates_;
            glfwPollEvents();
        }
    } while (!glfwWindowShouldClose(window_.get()));
}

void Gui::key_callback(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {}
void Gui::char_callback(GLFWwindow* /*window*/, unsigned /*codepoint*/) {}

void Gui::mouse_button_callback(GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/) {}
void Gui::cursor_pos_callback(GLFWwindow* /*window*/, double /*xpos*/, double /*ypos*/) {}
void Gui::scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double /*yoffset*/) {}

void Gui::framebuffer_size_callback(GLFWwindow* /*window*/, int /*width*/, int /*height*/) {}
void Gui::window_focus_callback(GLFWwindow* /*window*/, int /*focus*/) {}
void Gui::drop_callback(GLFWwindow* /*window*/, int /*count*/, const char** /*paths*/) {}

void Gui::reset_update_counter() {
    auto_updates_ = 5u;
}

void Gui::set_callbacks() {
    glfwSetWindowUserPointer(window_.get(), this);

    // Keyboard callbacks
    glfwSetKeyCallback(window_.get(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        // do this first so it can be overridden in subsequent callbacks
        if (key == GLFW_KEY_Q && (mods & GLFW_MOD_SHIFT) && (mods & GLFW_MOD_CONTROL)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            gui->key_callback(window, key, scancode, action, mods);
        }
    });

    glfwSetCharCallback(window_.get(), [](GLFWwindow* window, unsigned codepoint) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        ImGui_ImplGlfw_CharCallback(window, codepoint);
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            gui->char_callback(window, codepoint);
        }
    });

    // Mouse callbacks
    glfwSetMouseButtonCallback(window_.get(), [](GLFWwindow* window, int button, int action, int mods) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        if (!ImGui::GetIO().WantCaptureMouse) {
            gui->mouse_button_callback(window, button, action, mods);
        }
    });

    glfwSetCursorPosCallback(window_.get(), [](GLFWwindow* window, double xpos, double ypos) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        if (!ImGui::GetIO().WantCaptureMouse) {
            gui->cursor_pos_callback(window, xpos, ypos);
        }
    });

    glfwSetScrollCallback(window_.get(), [](GLFWwindow* window, double xoffset, double yoffset) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        if (!ImGui::GetIO().WantCaptureMouse) {
            gui->scroll_callback(window, xoffset, yoffset);
        }
    });

    // Window callbacks
    glfwSetFramebufferSizeCallback(window_.get(), [](GLFWwindow* window, int width, int height) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        gui->framebuffer_size_callback(window, width, height);
    });

    glfwSetWindowFocusCallback(window_.get(), [](GLFWwindow* window, int focus) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        gui->window_focus_callback(window, focus);
    });

    glfwSetDropCallback(window_.get(), [](GLFWwindow* window, int count, const char** paths) {
        auto gui = static_cast<Gui*>(glfwGetWindowUserPointer(window));
        gui->reset_update_counter();
        gui->drop_callback(window, count, paths);
    });
}

} // namespace proj
