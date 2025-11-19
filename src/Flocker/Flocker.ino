/***********************************************************************
* Arduino config for XIAO ESP32S3:
*  + Board: ESP32S3 Dev Module
*  + USB CDC on Boot: Enabled
*  + Flash: 8MB
*  + Partition: 8MB with spiffs
*  + PSRAM: OPI PSRAM
***********************************************************************/
#include <esp_psram.h>

#include "gps.h"
#include "led.h"
#include "cli.h"
#include "mbfs.h"
#include "cfg.h"
#include "scanner.h"

#define GPS_PORT_TX 6
#define GPS_PORT_RX 5
#define GPS_PORT_BAUD 9600

#define USER_LED 21

#define CLED_R 7
#define CLED_G 9
#define CLED_B 8

// globals
NMEAGPS gps;
MBFS flockfs;
CONFIG flockCfg;
SCANNER flockScan;

static LEDS commLeds;

/*
void heapCheck() {
  if (!heap_caps_check_integrity_all(true)) {
    Serial.printf("HEAPCHK->HEAPS FUCKED!\r\n");
  }
}
*/

void setup() {
  psramInit();
  pinMode(USER_LED, OUTPUT);
  Serial.begin(112500); // init USB serial
  delay(500);

  Serial.printf(CLI_CLEAR);

  if (!gps.begin(GPS_PORT_BAUD, GPS_PORT_RX, GPS_PORT_TX)) {
    Serial.printf("gps not init'd!\r\n");
  } 

  commLeds.begin(CLED_R, CLED_G, CLED_B);

  flockfs.begin();
  flockCfg.begin();
  flockScan.begin();
  setupCLI();
}

void loop() {
  static uint32_t msFlash = millis();

  gps.update();
  commLeds.update();
  updateCLI();

  if ((millis() - msFlash) > 500) {
    msFlash = millis();
    digitalWrite(USER_LED, !digitalRead(USER_LED));
  }
}
