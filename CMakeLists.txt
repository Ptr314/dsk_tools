cmake_minimum_required(VERSION 3.16)

project(dsk_tools VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC
    include/dsk_tools/dsk_tools.h
    src/dsk_tools.cpp
    include/dsk_tools/loader.h
    src/loader.cpp
    include/dsk_tools/loader_raw.h
    src/loader_raw.cpp
    include/dsk_tools/disk_image.h
    src/disk_image.cpp
    include/dsk_tools/image_agat140.h
    src/image_agat140.cpp
    include/dsk_tools/definitions.h
    include/dsk_tools/utils.h
    include/dsk_tools/image_agat840.h
    src/image_agat840.cpp
    include/dsk_tools/writer.h
    include/dsk_tools/writer_mfm.h
    include/dsk_tools/writer_hxc_hfe.h
    include/dsk_tools/writer.cpp
    include/dsk_tools/writer_hxc_hfe.cpp
    include/dsk_tools/writer_mfm.cpp
    include/dsk_tools/writer_hxc_mfm.h
    include/dsk_tools/writer_hxc_mfm.cpp
    include/dsk_tools/writer_raw.h
    include/dsk_tools/writer_raw.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
