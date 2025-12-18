
add_executable(fddconv
    bail.hpp
        fddconv.cpp
        cli_helpers.cpp
        cli_helpers.h
)
target_include_directories(fddconv PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fddconv PRIVATE dsk_tools)

# macOS requires linking against Foundation framework for Objective-C code
if(APPLE)
    target_link_libraries(fddconv PRIVATE "-framework Foundation")
endif()

# cxxopts requires C++17 (uses std::optional)
if(MSVC)
    set_target_properties(fddconv PROPERTIES CXX_STANDARD 17)
endif()

