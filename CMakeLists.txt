cmake_minimum_required(VERSION 3.12)
project(ProtoServer)

option(PROJ_BUILD_TESTS "Build Googletest unit tests" OFF)
option(PROJ_ARM_SERVER_ONLY "Compile only the server using C++14" ON)

#############################
### Project Configuration ###
#############################
if (PROJ_ARM_SERVER_ONLY)
    set(CMAKE_CXX_STANDARD 14)
else ()
    set(CMAKE_CXX_STANDARD 17)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()
message("-- Build type: ${CMAKE_BUILD_TYPE}")

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (NOT MSVC)
    set(PROJ_COMPILE_FLAGS -Wall -Wextra -Werror -Wpedantic -Winit-self
            -Wold-style-cast -Woverloaded-virtual -Wsign-conversion
            -Wmissing-declarations -Wmissing-include-dirs -Wshadow)
else ()
    set(PROJ_COMPILE_FLAGS /WX)
endif ()

# "Glob is terrible/root of all evil" yeah yeah. CONFIGURE_DEPENDS in cmake 3.12
# helps to fix that and it is super useful when refactoring
cmake_policy(SET CMP0009 NEW)

set(AUTO_GENERATION_MESSAGE "***This file is auto generated. If you modify it the changes will be overwritten.***")
configure_file(src/proj_config.h.in ${CMAKE_BINARY_DIR}/proj/proj_config.h)

##################
### Clang-Tidy ###
##################
find_program(CLANG_TIDY_EXE
        NAMES "clang-tidy"
        DOC "Path to clang-tidy executable"
        )
if (NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
else ()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-format-style=file" "-fix" "-fix-errors")
endif ()

###########################
### Protobuf Generation ###
###########################
find_package(gRPC REQUIRED)
add_subdirectory(protos)

#######################
### Thirdpart Repos ###
#######################
add_subdirectory(external)

###############
### Project ###
###############

### UTIL ###
file(GLOB_RECURSE UTIL_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util/*
        )

add_library(util ${UTIL_SOURCE_FILES})

target_include_directories(util PUBLIC src PUBLIC ${CMAKE_BINARY_DIR})
target_link_libraries(util PUBLIC proj_proto)
set_target_properties(util PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
target_compile_options(util PUBLIC ${PROJ_COMPILE_FLAGS})

### SERVER ###
file(GLOB_RECURSE SERVER_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server/*
        )

add_library(server ${SERVER_SOURCE_FILES})

target_include_directories(server PUBLIC src)
target_link_libraries(server PUBLIC util PUBLIC gRPC::grpc++_reflection)
set_target_properties(server PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

# Server only exec
add_executable(run_server src/server.cpp)
target_link_libraries(run_server server)
set_target_properties(run_server PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

### CLIENTS ###
if (NOT PROJ_ARM_SERVER_ONLY)

    file(GLOB_RECURSE CLIENT_SOURCE_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/client/*
            )

    add_library(client ${CLIENT_SOURCE_FILES})

    target_include_directories(client PUBLIC src)
    target_link_libraries(client PUBLIC util PUBLIC imgui)
    set_target_properties(client PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    # Client only exec
    add_executable(run_client src/client.cpp)
    target_link_libraries(run_client client)
    set_target_properties(run_client PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    # Client and server exec
    add_executable(client_and_server src/client_and_server.cpp)
    target_link_libraries(client_and_server client server)
    set_target_properties(client_and_server PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    # Generic client test exec
    add_executable(stream_test src/generic_stream_test.cpp)
    target_link_libraries(stream_test util)
    set_target_properties(stream_test PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif ()

###############
### Testing ###
###############
if (${PROJ_BUILD_TESTS})
    file(GLOB_RECURSE TEST_SOURCE_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/testing/*
            )

    add_executable(proto_server_tests ${TEST_SOURCE_FILES})
    target_link_libraries(proto_server_tests client server gmock_main)
    set_target_properties(proto_server_tests PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif ()

####################
### Clang-Format ###
####################
# only format things if this is the parent directory
get_directory_property(has_parent PARENT_DIRECTORY)
if (NOT has_parent)
    find_program(CLANG_FORMAT_EXE
            NAMES "clang-format"
            DOC "Path to clang-format executable"
            )
    if (NOT CLANG_FORMAT_EXE)
        message(STATUS "clang-format not found.")
    else ()
        add_custom_target(format_everything
                COMMAND ./format.sh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                )
        add_dependencies(proj_proto format_everything)
    endif ()
endif ()
