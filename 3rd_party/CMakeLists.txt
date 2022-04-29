add_library(hash_library OBJECT
    hash_library/md5.cpp
)

target_include_directories(hash_library PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
