cmake_minimum_required(VERSION 3.16)
project(CppLoadBalancer)
set(CMAKE_CXX_STANDARD 20)


include_directories(include)
include(FetchContent)

# --- Fetch nlohmann_json ---
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- Fetch GoogleTest ---
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

add_compile_definitions(UNIT_TEST)


enable_testing()
include(GoogleTest)


add_executable(config_manager_test
    tests/unit/config_manager_test.cpp
    src/config_manager.cpp
)
target_include_directories(config_manager_test PRIVATE include)
target_link_libraries(config_manager_test PRIVATE gtest_main nlohmann_json::nlohmann_json)
gtest_discover_tests(config_manager_test)

# --- BackendPool tests ---
add_executable(backend_pool_test
    tests/unit/backend_pool_test.cpp
    src/backend_pool.cpp
)
target_include_directories(backend_pool_test PRIVATE include)
target_link_libraries(backend_pool_test PRIVATE gtest_main nlohmann_json::nlohmann_json)
gtest_discover_tests(backend_pool_test)

# --- Router tests ---
add_executable(router_test
    tests/unit/router_test.cpp
    src/router.cpp
    src/backend_pool.cpp
)
target_include_directories(router_test PRIVATE include)
target_link_libraries(router_test PRIVATE gtest_main nlohmann_json::nlohmann_json)
gtest_discover_tests(router_test)

add_executable(acceptor_test
    tests/unit/acceptor_test.cpp
    src/acceptor.cpp
    src/logger.cpp
    src/router.cpp
    src/backend_pool.cpp
    src/connection.cpp
    src/connection_pool.cpp
    src/network_utils.cpp
)
target_include_directories(acceptor_test PRIVATE include)
target_link_libraries(acceptor_test PRIVATE gtest_main pthread nlohmann_json::nlohmann_json)
gtest_discover_tests(acceptor_test)

add_executable(connection_pool_test
    tests/unit/connection_pool_test.cpp
    src/connection_pool.cpp
    src/network_utils.cpp
)
target_include_directories(connection_pool_test PRIVATE include)
target_link_libraries(connection_pool_test PRIVATE gtest_main pthread nlohmann_json::nlohmann_json)
gtest_discover_tests(connection_pool_test)

add_executable(connection_test
    tests/unit/connection_test.cpp
    src/connection.cpp
    src/connection_pool.cpp
    src/network_utils.cpp
    src/logger.cpp
)
target_include_directories(connection_test PRIVATE include)
target_link_libraries(connection_test PRIVATE gtest_main gmock pthread nlohmann_json::nlohmann_json)
gtest_discover_tests(connection_test)

add_executable(reactor_test
    tests/unit/reactor_test.cpp
    src/reactor.cpp
    src/connection_pool.cpp
    src/network_utils.cpp
)
target_include_directories(reactor_test PRIVATE include tests/mocks)
target_link_libraries(reactor_test PRIVATE gtest gmock gtest_main pthread nlohmann_json::nlohmann_json)
gtest_discover_tests(reactor_test)

add_executable(load_balancer
    src/main.cpp
    src/logger.cpp
    src/config_manager.cpp
    src/backend_pool.cpp
    src/router.cpp
    src/acceptor.cpp
    src/connection.cpp
    src/reactor.cpp
    src/connection_pool.cpp
    src/network_utils.cpp
    src/event_loop_factory.cpp
)

if(APPLE)
    target_sources(load_balancer PRIVATE src/kqueue_event_loop.cpp)
elseif(UNIX)
    target_sources(load_balancer PRIVATE src/epoll_event_loop.cpp)
elseif(WIN32)
    target_sources(load_balancer PRIVATE src/iocp_event_loop.cpp) 
endif()

target_compile_features(load_balancer PRIVATE cxx_std_20)
target_link_libraries(load_balancer PRIVATE pthread nlohmann_json::nlohmann_json)
