cmake_minimum_required(VERSION 3.14)
project(KVStoreGRPC VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 查找系统库和头文件
# ============================================================================

# 查找 protobuf 头文件和库
find_path(PROTOBUF_INCLUDE_DIR 
    NAMES google/protobuf/message.h
    PATHS /usr/include /usr/local/include
)
find_library(PROTOBUF_LIBRARY 
    NAMES protobuf libprotobuf
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)

# 查找 grpc 头文件
find_path(GRPC_INCLUDE_DIR 
    NAMES grpc/grpc.h
    PATHS /usr/include /usr/local/include
)

# 查找 grpc 库
find_library(GRPCPP_LIBRARY 
    NAMES grpc++ libgrpc++
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)
find_library(GRPC_LIBRARY 
    NAMES grpc libgrpc
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)

# 查找 gpr 库 (gRPC 基础运行时库)
find_library(GPR_LIBRARY 
    NAMES gpr libgpr
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)

# 查找 absl 头文件
find_path(ABSL_INCLUDE_DIR 
    NAMES absl/strings/string_view.h
    PATHS /usr/include /usr/local/include
)

# 查找 absl 库
set(ABSL_LIBS_TO_FIND
    absl_strings absl_str_format_internal absl_synchronization
    absl_graphcycles_internal absl_symbolize absl_demangle_internal
    absl_malloc_internal absl_spinlock_wait absl_base absl_throw_delegate
    absl_raw_logging_internal absl_log_severity absl_cord absl_cordz_info
    absl_cordz_functions absl_cordz_handle absl_cord_internal
    absl_status absl_statusor absl_time absl_civil_time absl_time_zone
    absl_int128 absl_exponential_biased absl_bad_optional_access
    absl_bad_variant_access absl_hash absl_city absl_low_level_hash
)

set(ABSL_LIBRARIES "")
foreach(lib ${ABSL_LIBS_TO_FIND})
    find_library(ABSL_${lib}_LIBRARY NAMES ${lib}
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
    )
    if(ABSL_${lib}_LIBRARY)
        list(APPEND ABSL_LIBRARIES ${ABSL_${lib}_LIBRARY})
    endif()
endforeach()

# 查找其他依赖库
find_library(CARES_LIBRARY NAMES cares libcares
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)
find_library(RE2_LIBRARY NAMES re2 libre2
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
)

# 打印找到的库信息
message(STATUS "========================================")
message(STATUS "Found Protobuf: ${PROTOBUF_LIBRARY}")
message(STATUS "Found gRPC++: ${GRPCPP_LIBRARY}")
message(STATUS "Found gRPC: ${GRPC_LIBRARY}")
if(GPR_LIBRARY)
    message(STATUS "Found gpr: ${GPR_LIBRARY}")
endif()
message(STATUS "========================================")

# 检查必要的组件
if(NOT PROTOBUF_INCLUDE_DIR OR NOT PROTOBUF_LIBRARY)
    message(FATAL_ERROR "Protobuf not found! Install: sudo apt-get install libprotobuf-dev")
endif()
if(NOT GRPC_INCLUDE_DIR OR NOT GRPC_LIBRARY)
    message(FATAL_ERROR "gRPC not found! Install: sudo apt-get install libgrpc++-dev libgrpc-dev")
endif()

# ============================================================================
# 生成 Protobuf/gRPC 代码
# ============================================================================

find_program(PROTOC_EXECUTABLE protoc)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

if(NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc not found!")
endif()
if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found!")
endif()

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto)
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GENERATED_DIR})

set(PROTO_SRC ${PROTO_GENERATED_DIR}/kvstore.pb.cc)
set(PROTO_HDR ${PROTO_GENERATED_DIR}/kvstore.pb.h)
set(GRPC_SRC ${PROTO_GENERATED_DIR}/kvstore.grpc.pb.cc)
set(GRPC_HDR ${PROTO_GENERATED_DIR}/kvstore.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND ${PROTOC_EXECUTABLE}
    ARGS --grpc_out=${PROTO_GENERATED_DIR}
         --cpp_out=${PROTO_GENERATED_DIR}
         -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
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
    ${PROTOBUF_INCLUDE_DIR}
    ${GRPC_INCLUDE_DIR}
    ${ABSL_INCLUDE_DIR}
)

set(ALL_GRPC_LIBS ${PROTOBUF_LIBRARY})
if(GRPCPP_LIBRARY)
    list(APPEND ALL_GRPC_LIBS ${GRPCPP_LIBRARY})
endif()
list(APPEND ALL_GRPC_LIBS ${GRPC_LIBRARY})
if(GPR_LIBRARY)
    list(APPEND ALL_GRPC_LIBS ${GPR_LIBRARY})
endif()
list(APPEND ALL_GRPC_LIBS ${ABSL_LIBRARIES})
if(CARES_LIBRARY)
    list(APPEND ALL_GRPC_LIBS ${CARES_LIBRARY})
endif()
if(RE2_LIBRARY)
    list(APPEND ALL_GRPC_LIBS ${RE2_LIBRARY})
endif()

target_link_libraries(kvstore_proto PUBLIC
    ${ALL_GRPC_LIBS}
    pthread
    dl
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
