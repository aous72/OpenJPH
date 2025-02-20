## building ojph_stream_expand
##############################

set(CMAKE_CXX_STANDARD 14)

file(GLOB OJPH_STREAM_EXPAND  "*.cpp" "*.h")
file(GLOB OJPH_SOCKETS         "../others/ojph_sockets.cpp")
file(GLOB OJPH_SOCKETS_H       "../common/ojph_sockets.h")
file(GLOB OJPH_THREADS        "../others/ojph_threads.cpp")
file(GLOB OJPH_THREADS_H      "../common/ojph_threads.h")

list(APPEND SOURCES ${OJPH_STREAM_EXPAND} ${OJPH_SOCKETS} ${OJPH_SOCKETS_H} ${OJPH_THREADS} ${OJPH_THREADS_H})

source_group("main"        FILES ${OJPH_STREAM_EXPAND})
source_group("others"      FILES ${OJPH_SOCKETS} ${OJPH_THREADS})
source_group("common"      FILES ${OJPH_SOCKETS_H} ${OJPH_THREADS_H})

add_executable(ojph_stream_expand ${SOURCES})
target_include_directories(ojph_stream_expand PRIVATE ../common)
if(MSVC)
    target_link_libraries(ojph_stream_expand PUBLIC openjph ws2_32)
else()
    target_link_libraries(ojph_stream_expand PUBLIC openjph pthread)
endif(MSVC)

install(TARGETS ojph_stream_expand)
