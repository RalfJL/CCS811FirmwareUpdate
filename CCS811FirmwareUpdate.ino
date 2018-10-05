/******************************************************************************
  CCS811 firmware updater
  FirmwareUpdate.ino
  Oct. 2018 Ralf Lehmann
  Modified example from SparkFun to enable CCS811 app firmware update and verification
  All credits go to SparkFun (see below)

  Update is only working on ESP32 and propably on ESP8266.
  You will need
  - SPI Filesystem SPIFFS (available for ESP32 and ESP8266)
  - download the current CCS811 (App)firmware from https://ams.com/ccs811#tab/tools and put it as file "/CCS811_Firmware.bin" in the SPIFFS
  - install SPIFFS on your device
  - install the sketch and start a serial console
  - check that sensor is correctly attached by reading some sensor values
  - check currently installed app firmware; you can't change the boot firmware
  - check that the firmware file is readable "L"
  - start update procedure; app is verified and started afterwards


  BaselineOperator.ino

  Marshall Taylor @ SparkFun Electronics

  April 4, 2017

  https://github.com/sparkfun/CCS811_Air_Quality_Breakout
  https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library

  This example demonstrates usage of the baseline register.

  To use, wait until the sensor is burned in, warmed up, and in clean air.  Then,
  use the terminal to save the baseline to EEPROM.  Aftewards, the sensor can be
  powered up in dirty air and the baseline can be restored to the CCS811 to help
  the sensor stablize faster.

  EEPROM memory usage:

  addr: data
  ----------
  0x00: 0xA5
  0x01: 0xB2
  0x02: 0xnn
  0x03: 0xmm

  0xA5B2 is written as an indicator that 0x02 and 0x03 contain a valid number.
  0xnnmm is the saved data.

  The first time used, there will be no saved data

  Hardware Connections (Breakoutboard to Arduino):
  3.3V to 3.3V pin
  GND to GND pin
  SDA to A4
  SCL to A5
  Hardware Connections (Breakoutboard to ESP32):
  3.3V to 3.3V pin
  GND to GND pin
  SDA to 21
  SCL to 22

  Resources:
  Uses Wire.h for i2c operation
  Uses EEPROM.h for internal EEPROM driving

  Development environment specifics:
  Arduino IDE 1.8.1

  This code is released under the [MIT License](http://opensource.org/licenses/MIT).

  Please review the LICENSE.md file included with this example. If you have any questions
  or concerns with licensing, please contact techsupport@sparkfun.com.

  Distributed as-is; no warranty is given.
******************************************************************************/
#include <SparkFunCCS811.h>
#include <EEPROM.h>

#include <FS.h>
#include <SPIFFS.h>

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  int i = 0;
  while (file.available()) {
    Serial.print(file.read(), HEX);
    if (!( ++i % 20)) {
      Serial.println("");
    }
  }
}


//#define CCS811_ADDR 0x5B //Default I2C Address
#define CCS811_ADDR 0x5A //Alternate I2C Address

CCS811 mySensor(CCS811_ADDR);
#define FIRMWAREFILE "/CCS811_Firmware.bin"
#include "Wire.h"


/*
   Firmware update
   according to AN000371
*/
uint8_t CCS811_SW_RESET [] = { 0x11, 0xE5, 0x72, 0x8A };
uint8_t CCS811_ERASE [] = { 0xE7, 0xA7, 0xE6, 0x09 };
#define CCS811_APP_ERASE 0xF1
#define CCS811_REG_BOOT_APP 0xF2
#define CCS811_VERIFY 0xF3
void update_firmware()
{
  Serial.println("Updating firmware");
  uint8_t status_register;
  // switch to boot mode
  if ( mySensor.readRegister(CSS811_STATUS, &status_register) != CCS811Core::SENSOR_SUCCESS) {
    Serial.println("Unable to read status register");
    return;
  }
  if ( status_register & 0x80 ) {
    Serial.println("Sensor is in application mode");
    Serial.print("Switching to boot mode...");
    if ( mySensor.multiWriteRegister(CSS811_SW_RESET, CCS811_SW_RESET, 4) != CCS811Core::SENSOR_SUCCESS) {
      Serial.println("failed to write to sw register");
      return;
    }
    delay(500); // wait for sensor to setle
    if ( mySensor.readRegister(CSS811_STATUS, &status_register) != CCS811Core::SENSOR_SUCCESS) {
      Serial.println("Unable to read status register");
      return;
    }
    if ( status_register & 0x80 ) {
      Serial.println("Failed to switch to boot mode");
      return;
    } else {
      Serial.println("done");
    }
  }
  // open firmware file
  File fwfile = SPIFFS.open(FIRMWAREFILE);
  if ( !fwfile || fwfile.isDirectory()) {
    Serial.print("Failed to open firmware file: "); Serial.println(FIRMWAREFILE);
    return;
  }
  // erase old app
  if ( mySensor.multiWriteRegister(CCS811_APP_ERASE, CCS811_ERASE, 4) != CCS811Core::SENSOR_SUCCESS) {
    Serial.println("failed erase app");
    return;
  }
  delay(500); // wait for app to be erased
  // now write new app to device
  size_t c;
  uint8_t line[8];
  while ( c = fwfile.read(line, 8)) {
    Serial.print(c); Serial.print(": ");
    for (int i = 0; i < 8 ; i++) {
      Serial.print(line[i], HEX); Serial.print(" ");
    }
    if ( mySensor.multiWriteRegister(CCS811_REG_BOOT_APP, line, 8) != CCS811Core::SENSOR_SUCCESS) {
      Serial.println("failed write line");
    } else {
      Serial.println(" ok");
    }
    delay(100); // not too fast
  }
  // verify application
  if ( verify_app_firmware()) {
    Serial.println("App is good");
  } else {
    Serial.println("App is bad");
    Serial.println("Failed to update firmware");
    return;
  }
  Serial.println("Starting app");
  if ( start_app() ) {
    Serial.println("Ready to go");
  }
}

bool start_app() {
  CCS811Core::status returnCode;
  uint8_t status_register;
  if ( (returnCode = mySensor.readRegister(CSS811_STATUS, &status_register)) != CCS811Core::SENSOR_SUCCESS) {
    Serial.println("Unable to read status register");
    printDriverError( returnCode );
    return false;
  }
  if ( status_register & 0x80 ) {
    Serial.println("App is already started");
    return true;
  }
  returnCode = mySensor.begin();
  Serial.print("begin exited with: ");
  printDriverError( returnCode );
  Serial.println();
  if ( returnCode != CCS811Core::SENSOR_SUCCESS) {
    return false;
  }
  return true;
}

bool verify_app_firmware()
{
  // verify application
  uint8_t status_register;
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_VERIFY);
  delay(500); // scanning
  if ( mySensor.readRegister(CSS811_STATUS, &status_register) != CCS811Core::SENSOR_SUCCESS) {
    Serial.println("Unable to read status register");
    return false;
  }
  if ( status_register & 0x10) {
    return true;
  } else {
    Serial.print("Status register: "); Serial.println(status_register, BIN);
  }
}


#define CCS811_FW_BOOT_VERSION 0x23
bool getBootFirmware(CCS811 sens, uint8_t firm[3]) {
  uint8_t data[2];
  CCS811Core::status returnError = sens.multiReadRegister(CCS811_FW_BOOT_VERSION, data, 2);
  if ( returnError != CCS811Core::SENSOR_SUCCESS) {
    return false;
  }
  firm[0] = (data[0] >> 4) & 0x0f;
  firm[1] = data[0] & 0x0f;
  firm[2] = data[1];
  return true;
}

#define CCS811_FW_APP_VERSION 0x24
bool getAppFirmware(CCS811 sens, uint8_t firm[3]) {
  uint8_t data[2];
  CCS811Core::status returnError = sens.multiReadRegister(CCS811_FW_APP_VERSION, data, 2);
  if ( returnError != CCS811Core::SENSOR_SUCCESS) {
    return false;
  }
  firm[0] = (data[0] >> 4) & 0x0f;
  firm[1] = data[0] & 0x0f;
  firm[2] = data[1];
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
  }
  Serial.println();
  Serial.println("CCS811 Baseline Example");
  //Wire.setClock(100000L);

  CCS811Core::status returnCode = mySensor.begin();
  Serial.print("begin exited with: ");
  printDriverError( returnCode );
  Serial.println();

  //This looks for previously saved data in the eeprom at program start
  if ((EEPROM.read(0) == 0xA5) && (EEPROM.read(1) == 0xB2))
  {
    Serial.println("EEPROM contains saved data.");
  }
  else
  {
    Serial.println("Saved data not found!");
  }
  print_menu();
}

void print_menu() {
  Serial.println();

  Serial.println("Program running.  Send the following characters to operate:");
  Serial.println(" 's' - save baseline into EEPROM");
  Serial.println(" 'l' - load and apply baseline from EEPROM");
  Serial.println(" 'c' - clear baseline from EEPROM");
  Serial.println(" 'r' - read and print sensor data");
  Serial.println(" 'b' - print boot firmware version");
  Serial.println(" 'a' - print app firmware version");
  Serial.println(" 'v' - verify app firmware");
  Serial.println(" 'S' - start app firmware");
  Serial.println(" 'L' - list SPIFFS directory");
  Serial.println(" 'u' - update app firmware");
}



void loop()
{
  char c;
  unsigned int result;
  unsigned int baselineToApply;
  CCS811Core::status errorStatus;
  if (Serial.available())
  {
    c = Serial.read();
    switch (c)
    {
      case 's':
        //This gets the latest baseline from the sensor
        result = mySensor.getBaseline();
        Serial.print("baseline for this sensor: 0x");
        if (result < 0x100) Serial.print("0");
        if (result < 0x10) Serial.print("0");
        Serial.println(result, HEX);
        //The baseline is saved (with valid data indicator bytes)
        EEPROM.write(0, 0xA5);
        EEPROM.write(1, 0xB2);
        EEPROM.write(2, (result >> 8) & 0x00FF);
        EEPROM.write(3, result & 0x00FF);
        break;
      case 'l':
        if ((EEPROM.read(0) == 0xA5) && (EEPROM.read(1) == 0xB2))
        {
          Serial.println("EEPROM contains saved data.");
          //The recovered baseline is packed into a 16 bit word
          baselineToApply = ((unsigned int)EEPROM.read(2) << 8) | EEPROM.read(3);
          Serial.print("Saved baseline: 0x");
          if (baselineToApply < 0x100) Serial.print("0");
          if (baselineToApply < 0x10) Serial.print("0");
          Serial.println(baselineToApply, HEX);
          //This programs the baseline into the sensor and monitors error states
          errorStatus = mySensor.setBaseline( baselineToApply );
          if ( errorStatus == CCS811Core::SENSOR_SUCCESS )
          {
            Serial.println("Baseline written to CCS811.");
          }
          else
          {
            printDriverError( errorStatus );
          }
        }
        else
        {
          Serial.println("Saved data not found!");
        }
        break;
      case 'c':
        //Clear data indicator and data from the eeprom
        Serial.println("Clearing EEPROM space.");
        EEPROM.write(0, 0x00);
        EEPROM.write(1, 0x00);
        EEPROM.write(2, 0x00);
        EEPROM.write(3, 0x00);
        break;
      case 'r':
        if (mySensor.dataAvailable())
        {
          //Simply print the last sensor data
          mySensor.readAlgorithmResults();

          Serial.print("CO2[");
          Serial.print(mySensor.getCO2());
          Serial.print("] tVOC[");
          Serial.print(mySensor.getTVOC());
          Serial.print("]");
          Serial.println();
        }
        else
        {
          Serial.println("Sensor data not available.");
        }
        break;
      case 'b':
        uint8_t bootfirmware[3];
        if (getBootFirmware(mySensor, bootfirmware)) {
          Serial.print("Firmware: "); Serial.print(bootfirmware[0]); Serial.print("."); Serial.print(bootfirmware[1]); Serial.print("."); Serial.println(bootfirmware[2]);
        }
        break;
      case 'a':
        uint8_t appfirmware[3];
        if (getAppFirmware(mySensor, appfirmware)) {
          Serial.print("App Firmware: "); Serial.print(appfirmware[0]); Serial.print("."); Serial.print(appfirmware[1]); Serial.print("."); Serial.println(appfirmware[2]);
        }
        break;
      case 'L':
        listDir(SPIFFS, "/", 0);
        readFile(SPIFFS, FIRMWAREFILE);
        print_menu();
        break;
      case 'u':
        update_firmware();
        print_menu();
        break;
      case 'S':
        start_app();
        print_menu();
        break;
      case 'v':
        if ( verify_app_firmware()) {
          Serial.println("App Firmware ok");
        } else {
          Serial.println("App Firmware broken");
        }
      case '\n':
      case '\r':
        break;
      default:
        //        Serial.println(c, HEX);
        //        Serial.println(c);
        print_menu();
        break;
    }
  }
  delay(10);
}

//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void printDriverError( CCS811Core::status errorCode )
{
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}
