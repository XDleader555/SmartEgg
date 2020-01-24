# SmartEgg
Data collection firmware for an ESP32 based egg drop.

## Specifications
MCU: [Adafruit HUZZAH32](https://www.adafruit.com/product/3405)

Weight: 76.25 grams

Material: PET-G

Width: 53.10 mm (2.09 in)

Height: 70.53 mm (2.78 in)

<img src="https://github.com/XDleader555/SmartEgg/raw/master/Resources/SmartEgg_Assembly.jpg" width="256">

## Prerequisites
This project needs to be compiled using the latest esp-idf which supports the ardino-esp32 core.

### esptool
Make sure that python is added to your path and run
```
pip install esptool
```

### mkspiffs
To install mkspiffs, add the following entry to your esp-idf/tools/tools.json
```
    {
      "description": "mkspiffs",
      "export_paths": [
        [
          "mkspiffs-0.2.3-esp-idf-win32"
        ]
      ],
      "export_vars": {},
      "info_url": "https://github.com/igrr/mkspiffs",
      "install": "never",
      "license": "MIT",
      "name": "mkspiffs",
      "platform_overrides": [
        {
          "install": "always",
          "platforms": [
            "win64"
          ]
        }
      ],
      "version_cmd": [
        "mkspiffs.exe",
        "--version"
      ],
      "version_regex": "mkspiffs ver. ([0-9.]+)",
      "versions": [
        {
          "name": "0.2.3",
          "status": "recommended",
          "win64": {
            "sha256": "deaf940e58da4d51337b1df071c38647f9e1ea35bcf14ce7004e246175f37c9e",
            "size": 249820,
            "url": "https://github.com/igrr/mkspiffs/releases/download/0.2.3/mkspiffs-0.2.3-esp-idf-win32.zip"
          }
        }
      ]
    }
```
