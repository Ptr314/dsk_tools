@ECHO OFF

call vars-msvc-latest.cmd

SET _ARCHITECTURE=x86_64
SET _COMPILER=msvc
SET _PLATFORM=windows
SET _BUILD_DIR=.\build\%_PLATFORM%_%_ARCHITECTURE%_%_COMPILER%

set /p _VERSION=<..\VERSION

SET _RELEASE_NAME=dsk_tools-%_VERSION%-%_PLATFORM%-%_ARCHITECTURE%-%_COMPILER%
SET _RELEASE_DIR=.\release\%_RELEASE_NAME%
SET _RELEASE_ZIP=.\release\%_RELEASE_NAME%.zip

cmake -S ../ -B "%_BUILD_DIR%" -G "Ninja Multi-Config"
if errorlevel 1 exit /b 1

cmake --build "%_BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

REM Cleaning the results of a previous build
if exist "%_RELEASE_DIR%" rmdir /s /q "%_RELEASE_DIR%"
if exist "%_RELEASE_ZIP%" del /q "%_RELEASE_ZIP%"

mkdir "%_RELEASE_DIR%"

copy "%_BUILD_DIR%\utils\Release\fddconv.exe" "%_RELEASE_DIR%"
copy "%_BUILD_DIR%\utils\Release\aim2hfe.exe" "%_RELEASE_DIR%"

set "SEVENZIP=7z"
"%SEVENZIP%" >nul 2>&1
if errorlevel 9009 (
    if exist "C:\Program Files\7-Zip\7z.exe" (
        set "SEVENZIP=C:\Program Files\7-Zip\7z.exe"
    ) else if exist "C:\Program Files (x86)\7-Zip\7z.exe" (
        set "SEVENZIP=C:\Program Files (x86)\7-Zip\7z.exe"
    ) else (
        echo ERROR: 7z.exe not found. Please install 7-Zip or add it to PATH.
        exit /b 1
    )
)

pushd "%_RELEASE_DIR%"
"%SEVENZIP%" a "..\%_RELEASE_NAME%.zip" * -mx9
popd
