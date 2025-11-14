/***********************************************************************
* Arduino config for XIAO ESP32S3:
*  + Board: ESP32S3 Dev Module
*  + USB CDC on Boot: Enabled
*  + Flash: 8MB
*  + Partition: 8MB with spiffs
*  + PSRAM: OPI PSRAM
***********************************************************************/


#include "gps.h"
#include "led.h"
#include "cli.h"
#include "mbfs.h"

#define GPS_PORT_TX 6
#define GPS_PORT_RX 5
#define GPS_PORT_BAUD 9600

#define USER_LED 21

#define CLED_R 7
#define CLED_G 9
#define CLED_B 8

NMEAGPS gps;
static LEDS commLeds;
static bool cliEnabled = false;

void setup() {
  pinMode(USER_LED, OUTPUT);
  Serial.begin(112500); // init USB serial

  if (!gps.begin(GPS_PORT_BAUD, GPS_PORT_RX, GPS_PORT_TX)) {
    Serial.printf("gps not init'd!\r\n");
  } 

  commLeds.begin(CLED_R, CLED_G, CLED_B);

  if (!initFS()) {
    Serial.printf("No filesystem!!\r\n");
  } else {
    Serial.printf("Filesystem open.\r\n");
  }

  uint32_t now = millis();
  Serial.printf("Press spacebar for interactive mode\r\n");
  while ((millis() - now) < 5000) {
    if (Serial.available()) {
      if (Serial.read() == ' ') {
        if (setupCLI()) {
          cliEnabled = true;
          break;
        }
      }
    }
  }

  if (!cliEnabled) {
    Serial.printf("Non-interactive mode\r\n");
  }
}

void loop() {
  static uint32_t msnow = millis();

  gps.update();
  commLeds.update();

  if (cliEnabled) {
    updateCLI();
  }

  if ((millis() - msnow) > 500) {
    msnow = millis();
    digitalWrite(USER_LED, !digitalRead(USER_LED));
  }
}
