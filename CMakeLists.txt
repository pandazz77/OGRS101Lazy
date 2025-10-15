cmake_minimum_required(VERSION 3.10)
project(S101Lazy)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GDAL REQUIRED gdal)

set(SRCS
    src/s101lazy.cpp
)

add_subdirectory(./S101toGML)

add_library(${PROJECT_NAME} SHARED ${SRCS})
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARIES} S101GML)
target_include_directories(${PROJECT_NAME} PRIVATE ${GDAL_INCLUDE_DIRS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ogr_${PROJECT_NAME}")

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION
        $<$<PLATFORM_ID:Linux>:/usr/lib/gdalplugins>
    LIBRARY DESTINATION
        $<$<PLATFORM_ID:Linux>:/usr/lib/gdalplugins>
    COMPONENT ${PROJECT_NAME}
)