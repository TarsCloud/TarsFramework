
include_directories(${PROJECT_SOURCE_DIR}/patchclient)
include_directories(${PROJECT_SOURCE_DIR}/objects)

link_libraries(patch tarsobjects)

set(MODULE "tarsnode")

complice_module(${MODULE})

#tarsnode依赖patch的客户端
add_dependencies(${MODULE} patch tarsobjects)

#install(FILES pull-docker.sh DESTINATION deploy/framework/${BUILD_SERVER}/${MODULE}/bin/)
