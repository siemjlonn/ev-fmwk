include(NodeApi)

set (PACKAGE_JSON ${CMAKE_CURRENT_SOURCE_DIR}/package.json)
set (NPM_INSTALL_DONE ${CMAKE_CURRENT_BINARY_DIR}/.npm_install_done)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PACKAGE_JSON})

if ((NOT EXISTS ${NPM_INSTALL_DONE}) OR (${PACKAGE_JSON} IS_NEWER_THAN ${NPM_INSTALL_DONE}))
    file (COPY ${PACKAGE_JSON} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    message (STATUS "Installing package dependencies for everestjs")
    execute_process(
        COMMAND
            npm install
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
        OUTPUT_QUIET
        ERROR_VARIABLE
            NPM_INSTALL_ERROR
        RESULT_VARIABLE
            NPM_INSTALL_FAILED
    )

    if (NPM_INSTALL_FAILED)
        message (FATAL_ERROR "Installing npm dependencies for everest.js failed")
    endif ()

    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E touch ${NPM_INSTALL_DONE}
    )
endif()

add_subdirectory(src/native)


# set (EVERESTJS_BUILD_PACKAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/package)
# file(MAKE_DIRECTORY ${EVERESTJS_BUILD_PACKAGE_DIR})

add_custom_command(
    COMMENT
        "Packaging everestjs"
    OUTPUT
        .everestjs_packaged
    COMMAND
        npx tsc ${CMAKE_CURRENT_SOURCE_DIR}/src/index.ts -d --outDir lib > .npm_package_log 2>&1 || (cat .npm_package_log; false)
    COMMAND
        ${CMAKE_COMMAND} -E copy $<TARGET_FILE:everestjs> lib/$<TARGET_FILE_NAME:everestjs>
    COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/everestjs.node.d.ts lib/
    COMMAND
        ${CMAKE_COMMAND} -E touch .everestjs_packaged
    DEPENDS
        everestjs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/everestjs.node.d.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/src/index.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/package.json
)

add_custom_target(everestjs_package ALL
    DEPENDS
        .everestjs_packaged
)

# install(TARGETS everestjs
#     LIBRARY
#     DESTINATION everestjs
# )

# FIXME (aw): the install needs to be done to
# install(
#     FILES
#         index.js
#         package.json
#     DESTINATION everestjs
# )
