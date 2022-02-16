
set(MODULE "tarspatch")

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/deploy/${MODULE}/bin)

#complice_module(${MODULE} "Patch.tars")
complice_module(${MODULE})

install(FILES build-docker.sh DESTINATION deploy/framework/${BUILD_SERVER}/${MODULE}/bin/)
install(FILES Dockerfile DESTINATION deploy/framework/${BUILD_SERVER}/${MODULE}/bin/)
