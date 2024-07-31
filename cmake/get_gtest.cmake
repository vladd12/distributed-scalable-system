# Getting googletest sources from GitHub repository
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        origin/v1.12.x
)

# Compiling googletest library
FetchContent_MakeAvailable(googletest)

# Import as GTest::GTest package
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)
