@echo off
echo Searching for Visual Studio 2022...

REM Try common VS 2022 paths
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found VS 2022 Community
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found VS 2022 Professional
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found VS 2022 Enterprise
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    goto :compile
)

echo Visual Studio 2022 not found in standard locations
goto :end

:compile
echo.
echo Compiling UI demo with MSVC...
cl /EHsc /std:c++20 simple_ui_showcase.cpp /Fe:ui_demo.exe

if exist ui_demo.exe (
    echo.
    echo === COMPILATION SUCCESSFUL! ===
    echo Running UI Framework Demo...
    echo.
    ui_demo.exe
) else (
    echo Compilation failed
)

:end
pause
