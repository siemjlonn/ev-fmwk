add_executable(ping
    ping
)

target_compile_features(ping PRIVATE cxx_std_14)

target_link_libraries(ping
    PRIVATE
        ${CMAKE_DL_LIBS}
        everest 
        everest::log
        mqttc
        fmt::fmt
)


add_executable(ping_pong
    ping_pong
    ../testing_helpers
)

target_compile_features(ping_pong PRIVATE cxx_std_14)

target_link_libraries(ping_pong
    PRIVATE
        ${CMAKE_DL_LIBS}
        everest 
        everest::log
        mqttc
        fmt::fmt
)




