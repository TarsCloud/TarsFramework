
include_directories(${CMAKE_SOURCE_DIR}/tarscpp/util/include)

set(MODULE mysql-tool)

add_executable(${MODULE} main.cpp)

add_dependencies(${MODULE} tarsservant tarsutil)

target_link_libraries(${MODULE} tarsservant tarsutil)

if(TARS_SSL)
    target_link_libraries(${MODULE} tarsservant tarsutil ${LIB_SSL} ${LIB_CRYPTO})

    if(WIN32)
        target_link_libraries(${MODULE} Crypt32)
    endif()
endif()

if(TARS_HTTP2)
    target_link_libraries(${MODULE} ${LIB_HTTP2})
endif()

target_link_libraries(mysql-tool ${LIB_MYSQL})

install(TARGETS mysql-tool DESTINATION deploy)