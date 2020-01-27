@echo off
IF [%1]==[-p] (IF [%2] == [] GOTO :err ELSE GOTO :flash) ELSE GOTO :err

:flash
idf.py -p %2 -b 921600 flash
esptool --chip esp32 --port %2 --baud 921600 write_flash -z 0x1CD000 spiffs.bin
exit /b

:err
@echo Please specify port with -p
exit /b