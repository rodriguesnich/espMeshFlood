cmake_minimum_required(VERSION 3.10)

project(espMeshFlood VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(EFMP_BUILD_TESTS "Build EFMP test target" ON)

# Source files for the library
file(GLOB EFMP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/cache/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/message/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/backoff/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/protocol/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/serialization/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/transport/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/espMeshFlood/*.cpp"
)

add_library(espMeshFlood STATIC ${EFMP_SOURCES})

target_include_directories(espMeshFlood
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(espMeshFlood PUBLIC cxx_std_17)

if(EFMP_BUILD_TESTS)
    # Find Google Test
    find_package(GTest REQUIRED)

    # Test executable combines all tests from test/test_all.cpp.
    add_executable(test_all test/test_all.cpp)

    target_include_directories(test_all
        PRIVATE
            ${CMAKE_SOURCE_DIR}/test
            ${CMAKE_SOURCE_DIR}/test/mocks
    )

    target_compile_definitions(test_all PRIVATE UNIT_TESTING)

    target_link_libraries(test_all
        PRIVATE
            espMeshFlood
            GTest::gtest
            GTest::gtest_main
            pthread
    )

    # Enable testing
    enable_testing()
    add_test(NAME efmp_all_tests COMMAND test_all)
endif()

include(CMakePackageConfigHelpers)

install(TARGETS espMeshFlood
    EXPORT espMeshFloodTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/espMeshFlood
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/espMeshFloodConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/espMeshFloodConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/espMeshFloodConfig.cmake"
    INSTALL_DESTINATION lib/cmake/espMeshFlood
)

install(EXPORT espMeshFloodTargets
    FILE espMeshFloodTargets.cmake
    NAMESPACE espMeshFlood::
    DESTINATION lib/cmake/espMeshFlood
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/espMeshFloodConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/espMeshFloodConfigVersion.cmake"
    DESTINATION lib/cmake/espMeshFlood
)
