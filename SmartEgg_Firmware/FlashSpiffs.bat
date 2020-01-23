tools\mkspiffs.exe -c data -b 4096 -p 256 -s 0x233000 spiffs.bin
esptool.py --chip esp32 --port COM6 --baud 921600 write_flash -z 0x1CD000 spiffs.bin