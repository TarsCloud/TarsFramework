
set(MODULE tars-client)

include_directories(${CMAKE_SOURCE_DIR}/tarscpp/util/include)

if(WIN32)
include_directories(${CMAKE_SOURCE_DIR}/tarscpp/util/src/epoll_windows)
endif(WIN32)

include_directories(${CMAKE_SOURCE_DIR}/tarscpp/servant/protocol)

include_directories(.)

aux_source_directory(. DIR_SRC)

add_executable(${MODULE} ${DIR_SRC})

add_dependencies(${MODULE} tarsservant tarsutil FRAMEWORK-PROTOCOL)

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

target_link_libraries(tars-client ${LIB_MYSQL})

install(TARGETS tars-client DESTINATION deploy)