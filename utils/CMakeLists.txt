
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

# MinGW: link the GCC runtime, the C++ library and winpthread statically,
# so that the tool is a single .exe with no DLLs to carry around
if(MINGW)
    target_link_options(fddconv PRIVATE -static)
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
    target_link_options(aim2hfe PRIVATE -static)
endif()

# Release binaries: drop the code and the data nothing refers to, and the symbol table
foreach(TOOL fddconv aim2hfe)
    if(MSVC)
        target_compile_options(${TOOL} PRIVATE $<$<CONFIG:Release>:/Gy /Gw>)
        target_link_options(${TOOL} PRIVATE $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>)
    elseif(APPLE)
        target_compile_options(${TOOL} PRIVATE $<$<CONFIG:Release>:-ffunction-sections -fdata-sections>)
        target_link_options(${TOOL} PRIVATE $<$<CONFIG:Release>:-Wl,-dead_strip -Wl,-x>)
    else()
        target_compile_options(${TOOL} PRIVATE $<$<CONFIG:Release>:-ffunction-sections -fdata-sections>)
        target_link_options(${TOOL} PRIVATE $<$<CONFIG:Release>:-Wl,--gc-sections -s>)
    endif()
endforeach()

