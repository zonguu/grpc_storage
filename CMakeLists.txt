cmake_minimum_required(VERSION 3.14)
project(KVStoreGRPC VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 查找依赖
# ============================================================================

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
message(STATUS "Found gRPC: ${gRPC_VERSION}")

# ============================================================================
# 生成 Protobuf/gRPC 代码
# ============================================================================

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto)
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GENERATED_DIR})

set(PROTO_SRC ${PROTO_GENERATED_DIR}/kvstore.pb.cc)
set(PROTO_HDR ${PROTO_GENERATED_DIR}/kvstore.pb.h)
set(GRPC_SRC ${PROTO_GENERATED_DIR}/kvstore.grpc.pb.cc)
set(GRPC_HDR ${PROTO_GENERATED_DIR}/kvstore.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND protobuf::protoc
    ARGS --grpc_out=${PROTO_GENERATED_DIR}
         --cpp_out=${PROTO_GENERATED_DIR}
         -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating Protobuf and gRPC code"
)

# ============================================================================
# 创建协议库
# ============================================================================

add_library(kvstore_proto STATIC ${PROTO_SRC} ${GRPC_SRC})

target_include_directories(kvstore_proto PUBLIC
    ${PROTO_GENERATED_DIR}
)

target_link_libraries(kvstore_proto PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# ============================================================================
# 可执行文件
# ============================================================================

set(SERVER_SOURCES
    server/main.cpp
    server/file_storage_engine.cpp
    server/memory_storage_engine.cpp
)

add_executable(kvstore_server ${SERVER_SOURCES})

target_include_directories(kvstore_server PRIVATE
    server
    ${PROTO_GENERATED_DIR}
)

target_link_libraries(kvstore_server PRIVATE kvstore_proto)

add_executable(kvstore_client client/main.cpp)

target_include_directories(kvstore_client PRIVATE ${PROTO_GENERATED_DIR})

target_link_libraries(kvstore_client PRIVATE kvstore_proto)
