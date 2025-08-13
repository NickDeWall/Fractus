@echo off
setlocal enabledelayedexpansion

set "RELEASE_PACKAGE_DIR=build/debug"
set "BUILD_DIR=build/debug"
set "MINGW_BIN_DIR=C:\msys64\mingw64\bin"
set "EXECUTABLE_NAME=Fractus.exe"
set "DEPENDENCIES_FILE=scripts\dependencies.txt"

echo.
echo === Starting Dynamic Release Packaging ===
echo.

rem Create the destination directory for the package
if not exist "%RELEASE_PACKAGE_DIR%" (
    echo Creating directory: "%RELEASE_PACKAGE_DIR%"
    mkdir "%RELEASE_PACKAGE_DIR%"
) else (
    echo Using existing directory: "%RELEASE_PACKAGE_DIR%"
)

rem Copy the compiled executable
echo.
echo Copying executable: "%EXECUTABLE_NAME%"
copy "%BUILD_DIR%\%EXECUTABLE_NAME%" "%RELEASE_PACKAGE_DIR%"

echo.
echo Parsing ldd output for DLL dependencies and copying...

rem Read dependencies.txt and copy each DLL
for /f "tokens=1" %%a in ('findstr /i ".dll" "%DEPENDENCIES_FILE%"') do (
    set "dll_name=%%a"

    rem Clean up a bit if needed (e.g., remove spaces or tabs)
    set "dll_name=!dll_name: =!"

    rem Ldd output has the DLL name on a line by itself or as part of a path
    rem We extract just the filename from the path if needed
    for %%b in ("!dll_name!") do set "filename=%%~nxb"

    if exist "%MINGW_BIN_DIR%\!filename!" (
        echo Found and copying: !filename!
        copy "%MINGW_BIN_DIR%\!filename!" "%RELEASE_PACKAGE_DIR%"
    ) else (
        rem This typically happens for system DLLs like kernel32.dll
        echo Warning: Could not find !filename! in MinGW bin. Skipping.
    )
)

echo.
echo === Packaging complete! ===
echo Your self-contained application is ready in the '%RELEASE_PACKAGE_DIR%' folder.
pause