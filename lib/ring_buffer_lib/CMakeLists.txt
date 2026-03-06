cmake_minimum_required(VERSION 3.13)

if(NOT TARGET ring_buffer_lib)
    add_library(ring_buffer_lib INTERFACE)
    target_sources(ring_buffer_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ring_buffer_lib.c
    )
    target_include_directories(ring_buffer_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})
endif()
