@echo off
echo Starting SkillUp Application...
echo.

REM Check if the executable exists
if not exist "out\build\vcpkg\App\Release\skillup.exe" (
    echo Error: skillup.exe not found. Please build the project first.
    echo Run: cmake --build out/build/vcpkg --config Release
    pause
    exit /b 1
)

REM Change to the executable directory and run
cd "out\build\vcpkg\App\Release"
skillup.exe

REM Return to original directory
cd "%~dp0"

echo.
echo Application closed.
pause 