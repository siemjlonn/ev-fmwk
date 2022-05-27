set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# We need Node-API version 6 or higher
set(NODE_API_VERSION_REQUIRED 6)
require_node_api_version("${NODE_API_VERSION_REQUIRED}")

#FIXME (aw): the NODE_ADDON_API_DIR could be put into a cached variable
execute_process(
    COMMAND
        ${NODE} -p "require('node-addon-api').include"
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/../../
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX REPLACE "\"" "" NODE_ADDON_API_DIR "${NODE_ADDON_API_DIR}")

if (NOT EXISTS ${NODE_ADDON_API_DIR})
    message (FATAL_ERROR "Could not find node api path '${NODE_ADDON_API_DIR}'")
endif()

message (STATUS "Found node api path: ${NODE_ADDON_API_DIR}")

# define NAPI_VERSION
add_definitions(-DNAPI_VERSION=${NODE_API_VERSION_REQUIRED})
add_definitions(-DNAPI_CPP_EXCEPTIONS)

add_library(everestjs SHARED
    conversion    
    everestjs
    logger
    thread_safe_callbacks
)

# this will change the default ending of .so to .node
set_target_properties(everestjs
    PROPERTIES
        PREFIX ""
        SUFFIX ".node"
)

target_link_libraries(everestjs
    PRIVATE
        everest everest::log
)

target_include_directories(everestjs
    PRIVATE
        ${NODE_ADDON_API_DIR}
        ${NODEJS_INCLUDE_DIR}
)

# needs c++ 14
target_compile_features(everestjs PRIVATE cxx_std_14)
