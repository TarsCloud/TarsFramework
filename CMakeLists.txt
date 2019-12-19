cmake_minimum_required(VERSION 2.8)

project(framework)

set(CMAKE_VERBOSE_MAKEFILE off)

set(MYSQL_DIR_INC "/usr/local/mysql/include")
set(MYSQL_DIR_LIB "/usr/local/mysql/lib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O2 -Wall -Wno-deprecated")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall -Wno-deprecated")

#set(CMAKE_BUILD_TYPE "Debug")

set(TARS_VERSION "1.2.0")
add_definitions(-DTARS_VERSION="${TARS_VERSION}")

set(INSTALL_PREFIX "/usr/local/tars/cpp")

set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})

add_subdirectory(tarscpp)
set(TARS2CPP "${tools_BINARY_DIR}/tars2cpp/tars2cpp")

include_directories(${util_SOURCE_DIR}/include)
include_directories(${servant_SOURCE_DIR})
include_directories(${MYSQL_DIR_INC})
include_directories("thirdparty/rapidjson/include")

link_libraries(tarsservant tarsutil)
link_libraries(${MYSQL_DIR_LIB}/libmysqlclient.a)
link_libraries(pthread z dl rt)

macro(complice_module MODULE)

    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/servers/${MODULE}/bin)

    include_directories(${PROJECT_SOURCE_DIR}/tarscpp/servant/protocol/framework)
    include_directories(${servant_SOURCE_DIR}/servant)

    aux_source_directory(. DIR_SRCS)

    add_executable(${MODULE} ${DIR_SRCS})
    add_dependencies(${MODULE} FRAMEWORK-PROTOCOL)
    add_dependencies(${MODULE} tars2cpp)

endmacro()

#调用tars2cpp, 生成tars对应的文件
macro(complice_tars OUT_DEPENDS_LIST)
    set(DEPENDS_LIST)

    set(SERVANT_PROTOCOL "${servant_SOURCE_DIR}/protocol/servant")

    set(CLEAN_LIST)

    set(CURRENT_PROTOCOL_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    FILE(GLOB SRC_LIST "${CURRENT_PROTOCOL_DIR}/*.tars")

    foreach (FILE ${SRC_LIST})

        #设置tars文件搜索路径
        set(INCLUDE_STRING "--include=\"${SERVANT_PROTOCOL}\"")

        get_filename_component(NAME_WE ${FILE} NAME_WE)

        # 生成tars文件
        set(TARS_IN ${FILE})
        set(TARS_H ${NAME_WE}.h)

        add_custom_command(
                OUTPUT ${CURRENT_PROTOCOL_DIR}/${TARS_H}
                WORKING_DIRECTORY ${CURRENT_PROTOCOL_DIR}
                DEPENDS tars2cpp
                COMMAND ${TARS2CPP} --with-tars ${INCLUDE_STRING} ${TARS_IN}
                COMMENT "${TARS2CPP} --with-tars ${INCLUDE_STRING} ${TARS_IN}")

        list(APPEND DEPENDS_LIST ${CURRENT_PROTOCOL_DIR}/${TARS_H})

        #设置需要清除的文件
        list(APPEND CLEAN_LIST ${CURRENT_PROTOCOL_DIR}/${NAME_WE}.h)

    endforeach ()

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEAN_LIST}")

    set(OUT_DEPENDS_LIST ${DEPENDS_LIST})
endmacro()

add_subdirectory(tarscpp/servant/protocol/framework)
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
add_subdirectory(AuthServer)
############################################################################################
# 打包deploy, 用于部署

set(DEPENDS_LIST)
list(APPEND DEPENDS_LIST tarsnode)
list(APPEND DEPENDS_LIST tarsregistry)
list(APPEND DEPENDS_LIST tarsAdminRegistry)
list(APPEND DEPENDS_LIST tarspatch)
list(APPEND DEPENDS_LIST tarsconfig)

#set(FRAMEWORK-TGZ "${CMAKE_BINARY_DIR}/framework-tmp.tgz")
set(FRAMEWORK-TGZ "${CMAKE_BINARY_DIR}/framework.tgz")

#需要压缩的文件
set(MODULES "tars_install.sh")
set(MODULES "${MODULES} tarsnode_install.sh")

foreach (MODULE ${DEPENDS_LIST})
    set(MODULES "${MODULES} ${MODULE}")
endforeach ()

############################################################################################
# 打包tarsnotify tarsstat tarsproperty tarslog tarsauth tarsquerystat tarsqueryproperty

set(DEPENDS_LIST)
list(APPEND DEPENDS_LIST tarsnotify)
list(APPEND DEPENDS_LIST tarsstat)
list(APPEND DEPENDS_LIST tarsproperty)
list(APPEND DEPENDS_LIST tarslog)
list(APPEND DEPENDS_LIST tarsquerystat)
list(APPEND DEPENDS_LIST tarsqueryproperty)
#list(APPEND DEPENDS_LIST tarsauth)

foreach (MODULE ${DEPENDS_LIST})

    set(MODULE-TGZ "${CMAKE_BINARY_DIR}/${MODULE}.tgz")

    #兼容性更好
    add_custom_command(OUTPUT ${MODULE-TGZ}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MODULE}
            COMMAND ${CMAKE_COMMAND} -E copy servers/${MODULE}/bin/${MODULE} ${MODULE}/
            COMMAND ${CMAKE_COMMAND} -E tar czf ${MODULE-TGZ} ${MODULE}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${MODULE}
            COMMENT "create ${MODULE-TGZ}")

    add_custom_target(${MODULE}-tar DEPENDS ${MODULE-TGZ} ${MODULE})
endforeach ()

############################################################################################

install(DIRECTORY build/servers/ DESTINATION deploy/framework/servers
        PATTERN "tars*"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
        
install(DIRECTORY deploy DESTINATION ./ 
        PATTERN "*.sh"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)













