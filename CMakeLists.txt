project(skeletonexport C CXX)
cmake_minimum_required(VERSION 2.8)

# We require a C++11 compiler
# Use technique from: http://stackoverflow.com/a/25836953
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

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

add_executable(skeletonexport
    src/main.cpp
    src/io.cpp
    src/SceneDrawer.cpp
)
target_link_libraries(skeletonexport
    ${LIBOPENNI_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${HDF5_LIBRARIES}
)

# vim:sw=4:sts=4:et
