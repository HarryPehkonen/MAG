# Common library for shared utilities
add_library(mag_common STATIC
    common/message.cpp
    common/policy.cpp
    common/policy_config.cpp
    common/utils.cpp
    common/http_client.cpp
    common/llm_provider.cpp
    common/todo_manager.cpp
    common/conversation_manager.cpp
    common/input_handler.cpp
    common/readline_input_handler.cpp
    common/simple_input_handler.cpp
    common/cli_interface.cpp
    common/bash_tool.cpp
    providers/openai_provider.cpp
    providers/anthropic_provider.cpp
    providers/gemini_provider.cpp
    providers/mistral_provider.cpp
    llm_adapter/llm_client.cpp
    file_tool/file_operations.cpp
    network/nng_llm_client.cpp
    network/nng_file_client.cpp
    orchestrator/coordinator.cpp
)

target_include_directories(mag_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mag_common
    ${NNG_LIBRARIES}
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Add readline support if available
if(READLINE_FOUND)
    target_link_libraries(mag_common ${READLINE_LIBRARIES})
    target_include_directories(mag_common PRIVATE ${READLINE_INCLUDE_DIRS})
endif()

# LLM Adapter executable
add_executable(llm_adapter
    llm_adapter/main.cpp
)

target_link_libraries(llm_adapter
    mag_common
    ${NNG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
)

# File Tool executable
add_executable(file_tool
    file_tool/main.cpp
)

target_link_libraries(file_tool
    mag_common
    ${NNG_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Bash Tool executable
add_executable(bash_tool_service
    bash_tool/main.cpp
)

target_link_libraries(bash_tool_service
    mag_common
    ${NNG_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Main Orchestrator executable
add_executable(main_orchestrator
    orchestrator/main.cpp
    orchestrator/coordinator.cpp
)

target_link_libraries(main_orchestrator
    mag_common
    ${NNG_LIBRARIES}
    nlohmann_json::nlohmann_json
)