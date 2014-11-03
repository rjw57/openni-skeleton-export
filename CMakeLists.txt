project(skeletonexport C CXX)
cmake_minimum_required(VERSION 2.8)

# Look for OpenNI libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBOPENNI REQUIRED libopenni)
include_directories(${LIBOPENNI_INCLUDE_DIRS})
link_directories(${LIBOPENNI_LIBRARY_DIRS})

# Find OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Find GLUT
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})

# Find HDF5 C++ bindings
find_package(HDF5 REQUIRED COMPONENTS CXX)
include_directories(${HDF5_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

add_library(common
    src/io.cpp
    src/mainloop.cpp
)

add_executable(skeletonexport
    src/main.cpp
    src/SceneDrawer.cpp
)
target_link_libraries(skeletonexport
    common
    ${LIBOPENNI_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${HDF5_LIBRARIES}
)

add_executable(skeletonexport_nogui
    src/main_nogui.cpp
)
target_link_libraries(skeletonexport_nogui
    common
    ${LIBOPENNI_LIBRARIES}
    ${HDF5_LIBRARIES}
)

# vim:sw=4:sts=4:et
