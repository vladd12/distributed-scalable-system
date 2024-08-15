# Collecting tests sources
set(JSON_UNIT_TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/json_init_test.cpp
)

# Compiling tests as executable file
add_executable(json-unit-test ${JSON_UNIT_TEST_SRC})
# Registering test for ctest
add_test(NAME json-unit-test COMMAND json-unit-test)
# Including some directory
target_include_directories(json-unit-test PRIVATE "${DCS_SOURCES_DIR}")
# Linking googletest library
target_link_libraries(json-unit-test PRIVATE
  GTest::GTest
  nlohmann_json::nlohmann_json
)
