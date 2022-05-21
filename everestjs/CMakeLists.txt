#cmake_policy(SET CMP0042 NEW)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
add_library(everestjs SHARED
    conversion    
    everestjs
    logger
    thread_safe_callbacks
)

set (EVERESTJS_NATIVE_ADDON_NAME everestjs.node)
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


include(NodeApi)

# We need Node-API version 6 or higher
set(NODE_API_VERSION_REQUIRED 6)
require_node_api_version("${NODE_API_VERSION_REQUIRED}")

target_include_directories(everestjs
    PRIVATE
        ${NODE_ADDON_API_DIR}
        ${NODEJS_INCLUDE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# needs c++ 14
target_compile_features(everestjs PRIVATE cxx_std_14)

# define NAPI_VERSION
add_definitions(-DNAPI_VERSION=${NODE_API_VERSION_REQUIRED})
add_definitions(-DNAPI_CPP_EXCEPTIONS)

set (EVERESTJS_BUILD_PACKAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/package)
file(MAKE_DIRECTORY ${EVERESTJS_BUILD_PACKAGE_DIR})

add_custom_command(
    COMMENT
        "Packaging everestjs"
    OUTPUT
        package/package.json
        package/index.js
        package/index.d.ts
        package/${EVERESTJS_NATIVE_ADDON_NAME}
    COMMAND
        ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/package.json
            ${CMAKE_CURRENT_SOURCE_DIR}/index.js
            ${CMAKE_CURRENT_SOURCE_DIR}/index.d.ts
            ./
    COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:everestjs> ${EVERESTJS_NATIVE_ADDON_NAME}
    WORKING_DIRECTORY
        ${EVERESTJS_BUILD_PACKAGE_DIR}
    DEPENDS
        everestjs
        index.js
        index.d.ts
        package.json
)

add_custom_target(everestjs_package
    DEPENDS
        package/${EVERESTJS_NATIVE_ADDON_NAME}
)

install(TARGETS everestjs
    LIBRARY
    DESTINATION everestjs
)

install(
    FILES
        index.js
        package.json
    DESTINATION everestjs
)
