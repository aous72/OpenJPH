## building ojph_stream_expand
##############################

if (OJPH_BUILD_STREAM_EXPAND)

    include_directories(../common)
    include_directories(../../core/common)
    add_compile_options(-std=c++11)

    file(GLOB OJPH_STREAM_EXPAND  "ojph_stream_expand.cpp")
    file(GLOB OJPH_SOCKET         "../others/ojph_socket.cpp")
    file(GLOB OJPH_SOCKET_H       "../common/ojph_socket.h")

    list(APPEND SOURCES ${OJPH_STREAM_EXPAND} ${OJPH_SOCKET} ${OJPH_SOCKET_H})

    source_group("main"        FILES ${OJPH_STREAM_EXPAND})
    source_group("others"      FILES ${OJPH_SOCKET})
    source_group("common"      FILES ${OJPH_SOCKET_H})

    add_executable(ojph_stream_expand ${SOURCES})
    if(MSVC)
        target_link_libraries(ojph_stream_expand PUBLIC openjph ws2_32)
    else()
        target_link_libraries(ojph_stream_expand PUBLIC openjph)
    endif(MSVC)

endif(OJPH_BUILD_STREAM_EXPAND)
