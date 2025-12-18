@ECHO OFF

call vars-msvc-latest.cmd

SET _ARCHITECTURE=x86_64
SET _COMPILER=msvc
SET _PLATFORM=windows
SET _BUILD_DIR=.\build\%_PLATFORM%_%_ARCHITECTURE%_%_COMPILER%

set /p _VERSION=<..\VERSION

SET _RELEASE_NAME=dsk_tools-%_VERSION%-%_PLATFORM%-%_ARCHITECTURE%-%_COMPILER%
SET _RELEASE_DIR=.\release\%_RELEASE_NAME%

if not exist "%_BUILD_DIR%\" (
    cmake -S ../ -B "%_BUILD_DIR%" -G "Ninja Multi-Config"

    cd "%_BUILD_DIR%"
    ninja -f build-Release.ninja

    cd ..\..\
)

mkdir "%_RELEASE_DIR%"

copy "%_BUILD_DIR%\utils\Release\fddconv.exe" "%_RELEASE_DIR%"

