@echo off
cd /d "E:\jinjiangxueyuan\Stm32Project\ColdChainMonitor\MDK-ARM"
"D:\Program Files\keil\UV4\UV4.exe" -r "ColdChainMonitor.uvprojx" -j0
set UV_EXIT=%errorlevel%
findstr /C:"0 Error(s)" "LED\LED.build_log.htm" >nul 2>nul
if %errorlevel% equ 0 (
    echo BUILD_EXIT_CODE=0
    exit /b 0
)
echo BUILD_EXIT_CODE=%UV_EXIT%
exit /b %UV_EXIT%