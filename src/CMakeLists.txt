add_executable(manager 
    manager/main
    manager/runtime_env
    manager/manager
    manager/subprocess
)

target_link_libraries(manager
    PRIVATE
        everest ${CMAKE_DL_LIBS}
        everest::log
        Boost::program_options
        fmt::fmt
        mqttc
)

# needs c++ 14
target_compile_features(manager PRIVATE cxx_std_14)

add_executable(sample_module 
    sample_module
)

target_link_libraries(sample_module
    PRIVATE
        everest ${CMAKE_DL_LIBS}
        everest::log
        fmt::fmt
        mqttc
)

# needs c++ 14
target_compile_features(sample_module PRIVATE cxx_std_14)

install(TARGETS manager
        RUNTIME)
