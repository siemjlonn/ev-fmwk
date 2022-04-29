set(HEADERS
    "${PROJECT_SOURCE_DIR}/include/utils/config.hpp"
    "${PROJECT_SOURCE_DIR}/include/utils/exceptions.hpp"
    "${PROJECT_SOURCE_DIR}/include/utils/logging.hpp"
    "${PROJECT_SOURCE_DIR}/include/utils/message_queue.hpp"
    "${PROJECT_SOURCE_DIR}/include/utils/mqtt_abstraction.hpp"
    "${PROJECT_SOURCE_DIR}/include/utils/types.hpp"
	"${PROJECT_SOURCE_DIR}/include/utils/conversions.hpp"
    "${PROJECT_SOURCE_DIR}/include/framework/everest.hpp"
)

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

# EVerest framework
add_library(everest SHARED)
target_sources(everest
    PRIVATE
        config
        generated_module
        helpers
        interface
        module
        module_peer     
        mqtt_io_base
        mqtt_io_mqttc
        peer
        os
        schema
        types
        $<TARGET_OBJECTS:hash_library>
)
target_include_directories(everest
    PUBLIC
        ${INCLUDE_DIR}
    PRIVATE
        ${GENERATED_OUTPUT_DIR}/include
        $<TARGET_PROPERTY:hash_library,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(everest generate_inline_schema_files)

target_link_libraries(everest
    PRIVATE
        Boost::filesystem
        Boost::system
        Boost::thread
        Boost::program_options
        date
        everest::log
        fmt::fmt
        mqttc
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator::nlohmann_json_schema_validator
)

# this is important to allow our headers to find the nlohmann_* headers
# FIXME (aw): why does that work? why not just specify it once?
target_link_libraries(everest
    INTERFACE
        nlohmann_json_schema_validator::nlohmann_json_schema_validator)

# needs c++ 14
target_compile_features(everest PUBLIC cxx_std_14)

source_group(
    TREE "${INCLUDE_DIR}"
    PREFIX "Headers"
    FILES ${HEADERS})

install(TARGETS everest
    LIBRARY
    DESTINATION lib
)
