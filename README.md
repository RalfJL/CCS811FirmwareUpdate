# CCS811 Firmware Updater
This sketch can be used to update the firmware of a CCS811 Gas Sensor.
See: https://ams.com/ccs811
It can only be used on a **ESP32** (propably on ESP8266 too) because it needs the SPI filesystem as storage for the firmware file.

## Usage
* Please use a ESP32 or ESP8266 board with the latest Arduino IDE.
* Make sure that you installed the SPIFFS plugin to the Arduino IDE (it is needed to download the filesystem to your ESP board)
* Download the latest CCS811 firmware from: https://ams.com/ccs811#tab/tools 
* Put the downloaded firmware file into the **data** subfolder of the sketch and rename it to "/CCS811_Firmware.bin"
* use the tools/data upload menu from the Arduino IDE to download the filesystem into your ESP board
* install the sketch CCS811FirmwareUpgrade into you ESP board and open a serial console
In the serial console:
* make sure, that your CCS811 is hooked up correctly by reading some sensor values
* verify you currents app firmware ("a"); you can't upgrade the boot firmware
* make sure with "L" that your fimrware file is pushed down to your ESP board and is readable
* start the app firmware upgrade with "u"

After the upgrade the CCS811 is restarted to load the app. You may have to powercycle your device to restart the CCS811 
Cehck the firmware again with "a"
