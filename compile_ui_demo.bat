@echo off
echo Setting up Visual Studio 2022 environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

echo Compiling UI demo...
cl /EHsc /std:c++20 simple_ui_showcase.cpp /Fe:ui_demo.exe

if exist ui_demo.exe (
    echo Compilation successful! Running demo...
    echo.
    ui_demo.exe
) else (
    echo Compilation failed
)

pause
