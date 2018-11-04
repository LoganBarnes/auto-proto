include(cmake/DownloadProject.cmake)

if (PROJ_UPDATE_REPOS)
    set(UPDATE_STATUS 0)
else ()
    set(UPDATE_STATUS 1)
endif ()

########################
### Client Externals ###
########################
if (NOT PROJ_ARM_SERVER_ONLY)
    ########################
    ### OpenGL Externals ###
    ########################

    ### OpenGL ###
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)

    ### GL3W ###
    download_project(PROJ gl3w-dl
            GIT_REPOSITORY https://github.com/skaslev/gl3w.git
            PREFIX thirdparty
            GIT_TAG master
            UPDATE_DISCONNECTED ${UPDATE_STATUS}
            QUIET
            )

    # generate the gl3w headers and source file
    add_custom_command(
            OUTPUT
            "${gl3w-dl_BINARY_DIR}/src/gl3w.c"
            "${gl3w-dl_BINARY_DIR}/include/GL/gl3w.h"
            "${gl3w-dl_BINARY_DIR}/include/GL/glcorearb.h"
            COMMAND python ${gl3w-dl_SOURCE_DIR}/gl3w_gen.py
            DEPENDS ${gl3w-dl_SOURCE_DIR}/gl3w_gen.py
            WORKING_DIRECTORY ${gl3w-dl_BINARY_DIR}
    )

    add_library(gl3w
            ${gl3w-dl_BINARY_DIR}/src/gl3w.c
            ${gl3w-dl_BINARY_DIR}/include/GL/gl3w.h
            ${gl3w-dl_BINARY_DIR}/include/GL/glcorearb.h
            )

    target_include_directories(gl3w SYSTEM
            PUBLIC ${gl3w-dl_BINARY_DIR}/include
            )

    target_link_libraries(gl3w
            PUBLIC ${CMAKE_DL_LIBS}
            PUBLIC OpenGL::OpenGL
            )

    #########################
    ### Windowing and GUI ###
    #########################

    ### GLFW ###
    download_project(PROJ glfw-dl
            PREFIX thirdparty
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG master
            UPDATE_DISCONNECTED ${UPDATE_STATUS}
            QUIET
            )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    # compile glfw with current project
    add_subdirectory(${glfw-dl_SOURCE_DIR} ${glfw-dl_BINARY_DIR})

    ### ImGui ###
    download_project(PROJ imgui-dl
            PREFIX thirdparty
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.65
            UPDATE_DISCONNECTED ${UPDATE_STATUS}
            QUIET
            )

    add_library(imgui
            ${imgui-dl_SOURCE_DIR}/imconfig.h
            ${imgui-dl_SOURCE_DIR}/imgui.cpp
            ${imgui-dl_SOURCE_DIR}/imgui.h
            ${imgui-dl_SOURCE_DIR}/imgui_demo.cpp
            ${imgui-dl_SOURCE_DIR}/imgui_draw.cpp
            ${imgui-dl_SOURCE_DIR}/imgui_internal.h
            ${imgui-dl_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui-dl_SOURCE_DIR}/imstb_rectpack.h
            ${imgui-dl_SOURCE_DIR}/imstb_textedit.h
            ${imgui-dl_SOURCE_DIR}/imstb_truetype.h
            ${imgui-dl_SOURCE_DIR}/examples/imgui_impl_glfw.h
            ${imgui-dl_SOURCE_DIR}/examples/imgui_impl_glfw.cpp
            ${imgui-dl_SOURCE_DIR}/examples/imgui_impl_opengl3.h
            ${imgui-dl_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
            )
    target_include_directories(imgui SYSTEM PUBLIC
            ${imgui-dl_SOURCE_DIR}
            ${imgui-dl_SOURCE_DIR}/examples
            )

    target_link_libraries(imgui
            PUBLIC glfw
            PUBLIC gl3w
            )

endif (NOT PROJ_ARM_SERVER_ONLY)

###############
### Testing ###
###############
if (${PROJ_BUILD_TESTS})
    ### googletest ###
    download_project(PROJ googletest-dl
            PREFIX thirdparty
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.8.1
            UPDATE_DISCONNECTED ${UPDATE_STATUS}
            QUIET
            )

    # Prevent overriding the parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # compile googletest with current project
    add_subdirectory(${googletest-dl_SOURCE_DIR} ${googletest-dl_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()
