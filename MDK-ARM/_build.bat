@echo off
cd /d "E:\jinjiangxueyuan\Stm32Project\ColdChainMonitor\MDK-ARM"
"D:\Program Files\keil\UV4\UV4.exe" -r "ColdChainMonitor.uvprojx" -j0
echo BUILD_EXIT_CODE=%errorlevel%
