# CCS811 Firmware Updater
This sketch can be used to update the firmware of a CCS811 Gas Sensor.
See: https://ams.com/ccs811
It can only be used on a **ESP32** (probably on ESP8266 too) because it needs the SPI file system as storage for the firmware file.

## Usage
* Please use a ESP32 or ESP8266 board with the latest Arduino IDE.
* Make sure that you installed the SPIFFS plug in to the Arduino IDE (it is needed to download the file system to your ESP board)
* Download the sketch and open it in the Arduino IDE. This will create a sketch directory
* Download the latest CCS811 firmware from: https://ams.com/ccs811#tab/tools 
* Put the downloaded firmware file into the **data** sub folder (you will have to create that folder first) of the sketch directory and rename the firmware file to "/CCS811_Firmware.bin". Please read the SPIFFS documentation
* use the "**tools/data upload**" menu from the Arduino IDE to download the file system into your ESP board
* install the sketch CCS811FirmwareUpgrade into you ESP board and open a serial console

In the serial console:
* make sure, that your CCS811 is hooked up correctly by reading some sensor values
* verify your current app firmware ("a"); you can't upgrade the boot firmware
* make sure with "L" that your firmware file is pushed down to your ESP board and is readable
* start the app firmware upgrade with "u"

After the upgrade the CCS811 is restarted to load the app. You may have to power cycle your device to restart the CCS811 
Check the firmware version again with "a"

After a few days with the firmware 2.0.0, I am still very unhappy with the CCS811 sensor. Have a look at the chart below:
[https://github.com/RalfJL/CCS811FirmwareUpdate/CCS811-goes-mad.png]

The sensor is in a room where I do not use any sprays or something that emits VOC's. The windows is partially open almost all day. Actually there is only one human (me) that emits VOC's and a computer.
A second CCS811 sensor is 20 centimeters away from the first one in a different housing with a 6 °C higher temperature (due to the heat of the ESP32).
* after one or two days of continuous operation the values for eCO2 and TVOC suddenly shows peaks and are continuously rising
* at the point where TVOC was showing 7200 the other sensor 20 cm away was still reporting TVOC's lower than 200
* so I decided to power cycle the sensor and it was going down to almost 0 for TVOC
* after 30 minutes the "baseline" stored in the EEPROM of the ESP32 was restored to the sensor. 
* Both values spiked up again to fall after around 20 minutes to a low value. Close to what the second sensor shows 
* but the values are climbing again and stay at a high level, that the second sensor is not showing

Someone else having this experience?
