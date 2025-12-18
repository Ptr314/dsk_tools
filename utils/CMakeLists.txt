
add_executable(fddconv
    bail.hpp
    fddconv.h       fddconv.cpp
)
target_include_directories(fddconv PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fddconv PRIVATE dsk_tools)

