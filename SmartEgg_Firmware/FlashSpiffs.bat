tools\mkspiffs.exe -c data -b 4096 -p 256 -s 0x233000 spiffs.bin
esptool --chip esp32 --port COM8 --baud 921600 write_flash -z 0x1CD000 spiffs.bin