#include "cfg.h"
#include "mbfs.h"

extern MBFS flockfs;

#define CONFIG_FILENAME "config.json"
const char* defaultWiFiAPs[] = {"flock", "flock safety", "something else"};

void CONFIG::begin()
{
  size_t cfgSize = flockfs.getFileSize(CONFIG_FILENAME);
  Serial.printf("CONFIG-> size of file %d bytes\r\n", cfgSize);
  if (!cfgSize)
  {
    this->buildDefualtConfig();
    Serial.print("CONFIG-> Built default config data\r\n");
  }
  else
  {
    char* json = (char*)ps_malloc(cfgSize);
    flockfs.readFile(CONFIG_FILENAME, (uint8_t*)json, cfgSize);
    deserializeJson(cfg, json);
    Serial.print("CONFIG-> Loaded config from file\r\n");
    delete (json);
  }
}

bool CONFIG::buildDefualtConfig()
{
  flockfs.deleteFile(CONFIG_FILENAME);

  cfg["flockType"] = "Generic ESP32S3";

  JsonArray wifiAPs = cfg["WifiAPs"].to<JsonArray>();

  size_t dfltAPCount = sizeof(defaultWiFiAPs) / sizeof(*defaultWiFiAPs);

  for(size_t ii = 0; ii < dfltAPCount; ++ii)
  {
    wifiAPs.add(defaultWiFiAPs[ii]);
  }

  char* cfgjson = (char*)ps_malloc(2048);
  if (!cfgjson)
  {
    return(false);
  }
  memset(cfgjson, 0, 2048);
  
  serializeJson(cfg, cfgjson, 2047);
  flockfs.writeFile(CONFIG_FILENAME, (uint8_t*)cfgjson, strlen(cfgjson));
  
  free(cfgjson);
  return (true);
}

void CONFIG::outputJson()
{
  serializeJsonPretty(cfg, Serial);
  Serial.printf("\r\n");
}