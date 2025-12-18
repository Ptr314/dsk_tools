
add_executable(fddconv
    bail.hpp
    fddconv.h       fddconv.cpp
)
target_include_directories(fddconv PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fddconv PRIVATE dsk_tools)

# cxxopts requires C++17 (uses std::optional)
if(MSVC)
    set_target_properties(fddconv PROPERTIES CXX_STANDARD 17)
endif()

