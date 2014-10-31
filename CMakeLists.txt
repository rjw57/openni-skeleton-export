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

add_executable(skeletonexport
    src/main.cpp
    src/io.cpp
    src/SceneDrawer.cpp
)
target_link_libraries(skeletonexport
    ${LIBOPENNI_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
)

# vim:sw=4:sts=4:et
