@ECHO OFF

call vars-mingw-latest.cmd

SET _ARCHITECTURE=x86_64
SET _COMPILER=mingw
SET _PLATFORM=windows
SET _BUILD_DIR=.\build\%_PLATFORM%_%_ARCHITECTURE%_%_COMPILER%
SET CC=%_ROOT_MINGW%\gcc.exe

set /p _VERSION=<..\VERSION

SET _RELEASE_NAME="dsk_tools-%_VERSION%-%_PLATFORM%-%_ARCHITECTURE%-%_COMPILER%"
SET _RELEASE_DIR=".\release\%_RELEASE_NAME%"

if not exist %_BUILD_DIR%\ (
    set CC=%_ROOT_MINGW%\gcc.exe
    cmake -S ../ -B "%_BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release

    cd "%_BUILD_DIR%"
    ninja

    cd ..\..\
)

mkdir "%_RELEASE_DIR%"

copy "%_BUILD_DIR%\utils\fddconv.exe" "%_RELEASE_DIR%"

