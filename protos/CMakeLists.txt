find_package(gRPC REQUIRED) # Finding gRPC
find_package(Protobuf REQUIRED) # Finding protobuf

get_filename_component(SERVICE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/test_grpc_service.proto" ABSOLUTE)

add_library(service_generated "${SERVICE_PROTO}")
target_link_libraries(service_generated PUBLIC gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)

protobuf_generate(TARGET service_generated)
protobuf_generate(
  TARGET service_generated
  LANGUAGE grpc
  PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
  PLUGIN_OPTIONS generate_mock_code=true
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  DEPENDENCIES ${SERVICE_PROTO}
)
