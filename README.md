# wifirecruitment
Wifi Recruitment via ESP-01/EPS8266 Boards.
Developed using [platform.io](https://platformio.org/). Before use, please double check your platformio.ini file to match your dev board (not all will probably work). This has been fieldtested on ESP-01 (1MB of SPIFFS) and EPS8266 (4MB of SPIFFS).

## /data
This folder contains the local files uploaded to SPIFFS. To store locally on the device without being flushed away on reset. 

## /data
This folder contains the local files uploaded to SPIFFS, non volatile. Has to be uploaded separately with the platformio commandline as define [here](https://docs.platformio.org/en/latest/platforms/espressif8266.html#uploading-files-to-file-system-spiffs)

## Settings
The SSID is stored in EEPROM, to be non volatile. This can be changed when browsing to the device IP (192.168.4.1)) when connected on the network and ammending /admin to the URL.

Upload functionality can be somewhat buggy and needs a forced reload to start uploading files. Somehow the whole logic behind grabbing files of a URL is reversed around to only look for files when it's not found. Naming files the same as existing overwrites them.

Reliably throws up the captive portal on android and iphone devices without problem. Works on most modern browsers too, but nothing is guaranteed as per license.

## Notes
Definitely needs some look through before use! Enjoy though, feel free to do a pull request with improvements.

