# Getting spdlog sources from GitHub repository
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        27cb4c76708608465c413f6d0e6b8d99a4d84302 # v1.14.1
)

# Compiling spdlog library
FetchContent_MakeAvailable(spdlog)
