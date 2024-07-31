# Project definition and settings
cmake_minimum_required(VERSION 3.15)
project(dcs VERSION 1.0.0 LANGUAGES C CXX)

# Language settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake's fetch content options
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Adding path for cmake scripts
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#include(get_gtest) # Getting googletest library
#include(get_conan) # Getting CMake conan
#include(get_boost) # Getting boost library
include(get_spdlog) # Getting spdlog library
include(get_nlohmann_json) # Getting nlohmann_json library

# Setup variables for directories
set(DCS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(DCS_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DCS_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

set(DCS_DAEMON_HEADERS
    "${DCS_SOURCES_DIR}/config/config.hpp"
    "${DCS_SOURCES_DIR}/utils/logger.hpp"
)
set(DCS_DAEMON_SOURCES
  "${DCS_SOURCES_DIR}/config/config.cpp"
  "${DCS_SOURCES_DIR}/utils/logger.cpp"
  "${DCS_SOURCES_DIR}/utils/utils.cpp"
  "${DCS_SOURCES_DIR}/main.cpp"
)

# Creating executable target
add_executable(dcs-daemon ${DCS_DAEMON_HEADERS} ${DCS_DAEMON_SOURCES})

target_link_libraries(dcs-daemon PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json)
