# Project definition and settings
cmake_minimum_required(VERSION 3.15)
project(dcs VERSION 1.0.0 LANGUAGES C CXX)

# Language settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DCS_TESTS "Build dcs tests" ON)

# CMake's fetch content options
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Adding path for cmake scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#include(get_gtest) # Getting googletest library
include(get_conan) # Getting CMake conan
include(get_boost) # Getting boost library using conan
include(get_fmt) # Getting fmt library
include(get_spdlog) # Getting spdlog library
include(get_nlohmann_json) # Getting nlohmann_json library

find_package(Boost COMPONENTS filesystem iostreams program_options REQUIRED) # Finding boost

# Setup variables for directories
set(DCS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(DCS_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DCS_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

set(DCS_DAEMON_HEADERS
  "${DCS_SOURCES_DIR}/core/cmd_line_args.hpp"
  "${DCS_SOURCES_DIR}/core/errors.hpp"
  "${DCS_SOURCES_DIR}/core/logger.hpp"
  "${DCS_SOURCES_DIR}/core/sys_api.hpp"

  "${DCS_SOURCES_DIR}/config/configuration_v1.hpp"

  "${DCS_SOURCES_DIR}/config/json/fixed_string.hpp"
  "${DCS_SOURCES_DIR}/config/json/json_field_base.hpp"
  "${DCS_SOURCES_DIR}/config/json/json_struct.hpp"

  "${DCS_SOURCES_DIR}/dfs/block.hpp"
  "${DCS_SOURCES_DIR}/dfs/data_node.hpp"
  "${DCS_SOURCES_DIR}/dfs/name_node.hpp"

  "${DCS_SOURCES_DIR}/fs/file_system.hpp"
  "${DCS_SOURCES_DIR}/fs/local_file_system.hpp"
  "${DCS_SOURCES_DIR}/fs/distributed_file_system.hpp"
  "${DCS_SOURCES_DIR}/fs/file_system_provider.hpp"

  "${DCS_SOURCES_DIR}/ipc/ipc.hpp"
)
set(DCS_DAEMON_SOURCES
  "${DCS_SOURCES_DIR}/core/cmd_line_args.cpp"
  "${DCS_SOURCES_DIR}/core/logger.cpp"

  "${DCS_SOURCES_DIR}/config/configuration_v1.cpp"

  "${DCS_SOURCES_DIR}/dfs/name_node.cpp"

  "${DCS_SOURCES_DIR}/fs/local_file_system.cpp"
  "${DCS_SOURCES_DIR}/fs/distributed_file_system.cpp"
  "${DCS_SOURCES_DIR}/fs/file_system_provider.cpp"

  "${DCS_SOURCES_DIR}/ipc/ipc.cpp"

  "${DCS_SOURCES_DIR}/main.cpp"
)

# Creating executable target
add_executable(dcs-daemon ${DCS_DAEMON_HEADERS} ${DCS_DAEMON_SOURCES})

target_include_directories(dcs-daemon PRIVATE "${DCS_SOURCES_DIR}")

target_link_libraries(dcs-daemon PRIVATE
  Boost::program_options
  Boost::filesystem
  Boost::iostreams
  fmt::fmt
  spdlog::spdlog
  nlohmann_json::nlohmann_json
)

# Copy default config after build
add_custom_command(
  TARGET dcs-daemon POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/config/default_config.json
    ${CMAKE_CURRENT_BINARY_DIR}/default_config.json
)

# If enabled building tests for some features
if(BUILD_DCS_TESTS)
  message(STATUS "Tests building for dcs is enabled")
  include(CTest)
  include(get_gtest)
  enable_testing(true)
  add_subdirectory(tests)
endif()
