
set(MODULE "tarsregistry")

include_directories(${PROJECT_SOURCE_DIR}/objects)

link_libraries(objects)

complice_module(${MODULE})
