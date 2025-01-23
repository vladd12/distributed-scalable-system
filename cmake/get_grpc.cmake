# Getting boost target with conan
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/FindgRPC.cmake")
    # Getting build type, compiler, etc... (other settings)
    conan_cmake_autodetect(settings)
    # Collecting grpc library
    conan_cmake_configure(
      REQUIRES grpc/1.67.1
      GENERATORS cmake_find_package
      OPTIONS grpc:shared=False grpc:codegen=True grpc:cpp_plugin=True grpc:csharp_ext=False
      grpc:php_plugin=False grpc:node_plugin=False grpc:otel_plugin=False grpc:ruby_plugin=False
      grpc:csharp_plugin=False grpc:python_plugin=False grpc:objective_c_plugin=False
    )
    # Installing libraries
    conan_cmake_install(
      PATH_OR_REFERENCE .
      BUILD missing
      REMOTE conancenter
      SETTINGS ${settings}
    )
endif()
