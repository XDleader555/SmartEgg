@echo off
IF [%1]==[-p] (IF [%2] == [] GOTO :err ELSE GOTO :flash) ELSE GOTO :err

:flash
tools\mkspiffs.exe -c data -b 4096 -p 256 -s 0x233000 spiffs.bin
esptool.py --chip esp32 --port %2 --baud 921600 write_flash -z 0x1CD000 spiffs.bin
exit /b

:err
@echo Please specify port with -p
exit /b