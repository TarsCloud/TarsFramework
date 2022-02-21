
include_directories(${PROJECT_SOURCE_DIR}/patchclient)

link_libraries(patch)

set(MODULE "tarsnode")

complice_module(${MODULE})

#tarsnode依赖patch的客户端
add_dependencies(${MODULE} patch)

#install(FILES pull-docker.sh DESTINATION deploy/framework/${BUILD_SERVER}/${MODULE}/bin/)
