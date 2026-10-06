# Tests of the whole track code and of the HFE loader/writer (-DENABLE_DSK_TESTS=ON)

# Needs the core only, the same part eCat3 links
add_executable(test_tracks test_tracks.cpp)
target_link_libraries(test_tracks PRIVATE dsk_tools_core)

add_executable(test_hfe_io test_hfe_io.cpp)
target_link_libraries(test_hfe_io PRIVATE dsk_tools)
target_compile_definitions(test_hfe_io PRIVATE DSK_TOOLS_DISKDEFS="${CMAKE_CURRENT_SOURCE_DIR}/../src/diskdefs")

if(APPLE)
    target_link_libraries(test_hfe_io PRIVATE "-framework Foundation")
endif()

add_test(NAME tracks COMMAND test_tracks)

# Sample images for the HFE round trips: DSK_TEST_SAMPLES names the directory
# (DISK Commander keeps them in docs/samples)
set(DSK_TEST_SAMPLES "" CACHE PATH "Directory of sample disk images for the tests")
if(DSK_TEST_SAMPLES)
    foreach(sample
            "MS-DOS/DOS_49_BC.drMFM.PC.img|TYPE_FAT:PC-1440"
            "MS-DOS/Lexicon/LEXICON_1.IMA|TYPE_FAT:PC-720"
            "Atari/Garnizon.st|TYPE_FAT:ST-720"
            "Korvet/disk.kdi|TYPE_CPM:KORVET"
            "Irisha/8/disk5.gmd|TYPE_CPM:GMD-7012")
        string(REPLACE "|" ";" parts "${sample}")
        list(GET parts 0 file)
        list(GET parts 1 type)
        string(REGEX REPLACE "^TYPE_[A-Z0-9]+:" "" variant ${type})
        add_test(NAME hfe_io_${variant} COMMAND test_hfe_io "${DSK_TEST_SAMPLES}/${file}" ${type} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
endif()
