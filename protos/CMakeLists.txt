cmake_minimum_required(VERSION 3.12)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# search directory for protobuf
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_LIST_DIR})

# make an output directory
set(PROTO_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/proto)
set(PROTO_DST_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)
file(MAKE_DIRECTORY ${PROTO_DST_DIR})

set(PROTOBUF_IMPORT_DIRS ${PROTO_SRC_DIR})

function(create_proto_library target_name proto_dir)
    # gather proto files
    file(GLOB_RECURSE GVS_PROTO_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${proto_dir}/*.proto
            )

    # generate service objects
    grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTO_DST_DIR} ${GVS_PROTO_FILES})

    add_library(${target_name} ${GRPC_HDRS} ${GRPC_SRCS})
    target_link_libraries(${target_name}
            PUBLIC Threads::Threads
            PUBLIC protobuf::libprotobuf
            PUBLIC gRPC::grpc++
            PUBLIC ${CMAKE_DL_LIBS}
            )
    target_include_directories(${target_name} SYSTEM PUBLIC ${PROTO_DST_DIR})
endfunction()

create_proto_library(proj_proto ${CMAKE_CURRENT_LIST_DIR}/proto)
target_sources(proj_proto PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/thirdparty_src/proto_reflection_descriptor_database.cc
        ${CMAKE_CURRENT_LIST_DIR}/thirdparty_src/proto_reflection_descriptor_database.h
        )
target_include_directories(proj_proto PUBLIC thirdparty_src)
