# Getting fmt sources from GitHub repository
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        0c9fce2ffefecfdce794e1859584e25877b7b592 # v11.0.2
)

# Check if population has already been performed
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  # Fetch the fmt content using previously declared details
  FetchContent_Populate(fmt)
  # Changing some options for configuring fmt
  option(FMT_INSTALL "Generate the install target." OFF)
  # Bring the populated fmt content into the build
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()
