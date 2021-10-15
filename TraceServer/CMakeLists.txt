cmake_minimum_required(VERSION 2.8)
project(trace-server)

if (WIN32)
    include(c:\\tars\\cpp\\makefile\\tars-tools.cmake)
else ()
    include(/usr/local/tars/cpp/makefile/tars-tools.cmake)
endif ()

include_directories(/usr/local/tars/cpp/thirdparty/include)
link_directories(/usr/local/tars/cpp/thirdparty/lib)
link_directories(/lib/x86_64-linux-gnu)

add_subdirectory(src)