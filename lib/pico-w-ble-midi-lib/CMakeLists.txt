cmake_minimum_required(VERSION 3.13)

add_library(ble_midi_pkt_codec INTERFACE)
target_sources(ble_midi_pkt_codec INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ble_midi_pkt_codec.c
)
target_include_directories(ble_midi_pkt_codec INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(ble_midi_pkt_codec INTERFACE
    pico_stdlib
)

add_library(ble_midi_service_lib INTERFACE)
target_sources(ble_midi_service_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/midi_service_server.c
    ${CMAKE_CURRENT_LIST_DIR}/midi_service_stream_handler.c
)

target_include_directories(ble_midi_service_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(ble_midi_service_lib INTERFACE
    ble_midi_pkt_codec
    pico_stdlib
    pico_btstack_ble
    ring_buffer_lib
)

add_library(ble_midi_server_lib INTERFACE)
target_sources(ble_midi_server_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ble_midi_server.c
)
target_include_directories(ble_midi_server_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(ble_midi_server_lib INTERFACE
    ble_midi_service_lib
)

add_library(ble_midi_client_lib INTERFACE)
target_sources(ble_midi_client_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ble_midi_client.c
)
target_include_directories(ble_midi_client_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(ble_midi_client_lib INTERFACE
    ble_midi_pkt_codec
    pico_stdlib
    pico_btstack_ble
    ring_buffer_lib
)
