# Getting boost target with conan
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/FindBoost.cmake")
    # Getting build type, compiler, etc... (other settings)
    conan_cmake_autodetect(settings)
    # Collecting boost as header-only library
    conan_cmake_configure(
      REQUIRES boost/1.82.0
      GENERATORS cmake_find_package
    )
    # Installing libraries
    conan_cmake_install(
      PATH_OR_REFERENCE .
      BUILD missing
      REMOTE conancenter
      SETTINGS ${settings}
    )
endif()
