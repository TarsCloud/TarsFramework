
add_library(tarsobjects ObjectsCacheManager.cpp)

add_dependencies(tarsobjects FRAMEWORK-PROTOCOL)

install(TARGETS tarsobjects
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY . DESTINATION include/objects
        PATTERN "*.h")
