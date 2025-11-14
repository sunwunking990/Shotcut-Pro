@echo off
echo === ShotcutCPP Build and Run Script ===
echo.

REM Check for Visual Studio 2022
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

echo Visual Studio 2022 not found
goto :end

:compile
echo.
echo === Configuring CMake ===
if not exist build mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed
    cd ..
    goto :end
)

echo.
echo === Building Project ===
cmake --build . --config Release --parallel 4

if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    cd ..
    goto :end
)

echo.
echo === Build Successful! ===
echo.
echo === Running Application ===
cd Release
ShotcutCPP.exe

:end
cd ..
pause
