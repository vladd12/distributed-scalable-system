# Getting spdlog sources from GitHub repository
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        27cb4c76708608465c413f6d0e6b8d99a4d84302 # v1.14.1
)

# Compiling spdlog library
#FetchContent_MakeAvailable(spdlog)

# Check if population has already been performed
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  # Fetch the spdlog content using previously declared details
  FetchContent_Populate(spdlog)
  # Changing some options for configuring spdlog
  option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON)
  # Bring the populated spdlog content into the build
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()
