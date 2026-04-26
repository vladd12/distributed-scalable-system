set(HTTP_UNIT_TEST_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/http_common_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/http_headers_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/basic_http_session_test.cpp"
)

add_executable(http-unit-test ${HTTP_UNIT_TEST_SRC})
target_link_libraries(http-unit-test PRIVATE GTest::GTest dss_http)

add_test(NAME http-unit-test COMMAND http-unit-test)
