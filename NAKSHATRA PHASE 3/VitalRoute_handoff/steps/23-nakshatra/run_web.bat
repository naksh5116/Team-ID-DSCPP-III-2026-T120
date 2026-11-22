@echo off
rem Starts VitalRoute Web on Windows by double-click: the hospital as a website
rem that phones, tablets and other computers on the same network can open.
rem   run_web.bat                 this computer and the local network
rem   run_web.bat --local-only    this computer only
rem The C++ runtime DLLs come from MSYS2 (default C:\msys64); change RTBIN if
rem MSYS2 is installed elsewhere. Build first: make web (README, "Web version").
setlocal
set "RTBIN=C:\msys64\ucrt64\bin"
cd /d "%~dp0"
if not exist "bin\vitalroute_web.exe" (
    echo VitalRoute Web is not built yet. In the MSYS2 UCRT64 shell run:
    echo   make web
    pause
    exit /b 1
)
if not exist "%RTBIN%\libstdc++-6.dll" (
    echo The MSYS2 C++ runtime was not found in %RTBIN%. Edit RTBIN at the top of run_web.bat.
    pause
    exit /b 1
)
set "PATH=%RTBIN%;%PATH%"
rem Open the Launcher in the browser once the server has had a moment to start.
start "" /b cmd /c "timeout /t 2 /nobreak >nul & start "" http://localhost:8080/"
echo Close this window, or press Ctrl+C, to stop VitalRoute Web.
echo.
"bin\vitalroute_web.exe" --port 8080 %*
pause