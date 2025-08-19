# Test executable
add_executable(mag_tests
    test_main.cpp
    test_message.cpp
    test_policy.cpp
    test_file_operations.cpp
    test_llm_client.cpp
    test_coordinator_parsing.cpp
    test_coordinator_interfaces.cpp
    test_cli_interface.cpp
)

target_link_libraries(mag_tests
    mag_common
    GTest::gtest
    GTest::gtest_main
    GMock::gmock
    GMock::gmock_main
    ${NNG_LIBRARIES}
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Enable testing
enable_testing()
add_test(NAME MAGTests COMMAND mag_tests)