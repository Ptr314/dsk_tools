
# Embed the diskdefs file (used by CP/M and FAT filesystems) into the CLI tool.
# The main GUI app loads it via Qt resources; fddconv has no Qt, so we bake it in.
set(DISKDEFS_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../src/diskdefs)
file(READ ${DISKDEFS_SOURCE_FILE} DISKDEFS_CONTENTS)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/diskdefs_embed.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/diskdefs_embed.h
    @ONLY
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DISKDEFS_SOURCE_FILE})

add_executable(fddconv
    bail.hpp
        fddconv.cpp
        cli_helpers.cpp
        cli_helpers.h
        ${CMAKE_CURRENT_BINARY_DIR}/diskdefs_embed.h
)
target_include_directories(fddconv PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(fddconv PRIVATE dsk_tools)

# macOS requires linking against Foundation framework for Objective-C code
if(APPLE)
    target_link_libraries(fddconv PRIVATE "-framework Foundation")
endif()

# cxxopts requires C++17 (uses std::optional)
if(MSVC)
    set_target_properties(fddconv PROPERTIES CXX_STANDARD 17)
endif()

# MinGW: Statically link GCC runtime to avoid DLL dependencies
# Note: -static-libwinpthread not supported in MinGW 4.9.2
if(MINGW)
    target_link_options(fddconv PRIVATE -static-libgcc -static-libstdc++)
endif()

# Direct AIM -> HFE converter (no image/filesystem parsing, so no diskdefs needed)
add_executable(aim2hfe
    bail.hpp
        aim2hfe.cpp
        cli_helpers.cpp
        cli_helpers.h
)
target_include_directories(aim2hfe PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(aim2hfe PRIVATE dsk_tools)

if(APPLE)
    target_link_libraries(aim2hfe PRIVATE "-framework Foundation")
endif()

if(MSVC)
    set_target_properties(aim2hfe PROPERTIES CXX_STANDARD 17)
endif()

if(MINGW)
    target_link_options(aim2hfe PRIVATE -static-libgcc -static-libstdc++)
endif()

