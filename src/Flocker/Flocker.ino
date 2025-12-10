/***********************************************************************
* Arduino config for XIAO ESP32S3:
*  + Board: ESP32S3 Dev Module
*  + USB CDC on Boot: Enabled
*  + Flash: 8MB
*  + Partition: custom
*  + PSRAM: OPI PSRAM
***********************************************************************/
//#include <esp_psram.h>

#include "gps.h"
#include "led.h"
#include "cli.h"
#include "flockFs.h"
#include "flockCfg.h"
#include "scanner.h"
#include "flockLog.h"

// constants for config'ing objects
#define GPS_PORT_TX 5
#define GPS_PORT_RX 6
#define GPS_PORT_BAUD 9600
#define ADDR_LED_PIN 9
#define USER_LED 21

// globals
FLOGGER flockLog;
NMEAGPS gps;
MBFS flockfs;
CONFIG flockCfg;
SCANNER flockScan;
LEDS flockLED;
bool psRamInitOk;
bool initOk;

static uint8_t cfgListenerID = BAD_LISTENER_ID;

void heapCheck() {
  if (!heap_caps_check_integrity_all(true)) {
    Serial.printf("HEAPCHK->HEAPS FUCKED!\r\n");
  }
}

void setup() {
  initOk = true;
  psRamInitOk = psramInit();
  pinMode(USER_LED, OUTPUT);
  Serial.begin(112500);  // init USB serial
  delay(2000);

  // order is important here
  initOk &= gps.begin(GPS_PORT_BAUD, GPS_PORT_RX, GPS_PORT_TX);
  initOk &= flockfs.begin();
  initOk &= flockCfg.begin();
  initOk &= flockLog.begin(200);
  initOk &= flockScan.begin();
  initOk &= flockLED.begin(ADDR_LED_PIN, 140);
  initOk &= setupCLI();

  if (!initOk)
  {
    delay(2000);
    Serial.printf("SOMETHNG DIDN'T INIT.  Fuck :/\r\n");
    delay(2000);
  }
  else
  {
    flockCfg.registerListener(cfgListenerID);
  }

  flockLog.addLogLine("main", "Leaving setup()\r\n");
}

void loop() {
  static uint32_t msFlash = millis();
  static uint32_t msHeap = millis() - 1000;

  if (initOk)
  {
    gps.update();
    flockLED.update();
    updateCLI();
    flockLog.update();
  }

  if ((millis() - msFlash) > 500) {
    msFlash = millis();
    digitalWrite(USER_LED, !digitalRead(USER_LED));
  }

  if ((millis() - msHeap) > 2000) {
    msHeap = millis();
    heapCheck();
  }
}
