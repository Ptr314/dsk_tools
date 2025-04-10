cmake_minimum_required(VERSION 3.16)

project(dsk_tools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC
    include/dsk_tools/dsk_tools.h       src/dsk_tools.cpp
    src/definitions.h
    src/charmaps.h
    src/bas_tokens.h
    src/utils.h                         src/utils.cpp

    src/loader.h                        src/loader.cpp
    src/loader_raw.h                    src/loader_raw.cpp
    src/loader_aim.h                    src/loader_aim.cpp
    src/loader_gcr.h                    src/loader_gcr.cpp
    src/loader_gcr_nic.h                src/loader_gcr_nic.cpp
    src/loader_gcr_nib.h                src/loader_gcr_nib.cpp
    src/loader_gcr_mfm.h                src/loader_gcr_mfm.cpp
    src/loader_hxc_hfe.h                src/loader_hxc_hfe.cpp
    src/loader_fil.h                    src/loader_fil.cpp

    src/disk_image.h                    src/disk_image.cpp
    src/image_agat140.h                 src/image_agat140.cpp
    src/image_agat840.h                 src/image_agat840.cpp

    src/writer.h                        src/writer.cpp
    src/writer_hxc_hfe.h                src/writer_hxc_hfe.cpp
    src/writer_mfm.h                    src/writer_mfm.cpp
    src/writer_hxc_mfm.h                src/writer_hxc_mfm.cpp
    src/writer_raw.h                    src/writer_raw.cpp

    src/filesystem.h                    src/filesystem.cpp
    src/fs_dos33.h                      src/fs_dos33.cpp
    src/fs_spriteos.h                   src/fs_spriteos.cpp
    src/fs_cpm.h                        src/fs_cpm.cpp
    src/image_fil.h                     src/image_fil.cpp
    src/fs_fil.h                        src/fs_fil.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_source_files_properties(include/dsk_tools/dsk_tools.h PROPERTIES COMPILE_FLAGS "-Wno-unused-include")

option(ENABLE_DSK_TOOLS "Enable command line tools" ON)

if(ENABLE_DSK_TOOLS)
    add_subdirectory(utils)
endif()
