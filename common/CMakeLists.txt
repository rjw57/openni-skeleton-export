# Code common to all utilities
include_directories(include)
add_library(common
    io.cpp
    mainloop.cpp
)
target_link_libraries(common
    ${LIBOPENNI_LIBRARIES}
    ${HDF5_LIBRARIES}
)

# vim:sw=4:sts=4:et
