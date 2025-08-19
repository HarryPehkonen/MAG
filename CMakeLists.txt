cmake_minimum_required(VERSION 3.16)
project(MAG VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module for downloading dependencies
include(FetchContent)

# Set policy for FetchContent timestamps
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# ==================== CURL ====================
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(STATUS "CURL not found, will use system package manager dependencies")
    message(STATUS "Please install libcurl development package:")
    message(STATUS "  Ubuntu/Debian: sudo apt-get install libcurl4-openssl-dev")
    message(STATUS "  CentOS/RHEL: sudo yum install libcurl-devel") 
    message(STATUS "  macOS: brew install curl")
    message(FATAL_ERROR "CURL is required but not found")
endif()

# ==================== READLINE (optional) ====================
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(READLINE readline)
endif()

if(READLINE_FOUND)
    add_definitions(-DHAS_READLINE)
    message(STATUS "Readline found - enhanced CLI features enabled")
else()
    message(STATUS "Readline not found - using simple input fallback")
endif()

# ==================== nlohmann/json ====================
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found locally, downloading...")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    )
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json downloaded successfully")
else()
    message(STATUS "Found system nlohmann/json")
endif()

# ==================== NNG ====================
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(NNG QUIET nng)
endif()

if(NOT NNG_FOUND)
    message(STATUS "NNG not found locally, downloading...")
    FetchContent_Declare(
        nng
        GIT_REPOSITORY https://github.com/nanomsg/nng.git
        GIT_TAG v1.7.3
        GIT_SHALLOW TRUE
    )
    
    # Configure NNG options
    set(NNG_TESTS OFF CACHE BOOL "Build NNG tests")
    set(NNG_TOOLS OFF CACHE BOOL "Build NNG tools")
    set(NNG_ENABLE_STATS OFF CACHE BOOL "Enable NNG statistics")
    
    FetchContent_MakeAvailable(nng)
    message(STATUS "NNG downloaded and configured successfully")
    
    # Set variables for compatibility
    set(NNG_LIBRARIES nng)
    set(NNG_INCLUDE_DIRS ${nng_SOURCE_DIR}/include)
    
else()
    message(STATUS "Found system NNG")
endif()

# ==================== FTXUI ====================
### find_package(ftxui QUIET)
### if(NOT ftxui_FOUND)
###     message(STATUS "FTXUI not found locally, downloading...")
###     FetchContent_Declare(
###         ftxui
###         GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
###         GIT_TAG v5.0.0
###         GIT_SHALLOW TRUE
###     )
###     
###     # Configure FTXUI options
###     set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "Build FTXUI examples")
###     set(FTXUI_BUILD_TESTS OFF CACHE BOOL "Build FTXUI tests")
###     set(FTXUI_BUILD_DOCS OFF CACHE BOOL "Build FTXUI documentation")
###     set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "Enable FTXUI install")
###     
###     FetchContent_MakeAvailable(ftxui)
###     message(STATUS "FTXUI downloaded and configured successfully")
### else()
###     message(STATUS "Found system FTXUI")
### endif()

# ==================== GTest/GMock ====================
# First try pkg-config (most reliable for system installations)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTEST QUIET gtest)
    pkg_check_modules(GTEST_MAIN QUIET gtest_main)
    pkg_check_modules(GMOCK QUIET gmock)
    pkg_check_modules(GMOCK_MAIN QUIET gmock_main)
endif()

# If pkg-config found both GTest and GMock, use them
if(GTEST_FOUND AND GMOCK_FOUND)
    message(STATUS "Found system GTest and GMock via pkg-config")
    
    # Create imported targets
    add_library(GTest::gtest INTERFACE IMPORTED)
    target_link_libraries(GTest::gtest INTERFACE ${GTEST_LIBRARIES})
    target_include_directories(GTest::gtest INTERFACE ${GTEST_INCLUDE_DIRS})
    
    add_library(GTest::gtest_main INTERFACE IMPORTED)
    target_link_libraries(GTest::gtest_main INTERFACE ${GTEST_MAIN_LIBRARIES})
    target_include_directories(GTest::gtest_main INTERFACE ${GTEST_MAIN_INCLUDE_DIRS})
    
    add_library(GMock::gmock INTERFACE IMPORTED)
    target_link_libraries(GMock::gmock INTERFACE ${GMOCK_LIBRARIES})
    target_include_directories(GMock::gmock INTERFACE ${GMOCK_INCLUDE_DIRS})
    
    add_library(GMock::gmock_main INTERFACE IMPORTED)
    target_link_libraries(GMock::gmock_main INTERFACE ${GMOCK_MAIN_LIBRARIES})
    target_include_directories(GMock::gmock_main INTERFACE ${GMOCK_MAIN_INCLUDE_DIRS})
    
    set(GTEST_SYSTEM_FOUND TRUE)
else()
    # Fallback to CMake find_package
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Found system GTest via find_package, checking for GMock...")
        
        # Try to find GMock libraries manually
        find_library(GMOCK_LIBRARY gmock PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        find_library(GMOCK_MAIN_LIBRARY gmock_main PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        
        if(GMOCK_LIBRARY AND GMOCK_MAIN_LIBRARY)
            message(STATUS "Found system GMock libraries")
            
            # Create GMock targets
            add_library(GMock::gmock INTERFACE IMPORTED)
            target_link_libraries(GMock::gmock INTERFACE ${GMOCK_LIBRARY})
            target_include_directories(GMock::gmock INTERFACE /usr/include)
            
            add_library(GMock::gmock_main INTERFACE IMPORTED)
            target_link_libraries(GMock::gmock_main INTERFACE ${GMOCK_MAIN_LIBRARY})
            target_include_directories(GMock::gmock_main INTERFACE /usr/include)
            
            set(GTEST_SYSTEM_FOUND TRUE)
        else()
            message(STATUS "GMock libraries not found, will download")
        endif()
    endif()
endif()

# Download if system installation not complete
if(NOT GTEST_SYSTEM_FOUND)
    message(STATUS "Downloading GoogleTest with GMock...")
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    
    # Configure GTest options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Install GTest")
    set(BUILD_GMOCK ON CACHE BOOL "Build GMock")
    
    FetchContent_MakeAvailable(googletest)
    message(STATUS "GoogleTest with GMock downloaded and configured successfully")
    
    # Create aliases for compatibility
    if(NOT TARGET GTest::gtest)
        add_library(GTest::gtest ALIAS gtest)
    endif()
    if(NOT TARGET GTest::gtest_main)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()
    if(NOT TARGET GMock::gmock)
        add_library(GMock::gmock ALIAS gmock)
    endif()
    if(NOT TARGET GMock::gmock_main)
        add_library(GMock::gmock_main ALIAS gmock_main)
    endif()
endif()

# ==================== Include Directories ====================
include_directories(${CMAKE_SOURCE_DIR}/include)

# Handle NNG include directories properly
if(NNG_FOUND)
    include_directories(${NNG_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NNG_CFLAGS_OTHER}")
else()
    # When using downloaded NNG, include its directories
    include_directories(${nng_SOURCE_DIR}/include)
endif()

# Print summary
message(STATUS "=== MAG Build Configuration ===")
message(STATUS "CURL: ${CURL_FOUND}")
message(STATUS "nlohmann/json: ${nlohmann_json_FOUND}")
message(STATUS "NNG: ${NNG_FOUND}")
### message(STATUS "FTXUI: ${ftxui_FOUND}")
message(STATUS "Readline: ${READLINE_FOUND}")
message(STATUS "GTest: ${GTest_FOUND}")
message(STATUS "GMock: ${GMOCK_AVAILABLE}")
message(STATUS "===============================")

# Create subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Install targets
install(TARGETS llm_adapter file_tool main_orchestrator
        RUNTIME DESTINATION bin)
