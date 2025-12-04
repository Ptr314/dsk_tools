cmake_minimum_required(VERSION 3.16)

project(dsk_tools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC
    include/dsk_tools/dsk_tools.h       src/dsk_tools.cpp
    src/definitions.h
    src/charmaps.h
    src/bas_tokens.h
    src/utils.h                         src/utils.cpp
    src/bit_enums.h

    src/loaders/loader.h                src/loaders/loader.cpp
    src/loaders/loader_raw.h            src/loaders/loader_raw.cpp
    src/loaders/loader_aim.h            src/loaders/loader_aim.cpp
    src/loaders/loader_hxc_hfe.h        src/loaders/loader_hxc_hfe.cpp
    src/loaders/loader_fil.h            src/loaders/loader_fil.cpp
    src/loaders/loader_nib.h            src/loaders/loader_nib.cpp
    src/loaders/loader_mfm.h            src/loaders/loader_mfm.cpp
    src/loaders/loader_nic.h            src/loaders/loader_nic.cpp
    src/loaders/loader_hxc_mfm.h        src/loaders/loader_hxc_mfm.cpp

    src/images/disk_image.h             src/images/disk_image.cpp
    src/images/image_agat140.h          src/images/image_agat140.cpp
    src/images/image_agat840.h          src/images/image_agat840.cpp
    src/images/image_fil.h              src/images/image_fil.cpp

    src/writers/writer.h                src/writers/writer.cpp
    src/writers/writer_hxc_hfe.h        src/writers/writer_hxc_hfe.cpp
    src/writers/writer_mfm.h            src/writers/writer_mfm.cpp
    src/writers/writer_hxc_mfm.h        src/writers/writer_hxc_mfm.cpp
    src/writers/writer_raw.h            src/writers/writer_raw.cpp

    src/filesystems/filesystem.h        src/filesystems/filesystem.cpp
    src/filesystems/fs_dos33.h          src/filesystems/fs_dos33.cpp
    src/filesystems/fs_spriteos.h       src/filesystems/fs_spriteos.cpp
    src/filesystems/fs_cpm.h            src/filesystems/fs_cpm.cpp
    src/filesystems/fs_fil.h            src/filesystems/fs_fil.cpp
    src/viewers/viewer.h
    src/viewers/viewer_binary.h         src/viewers/viewer_binary.cpp
    src/viewers/viewer_text.h           src/viewers/viewer_text.cpp
    src/viewers/viewer_basic_agat.h
    src/viewers/viewer_basic_agat.cpp
    src/viewers/viewer_basic_apple.h
    src/viewers/viewer_basic_apple.cpp
    src/viewers/viewer_basic_mbasic.h
    src/viewers/viewer_basic_mbasic.cpp
    src/viewers/viewer_pics_agat.h
    src/viewers/viewer_pics_agat.cpp
    src/viewers/viewer_pics.h
    src/viewers/viewer_pics.cpp
    src/viewers/agat_fonts.h
        src/filesystems/fs_host.h
        src/filesystems/fs_host.cpp
)

# Platform-specific sources and libraries
if(APPLE)
    target_sources(${PROJECT_NAME} PRIVATE
        src/filesystems/fs_host_helpers_mac.mm
    )
    # macOS needs to enable Objective-C++ compilation
    set_source_files_properties(src/filesystems/fs_host_helpers_mac.mm PROPERTIES LANGUAGE OBJCXX)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE shell32)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loaders/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/images/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/writers/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystems/
)

set_source_files_properties(include/dsk_tools/dsk_tools.h PROPERTIES COMPILE_FLAGS "-Wno-unused-include")

option(ENABLE_DSK_TOOLS "Enable command line tools" ON)

if(ENABLE_DSK_TOOLS)
    add_subdirectory(utils)
endif()
