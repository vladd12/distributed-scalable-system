# Project definition and settings
cmake_minimum_required(VERSION 3.15)
project(dss VERSION 1.0.0 LANGUAGES C CXX)

# Language settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DSS_TESTS "Build dcs tests" ON)

# CMake's fetch content options
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Adding path for cmake scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Getting dependencies
include(get_conan) # Getting CMake conan
include(get_boost) # Getting boost library using conan
include(get_fmt) # Getting fmt library
include(get_spdlog) # Getting spdlog library
include(get_nlohmann_json) # Getting nlohmann_json library

find_package(Boost COMPONENTS headers filesystem iostreams program_options REQUIRED) # Finding boost

# Setup variables for directories
set(DSS_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(DSS_CONFIG_DIR "${DSS_SOURCES_DIR}/config")
set(DSS_CONFIG_SOURCES
  "${DSS_CONFIG_DIR}/configuration.hpp"
  "${DSS_CONFIG_DIR}/configuration.cpp"
)

add_library(dss_config STATIC ${DSS_CONFIG_SOURCES})
target_include_directories(dss_config PUBLIC "${DSS_SOURCES_DIR}")
target_link_libraries(dss_config PUBLIC nlohmann_json::nlohmann_json)

set(DSS_CORE_DIR "${DSS_SOURCES_DIR}/core")
set(DSS_CORE_SOURCES
  "${DSS_CORE_DIR}/cmd_line_args.hpp"
  "${DSS_CORE_DIR}/cmd_line_args.cpp"
  "${DSS_CORE_DIR}/errors.hpp"
  "${DSS_CORE_DIR}/logger.hpp"
  "${DSS_CORE_DIR}/logger.cpp"
  "${DSS_CORE_DIR}/sys_api.hpp"
)

add_library(dss_core STATIC ${DSS_CORE_SOURCES})
target_include_directories(dss_core PUBLIC "${DSS_SOURCES_DIR}")
target_link_libraries(dss_core PUBLIC
  Boost::program_options
  spdlog::spdlog
  dss_config
)

set(DSS_DFS_DIR "${DSS_SOURCES_DIR}/dfs")
set(DSS_DFS_SOURCES
  "${DSS_DFS_DIR}/block.hpp"
  "${DSS_DFS_DIR}/data_node.hpp"
  "${DSS_DFS_DIR}/name_node.hpp"
  "${DSS_DFS_DIR}/name_node.cpp"
)

add_library(dss_dfs STATIC ${DSS_DFS_SOURCES})
target_include_directories(dss_dfs PUBLIC "${DSS_SOURCES_DIR}")

set(DSS_FS_DIR "${DSS_SOURCES_DIR}/fs")
set(DSS_FS_SOURCES
  "${DSS_FS_DIR}/file_system.hpp"
  "${DSS_FS_DIR}/local_file_system.hpp"
  "${DSS_FS_DIR}/local_file_system.cpp"
  "${DSS_FS_DIR}/distributed_file_system.hpp"
  "${DSS_FS_DIR}/distributed_file_system.cpp"
  "${DSS_FS_DIR}/file_system_provider.hpp"
  "${DSS_FS_DIR}/file_system_provider.cpp"
)

add_library(dss_fs STATIC ${DSS_FS_SOURCES})
target_include_directories(dss_fs PUBLIC "${DSS_SOURCES_DIR}")
target_link_libraries(dss_fs PUBLIC
  Boost::filesystem
  Boost::iostreams
  fmt::fmt
  dss_config
  dss_core
  dss_dfs
)

set(DSS_HTTP_DIR "${DSS_SOURCES_DIR}/http")
set(DSS_HTTP_SOURCES
  "${DSS_HTTP_DIR}/common.hpp"
  "${DSS_HTTP_DIR}/common.cpp"
  "${DSS_HTTP_DIR}/request.hpp"
  "${DSS_HTTP_DIR}/request.cpp"
  "${DSS_HTTP_DIR}/response.hpp"
  "${DSS_HTTP_DIR}/response.cpp"
  "${DSS_HTTP_DIR}/server.hpp"
  "${DSS_HTTP_DIR}/server.cpp"
  "${DSS_HTTP_DIR}/client.hpp"
  "${DSS_HTTP_DIR}/client.cpp"
)

add_library(dss_http STATIC ${DSS_HTTP_SOURCES})
target_include_directories(dss_http PUBLIC "${DSS_SOURCES_DIR}")
target_link_libraries(dss_http PUBLIC
  Boost::headers
  dss_core
)
if(MSVC)
  target_compile_definitions(dss_http PUBLIC _WIN32_WINNT=0x0601)
endif()


# Creating executable target
add_executable(dss_daemon "${DSS_SOURCES_DIR}/main.cpp")
target_include_directories(dss_daemon PRIVATE "${DSS_SOURCES_DIR}")
target_link_libraries(dss_daemon PRIVATE
  dss_fs
  dss_http
)

# Creating executable target
add_executable(dss_client "${DSS_SOURCES_DIR}/client.cpp")
target_include_directories(dss_client PRIVATE "${DSS_SOURCES_DIR}")
target_link_libraries(dss_client PRIVATE dss_http)

# Creating executable target
add_executable(dss_server "${DSS_SOURCES_DIR}/server.cpp")
target_include_directories(dss_server PRIVATE "${DSS_SOURCES_DIR}")
target_link_libraries(dss_server PRIVATE dss_http)

# Copy default config after build
add_custom_command(
  TARGET dss_daemon POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E copy
    -t "${CMAKE_CURRENT_BINARY_DIR}" # destination
    "${CMAKE_CURRENT_SOURCE_DIR}/config/data_node_config.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/name_node_config.json"
)

# If enabled building tests for some features
if(BUILD_DSS_TESTS)
  message(STATUS "Tests building for dcs is enabled")
  include(CTest)
  include(get_gtest)
  enable_testing(true)
  add_subdirectory(tests)
endif()
