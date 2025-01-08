set(ABSL_ENABLE_INSTALL ON)
set(RE2_BUILD_TESTING OFF)

# Getting grpc sources from GitHub repository
FetchContent_Declare(
    gRPC
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG        b4ef7c141d960be62e0008601261bb22cecb5d40 # v1.69.0
)

# Compiling grpc library
FetchContent_MakeAvailable(gRPC)

# Need for gRPC
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()
