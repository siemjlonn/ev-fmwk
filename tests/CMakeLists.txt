include(CTest)
include(Catch)

add_executable(test_config)

target_include_directories(test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(test_config PRIVATE
    test_config.cpp
    helpers.cpp
)

target_link_libraries(test_config
    PRIVATE
        everest::framework
        everest::log
        Catch2::Catch2WithMain
)

catch_discover_tests(test_config)

include(test_directory_setups/empty_config.cmake)
include(test_directory_setups/broken_yaml.cmake)
include(test_directory_setups/missing_module.cmake)
include(test_directory_setups/broken_manifest_1.cmake)
include(test_directory_setups/broken_manifest_2.cmake)
include(test_directory_setups/missing_interface.cmake)
include(test_directory_setups/valid_config.cmake)
include(test_directory_setups/empty_yaml_object.cmake)
include(test_directory_setups/empty_yaml.cmake)
include(test_directory_setups/null_yaml.cmake)
include(test_directory_setups/string_yaml.cmake)
