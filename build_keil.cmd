@echo off
REM ===== Build ColdChainMonitor with Keil MDK =====
set UV4="D:\Program Files\keil\UV4\UV4.exe"
set PROJ=..\MDK-ARM\ColdChainMonitor.uvprojx

if not exist %UV4% (
    echo Keil UV4 not found at %UV4%
    echo Please update the path in this script
    pause
    exit /b 1
)

echo Building ColdChainMonitor...
%UV4% -r %PROJ% -j0 -o build.log

if %errorlevel% equ 0 (
    echo Build successful!
) else (
    echo Build failed! Check build.log for details.
    type build.log
)
pause
