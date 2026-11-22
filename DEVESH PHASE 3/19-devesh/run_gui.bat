@echo off
rem Starts the VitalRoute GUI on Windows by double-click.
rem Qt's DLLs come from MSYS2 (default C:\msys64); change QTBIN if MSYS2 is
rem installed elsewhere. Build the GUI first (README, "GUI").
setlocal
set "QTBIN=C:\msys64\ucrt64\bin"
cd /d "%~dp0"
if not exist "build-gui\vitalroute_gui.exe" (
    echo The GUI is not built yet. In the MSYS2 UCRT64 shell run:
    echo   cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
    echo   cmake --build build-gui
    pause
    exit /b 1
)
if not exist "%QTBIN%\Qt6Widgets.dll" (
    echo Qt 6 was not found in %QTBIN%. Edit QTBIN at the top of run_gui.bat.
    pause
    exit /b 1
)
set "PATH=%QTBIN%;%PATH%"
start "" "build-gui\vitalroute_gui.exe"
