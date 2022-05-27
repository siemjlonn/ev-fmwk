list (APPEND SCHEMA_FILES
    interface.json
    config.json
    manifest.json
)

list (APPEND INLINE_SCHEMA_FILES)

set (GENERATED_INLINE_SCHEMA_OUTPUT_DIR ${GENERATED_OUTPUT_DIR}/include/generated/schema)
file (MAKE_DIRECTORY ${GENERATED_INLINE_SCHEMA_OUTPUT_DIR})

foreach (SCHEMA_FILE ${SCHEMA_FILES})
    get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
    file (MD5 ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_FILE} SCHEMA_FILE_MD5)
    set (INLINE_SCHEMA_FILE ${GENERATED_INLINE_SCHEMA_OUTPUT_DIR}/${SCHEMA_NAME}.h)
    # FIXME (aw): this is close to ugly
    add_custom_command(
        COMMENT
            "Generating c++ source file for schema ${SCHEMA_FILE}"
        OUTPUT
            ${INLINE_SCHEMA_FILE}
        DEPENDS
            ${SCHEMA_FILE}
        COMMAND
            bash -c "xxd -i ${SCHEMA_FILE} | sed \"s/unsigned/static const/\" > ${INLINE_SCHEMA_FILE}"
        COMMAND
            bash -c "echo \"const char ${SCHEMA_NAME}_json_md5[] = \\\"${SCHEMA_FILE_MD5}\\\";\" >> ${INLINE_SCHEMA_FILE}"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    list (APPEND INLINE_SCHEMA_FILES ${INLINE_SCHEMA_FILE})
endforeach()

add_custom_target(generate_inline_schema_files
    DEPENDS ${INLINE_SCHEMA_FILES}
)

install(FILES ${SCHEMA_FILES}
        DESTINATION schemas
)
