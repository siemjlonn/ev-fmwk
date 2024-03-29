# check node API version based on https://nodejs.org/api/n-api.html#node-api-version-matrix

function(get_node_api_version NODE_VERSION_IN OUTPUT_NODE_API_VERSION)
    string(REPLACE "v" "" NODE_VERSION "${NODE_VERSION_IN}")
    set(${OUTPUT_NODE_API_VERSION} "" PARENT_SCOPE)
    if(     ("${NODE_VERSION}" VERSION_GREATER_EQUAL "16")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "15.12")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "12.22"))
            set(${OUTPUT_NODE_API_VERSION} 8 PARENT_SCOPE)
    elseif(     ("${NODE_VERSION}" VERSION_GREATER_EQUAL "15")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "14.12")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "12.19")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "10.23"))
            set(${OUTPUT_NODE_API_VERSION} 7 PARENT_SCOPE)
    elseif(     ("${NODE_VERSION}" VERSION_GREATER_EQUAL "14")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "12.17")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "10.20"))
            set(${OUTPUT_NODE_API_VERSION} 6 PARENT_SCOPE)
    elseif(     ("${NODE_VERSION}" VERSION_GREATER_EQUAL "13")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "12.11")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "10.17"))
            set(${OUTPUT_NODE_API_VERSION} 5 PARENT_SCOPE)
    elseif(     ("${NODE_VERSION}" VERSION_GREATER_EQUAL "12")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "11.8")
            OR ("${NODE_VERSION}" VERSION_GREATER_EQUAL "10.16"))
            set(${OUTPUT_NODE_API_VERSION} 4 PARENT_SCOPE)
    elseif(("${NODE_VERSION}" VERSION_GREATER_EQUAL "10"))
            set(${OUTPUT_NODE_API_VERSION} 3 PARENT_SCOPE)
    endif()
endfunction()

function(require_node_api_version NODE_VERSION NODE_API_VERSION_REQUIRED)
    get_node_api_version("${NODE_VERSION}" NODE_API_VERSION)
    if ("${NODE_API_VERSION}" STREQUAL "")
        message(FATAL_ERROR "Could not determine a Node-API version from the provided nodejs version '${NODE_VERSION}'")
    endif()
    if("${NODE_API_VERSION}" LESS "${NODE_API_VERSION_REQUIRED}")
        message(FATAL_ERROR "Node-API version ${NODE_API_VERSION_REQUIRED} or higher is required. However your nodejs version '${NODE_VERSION}' can only provide Node-API version '${NODE_API_VERSION}'")
    else()
        message(STATUS "Found nodejs version '${NODE_VERSION}' that can provide Node-API version '${NODE_API_VERSION}' which satifies the requirement of Node-API version '${NODE_API_VERSION_REQUIRED}'")
    endif()
endfunction()
