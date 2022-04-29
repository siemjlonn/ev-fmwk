
setup_target_for_coverage_gcovr_html(
    NAME gcovr_coverage
    EXECUTABLE ctest --output-on-failure
    EXCLUDE
        "${CMAKE_SOURCE_DIR}/tests"
        "${CMAKE_SOURCE_DIR}/3rd_party"
        "${CMAKE_SOURCE_DIR}/src/manager/main.cpp"
        "${CMAKE_BINARY_DIR}"
    DEPENDENCIES tests everest
)

setup_target_for_coverage_lcov(
    NAME lcov_coverage
    EXECUTABLE ctest --output-on-failure
    EXCLUDE
    # FIXME (aw): DRY
        "${CMAKE_SOURCE_DIR}/tests"
        "${CMAKE_SOURCE_DIR}/3rd_party"
        "${CMAKE_SOURCE_DIR}/src/manager/main.cpp"
        "${CMAKE_BINARY_DIR}"
    DEPENDENCIES tests everest
)
# setup catch2 library
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)

add_library(catch2_loader OBJECT catch2_loader testing_helpers)
target_include_directories(catch2_loader
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(catch2_loader
    PUBLIC
        Catch2::Catch2
    PRIVATE
        Boost::filesystem
        nlohmann_json::nlohmann_json
)

add_definitions(-DUNIT_TESTING_MODE=1)
add_definitions(-DTEST_RESOURCES_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")

# define tests
add_executable(tests
    test_config
    test_helpers
    test_message_worker
    test_module
    test_module_peer
    test_peer
    test_schema
)

target_link_libraries(tests
    PRIVATE
        catch2_loader
        everest ${CMAKE_DL_LIBS}
        everest::log
        fmt::fmt
)

catch_discover_tests(tests)

# add example binaries
add_subdirectory(examples)
