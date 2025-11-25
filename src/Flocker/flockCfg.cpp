#include <Arduino.h>

#include "flockCfg.h"

#include "globals.h"

// JSON keys for config set
#define FLOCKTYPE "flockType"
#define TIMEZONE "timeZone"
#define WIFIAPS "WifiAPs"

// name of configuration file
#define CONFIG_FILENAME "config.json"
#define CONFIG_BACKUP_FILENAME "config.bak"

// default WiFi names corresponding to know surveillance devices.  This
// list is the initial - more can be added later by the user
const char* defaultWiFiAPs[] = {"flock", "fs ext Battery", "penguin", "pigvision"};

// Struct for following list of timezones
struct cfg_tz_t
{
    uint8_t inx;
    const char* desc;
    const char* tz;
};

// List of timezones to select from.  This is a very US-centric
// list, with default "offset from GMT" zones included
const cfg_tz_t zones[] = {{0, "default CSD/CDT", "CST6CDT,M3.2.0,M11.1.0"},
                          {1, "America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
                          {2, "America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
                          {3, "America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
                          {4, "America/Detroit", "EST5EDT,M3.2.0,M11.1.0"},
                          {5, "America/Indiana/Indianapolis", "EST5EDT,M3.2.0,M11.1.0"},
                          {6, "America/Indiana/Knox", "CST6CDT,M3.2.0,M11.1.0"},
                          {7, "America/Juneau", "AKST9AKDT,M3.2.0,M11.1.0"},
                          {8, "America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
                          {9, "America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
                          {10, "America/Phoenix", "MST7"},
                          {11, "Pacific/Honolulu", "HST10"},
                          {12, "Etc/Greenwich", "GMT0"},
                          {13, "Etc/GMT", "GMT0"},
                          {14, "Etc/GMT-0", "GMT0"},
                          {15, "Etc/GMT-1", "<+01>-1"},
                          {16, "Etc/GMT-2", "<+02>-2"},
                          {17, "Etc/GMT-3", "<+03>-3"},
                          {18, "Etc/GMT-4", "<+04>-4"},
                          {19, "Etc/GMT-5", "<+05>-5"},
                          {20, "Etc/GMT-6", "<+06>-6"},
                          {21, "Etc/GMT-7", "<+07>-7"},
                          {22, "Etc/GMT-8", "<+08>-8"},
                          {23, "Etc/GMT-9", "<+09>-9"},
                          {24, "Etc/GMT-10" ,"<+10>-10"},
                          {25, "Etc/GMT-11" ,"<+11>-11"},
                          {26, "Etc/GMT-12" ,"<+12>-12"},
                          {27, "Etc/GMT-13" ,"<+13>-13"},
                          {28, "Etc/GMT-14" ,"<+14>-14"},
                          {29, "Etc/GMT0"," GMT0"},
                          {30, "Etc/GMT+0", "GMT0"},
                          {31, "Etc/GMT+1", "<-01>1"},
                          {32, "Etc/GMT+2", "<-02>2"},
                          {33, "Etc/GMT+3", "<-03>3"},
                          {34, "Etc/GMT+4", "<-04>4"},
                          {35, "Etc/GMT+5", "<-05>5"},
                          {36, "Etc/GMT+6", "<-06>6"},
                          {37, "Etc/GMT+7", "<-07>7"},
                          {38, "Etc/GMT+8", "<-08>8"},
                          {39, "Etc/GMT+9", "<-09>9"},
                          {40, "Etc/GMT+10", "<-10>10"},
                          {41, "Etc/GMT+11", "<-11>11"},
                          {42, "Etc/GMT+12", "<-12>12"}};

const size_t tzCount = sizeof(zones) / sizeof(cfg_tz_t);


bool CONFIG::begin()
{
  if (!flockfs.fileExists(CONFIG_FILENAME))
  {
    Serial.print(CLI_BOLD_RED "CONFIG::begin()-> Built default config data\r\n" CLI_RESET);
    return (this->buildDefualtConfig());
  }
  else
  {
    size_t cfgSize = flockfs.getFileSize(CONFIG_FILENAME);

    char* json = (char*)ps_malloc(cfgSize);
    if (!json)
    {
      Serial.print(CLI_BOLD_RED "CONFIG::begin()-> Did not allocate to build default config data\r\n" CLI_RESET);
      return (false);
    }
    
    if (flockfs.readFile(CONFIG_FILENAME, (uint8_t*)json, cfgSize) == -1)
    {
      Serial.printf(CLI_BOLD_RED "CONFIG::begin()-> Error reading config file\r\n" CLI_RESET);
      return (false);
    }

    deserializeJson(cfg, json);
    delete (json);
  }

  return(true);
}

bool CONFIG::buildDefualtConfig()
{
  flockfs.deleteFile(CONFIG_FILENAME);

  cfg[FLOCKTYPE] = "Generic ESP32S3";         // tag that can be added to data
  cfg[TIMEZONE] = "CST6CDT,M3.2.0,M11.1.0";   // my timezone :)

  JsonArray wifiAPs = cfg[WIFIAPS].to<JsonArray>();

  size_t dfltAPCount = sizeof(defaultWiFiAPs) / sizeof(*defaultWiFiAPs);

  for(size_t ii = 0; ii < dfltAPCount; ++ii)
  {
    wifiAPs.add(defaultWiFiAPs[ii]);
  }

  return (this->writeConfig());
}

// Pretty-print JSON configuration
void CONFIG::outputJson()
{
  Serial.printf(CLI_GRN);
  serializeJsonPretty(cfg, Serial);
  Serial.printf("\r\n" CLI_RESET);
}

// Start a menu-driven time zone selection chingus
void CONFIG::selectTimeZone()
{
  // print all of the available timezones in two columns
  for (size_t ii = 1; ii < tzCount / 2; ++ii)
  {
    Serial.printf(CLI_YEL " %2d)" CLI_BOLD_GRN " %-30s " CLI_YEL " %2d) " CLI_BOLD_GRN "%s\r\n" 
        CLI_RESET, ii, zones[ii].desc, ii + 21, zones[ii + 21].desc);
  }

  holdCLI(true);  // steal serial input from CLI handler
  Serial.printf("Select timezone by number: ");

  char tzInput[14] = {0};
  uint8_t posn = 0;

  while (posn < 13)
  {
    while (!Serial.available());
    char c = Serial.read();
    if (c == '\r')
    {
      break;
    }
    else if (c == '\b')
    {
      if (posn)
      {
        --posn;
        Serial.printf(CLI_BACKSPACE);
        Serial.printf(CLI_DELETE);
      }
    }
    else
    {
      tzInput[posn] = c;
      Serial.print(c);
      ++posn;
    }
  }

  tzInput[posn] = 0;
  int selected = atoi(tzInput);
  if (selected > tzCount) selected = 0;
  Serial.printf("\r\n");
  
  holdCLI(false);

  cfg[TIMEZONE] = zones[selected].tz;
  this->setTimeZone();
  this->writeConfig();
}

void CONFIG::setTimeZone()
{
  const char* tz = cfg[TIMEZONE];
  setenv("TZ", tz, 1);
  tzset();  
}

const char* CONFIG::getTimeZone()
{
  return (cfg[TIMEZONE]);
}

bool CONFIG::writeConfig()
{
  bool retVal = false;
  char* cfgjson = (char*)ps_malloc(2048);

  if (!cfgjson)
  {
    Serial.printf(CLI_BOLD_RED "CONFIG::writeConfig()->DID NOT ALLOCATE WRITE BUFFER" CLI_RESET);
    return(retVal);
  }

  flockfs.renameFile(CONFIG_FILENAME, CONFIG_BACKUP_FILENAME);

  memset(cfgjson, 0, 2048);
  serializeJson(cfg, cfgjson, 2047);
  
  if (flockfs.writeFile(CONFIG_FILENAME, (uint8_t*)cfgjson, strlen(cfgjson)) == -1)
  {
    Serial.printf(CLI_BOLD_RED "CONFIG::writeConfig()-> Error writing config file\r\n" CLI_RESET);
  }
  else
  {
    retVal = true;
  }
  
  free(cfgjson);
  return (retVal);
}
