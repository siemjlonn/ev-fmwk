set(SETUP_NAME "broken_manifest_2")
set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SETUP_NAME})

configure_file(test_configs/${SETUP_NAME}_config.yaml ${SETUP_NAME}/config.yaml COPYONLY)
configure_file(test_logging.ini ${SETUP_NAME}/logging.ini COPYONLY)
file(COPY ../schemas/ DESTINATION ${SETUP_NAME}/schemas)
file(COPY test_modules/TESTBrokenManifest2 DESTINATION ${SETUP_NAME}/modules)
file(COPY test_interfaces/test_interface.yaml DESTINATION ${SETUP_NAME}/interfaces)
file(MAKE_DIRECTORY "${PREFIX_DIR}/types")
file(MAKE_DIRECTORY "${PREFIX_DIR}/errors")
file(MAKE_DIRECTORY "${PREFIX_DIR}/interfaces")
file(MAKE_DIRECTORY "${PREFIX_DIR}/www")
file(MAKE_DIRECTORY "${PREFIX_DIR}/etc/everest")
file(MAKE_DIRECTORY "${PREFIX_DIR}/share/everest")
