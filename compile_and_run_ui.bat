@echo off
echo === Shotcut UI Framework Demo Compiler ===
echo.

REM Try to find and use available C++ compiler
where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found g++ compiler
    g++ -std=c++20 -O2 simple_ui_showcase.cpp -o ui_demo.exe
    if exist ui_demo.exe (
        echo Compilation successful! Running demo...
        echo.
        ui_demo.exe
    ) else (
        echo Compilation failed
    )
    goto :end
)

where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found MSVC compiler
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\VCVARSALL.BAT"
    cl /EHsc /O2 simple_ui_showcase.cpp /Fe:ui_demo.exe
    if exist ui_demo.exe (
        echo Compilation successful! Running demo...
        echo.
        ui_demo.exe
    ) else (
        echo Compilation failed
    )
    goto :end
)

echo No C++ compiler found
echo Please install g++ or Visual Studio with C++ compiler
echo.

:end
pause
