cmake_minimum_required(VERSION 3.10)

project(efmp_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/test)
include_directories(${CMAKE_SOURCE_DIR}/test/mocks)

# Find Google Test
find_package(GTest REQUIRED)

# Exclude BLE from native builds via UNIT_TESTING define (ble_host_transport.cpp is a no-op)
add_compile_definitions(UNIT_TESTING)

# Source files for the library
file(GLOB EFMP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/cache/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/message/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/backoff/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/protocol/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/serialization/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/transport/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/host/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/*.cpp"
)

# Test executable combining all test files
add_executable(test_all
    test/test_all.cpp
    ${EFMP_SOURCES}
)

target_link_libraries(test_all
    GTest::gtest
    GTest::gtest_main
    pthread
)

# Enable testing
enable_testing()
add_test(NAME efmp_all_tests COMMAND test_all)
