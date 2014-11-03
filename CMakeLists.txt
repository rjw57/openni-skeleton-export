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

# Build code common to all utilities
add_subdirectory(common)
include_directories(common/include)

# GUI skeleton viewer
add_executable(skeletonexport skeletonexport/main.cpp skeletonexport/SceneDrawer.cpp)
target_link_libraries(skeletonexport
    common
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
)

# Non-GUI skeleton viewer
add_executable(skeletonexport_nogui skeletonexport_nogui.cpp)
target_link_libraries(skeletonexport_nogui common)

# vim:sw=4:sts=4:et
