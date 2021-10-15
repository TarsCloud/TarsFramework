set(MODULE "tarstrace")

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/deploy/${MODULE}/bin/)


#include_directories(asio/include)
include_directories(rapidjson/include)

complice_module(${MODULE})

# cmake_minimum_required(VERSION 2.8)
# project(trace-server)

# if (WIN32)
#     include(c:\\tars\\cpp\\makefile\\tars-tools.cmake)
# else ()
#     include(/usr/local/tars/cpp/makefile/tars-tools.cmake)
# endif ()

# include_directories(/usr/local/tars/cpp/thirdparty/include)
# link_directories(/usr/local/tars/cpp/thirdparty/lib)
# link_directories(/lib/x86_64-linux-gnu)

# add_subdirectory(src)
