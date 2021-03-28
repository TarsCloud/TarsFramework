cmake_minimum_required(VERSION 3.2)

project(framework)

set(CMAKE_VERBOSE_MAKEFILE off)

set(TARS_CPP_COMMON "1")

include("${PROJECT_SOURCE_DIR}/tarscpp/cmake/GenVersion.cmake")

include("${PROJECT_SOURCE_DIR}/tarscpp/cmake/Common.cmake")

include("${PROJECT_SOURCE_DIR}/tarscpp/cmake/BuildTarsCpp.cmake")

include("${PROJECT_SOURCE_DIR}/tarscpp/cmake/Thirdparty.cmake")

include("${PROJECT_SOURCE_DIR}/tarscpp/cmake/ParseAsm.cmake")

add_subdirectory(tarscpp)

set(major "")
set(minor "")
set(patch "")
get_version(major minor patch)
set(FRAMEWORK_VERSION ${major}.${minor}.${patch})
add_definitions(-DFRAMEWORK_VERSION="${FRAMEWORK_VERSION}")
add_definitions(-DTARS_VERSION="${TARS_VERSION}")

message("FRAMEWORK_VERSION:         ${FRAMEWORK_VERSION}")
message("----------------------------------------------------")

include_directories(${servant_SOURCE_DIR})

set(BUILD_SERVER servers)

if(WIN32)
    set(TARS_INSTALL_PATH c:\\tars-install)
else(WIN32)
    set(TARS_INSTALL_PATH /usr/local/app)
endif(WIN32)

macro(complice_module MODULE)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${BUILD_SERVER}/${MODULE}/bin)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/${BUILD_SERVER}/${MODULE}/bin)
    endforeach()   

    include_directories(${PROJECT_SOURCE_DIR}/tarscpp/servant/protocol/framework)
    include_directories(${PROJECT_SOURCE_DIR}/tarscpp/util/include)
    if(WIN32)
        include_directories(${PROJECT_SOURCE_DIR}/tarscpp/util/src/epoll_windows)
    endif()

    include_directories(${servant_SOURCE_DIR}/servant)

    aux_source_directory(. DIR_SRCS)

    add_executable(${MODULE} ${DIR_SRCS})
    add_dependencies(${MODULE} FRAMEWORK-PROTOCOL)
    add_dependencies(${MODULE} tars2cpp)

    target_link_libraries(${MODULE} tarsservant tarsutil ${LIB_MYSQL})

    if(TARS_SSL)
        target_link_libraries(${MODULE} ${LIB_SSL} ${LIB_CRYPTO})
    endif()

    if(TARS_HTTP2)
        target_link_libraries(${MODULE} ${LIB_HTTP2})
    endif()

    if(NOT WIN32)
        link_libraries(pthread z dl)
    endif()
endmacro()

add_subdirectory(mysql-tool)
#add_subdirectory(tars-client)

add_subdirectory(ConfigServer)
add_subdirectory(NodeServer)
add_subdirectory(NotifyServer)
add_subdirectory(PatchServer)
add_subdirectory(AdminRegistryServer)
add_subdirectory(RegistryServer)
add_subdirectory(PropertyServer)
add_subdirectory(LogServer)
add_subdirectory(patchclient)
add_subdirectory(StatServer)
add_subdirectory(QueryStatServer)
add_subdirectory(QueryPropertyServer)
############################################################################################
# 打包deploy, 用于部署

set(DEPENDS_LIST)
list(APPEND DEPENDS_LIST tarsnode)
list(APPEND DEPENDS_LIST tarsregistry)
list(APPEND DEPENDS_LIST tarsAdminRegistry)
list(APPEND DEPENDS_LIST tarspatch)
list(APPEND DEPENDS_LIST tarsconfig)

set(FRAMEWORK-TGZ "${CMAKE_BINARY_DIR}/framework.tgz")

#执行命令
add_custom_command(OUTPUT ${FRAMEWORK-TGZ}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${BUILD_SERVER}
        COMMAND ${CMAKE_COMMAND} -E make_directory test 
        COMMAND ${CMAKE_COMMAND} -E tar czf ${FRAMEWORK-TGZ} ${DEPENDS_LIST}
        COMMAND ${CMAKE_COMMAND} -E copy ${FRAMEWORK-TGZ} ${PROJECT_BINARY_DIR}
        COMMENT "make framework-tar")

add_custom_target(framework-tar
        DEPENDS ${FRAMEWORK-TGZ} ${DEPENDS_LIST})

############################################################################################
# 打包tarsnotify tarsstat tarsproperty tarslog tarsauth tarsquerystat tarsqueryproperty

set(DEPENDS_LIST)
list(APPEND DEPENDS_LIST tarsnotify)
list(APPEND DEPENDS_LIST tarsstat)
list(APPEND DEPENDS_LIST tarsproperty)
list(APPEND DEPENDS_LIST tarslog)
list(APPEND DEPENDS_LIST tarsquerystat)
list(APPEND DEPENDS_LIST tarsqueryproperty)

foreach (MODULE ${DEPENDS_LIST})

    set(MODULE-TGZ "${CMAKE_BINARY_DIR}/${MODULE}.tgz")

    add_custom_command(OUTPUT ${MODULE-TGZ}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MODULE}
            COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_SERVER}/${MODULE}/bin/${MODULE} ${MODULE}/
            COMMAND ${CMAKE_COMMAND} -E tar czf ${MODULE-TGZ} ${MODULE}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${MODULE}
            COMMENT "create ${MODULE-TGZ}")

    add_custom_target(${MODULE}-tar DEPENDS ${MODULE-TGZ} ${MODULE})
endforeach ()

############################################################################################

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/FRAMEWORK_VERSION.txt CONTENT "${FRAMEWORK_VERSION}")

install(FILES ${CMAKE_BINARY_DIR}/FRAMEWORK_VERSION.txt DESTINATION deploy)

install(DIRECTORY ${PROJECT_BINARY_DIR}/${BUILD_SERVER}/ DESTINATION deploy/framework/${BUILD_SERVER}
        PATTERN "tars*"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)

IF(WIN32)
install(DIRECTORY deploy DESTINATION ./ 
        PATTERN "*.bat"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)

install(FILES ${PROJECT_BINARY_DIR}/src/mysql/lib/libmysql.dll DESTINATION deploy)

else(WIN32)
install(DIRECTORY deploy DESTINATION ./ 
        PATTERN "*.sh"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
install(FILES ${PROJECT_BINARY_DIR}/bin/tars2case DESTINATION deploy)

ENDIF(WIN32)

############################################################################################
#for debug

macro(copy_server server)
if(WIN32)
        add_custom_target(cp-${server}
                WORKING_DIRECTORY ${WORKING_DIRECTORY}
                DEPENDS ${server}
                # COMMAND ${TARS_INSTALL_PATH}/${server}/util/stop.bat
                COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_SERVER}/${server}/bin/${server}.exe ${TARS_INSTALL_PATH}/tars/${server}/bin/
                # COMMAND start /b ${TARS_INSTALL_PATH}/${server}/util/start.bat
                COMMENT "cp ${server} & restart: copy ${BUILD_SERVER}/${server}/bin/${server}.exe ${TARS_INSTALL_PATH}/tars/${server}/bin/")
else(WIN32)
        add_custom_target(cp-${server}
                WORKING_DIRECTORY ${WORKING_DIRECTORY}
                DEPENDS ${server}
                COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_SERVER}/${server}/bin/${server} ${TARS_INSTALL_PATH}/tars/${server}/bin/
                COMMAND ${TARS_INSTALL_PATH}/tars/${server}/util/start.sh
                COMMENT "cp ${server} & restart")
endif(WIN32)
endmacro()

copy_server(tarsAdminRegistry)
copy_server(tarsnotify)
copy_server(tarsnode)
copy_server(tarsstat)
copy_server(tarsquerystat)
copy_server(tarsqueryproperty)
copy_server(tarsregistry)
copy_server(tarslog)
copy_server(tarspatch)
copy_server(tarsproperty)





