#include <map>

#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"

#include "scanner.h"
#include "cli.h"
#include "cfg.h"
#include "gps.h"
#include "mbfs.h"
#include "md5.h"

extern CONFIG flockCfg;
extern MBFS flockfs;
extern CONFIG flockCfg;


typedef struct
{
  uint8_t channel;
  char ssid[33];
  uint8_t mac[6];
  int8_t rssi;
} found_beacon_t;

static const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                   36, 40, 44, 48, 149, 153, 157, 161, 165};

const size_t channelCount = sizeof(channels) / sizeof(channels[0]);
static uint16_t channelInx = 0;     
static bool once = false;
static bool scanning = false;
static md5_context ctx;
static uint8_t md5sum[16];
static std::map<uint32_t, found_beacon_t> beacons;

const char* HexDump(const void* data, size_t size) 
{
  static char ret[2048] = {0};
  char line[120];
  char adder[8];
  char prnt[40];
  const char* d = (const char*)data;

  size_t lineLen = 119;
  size_t prntLen = 39;

  const uint8_t* thisChar = (uint8_t*)data;
  size_t posn = 0;

  for (size_t ii = 0; ii < size; ++ii)
  {
    if (!posn)
    {
      snprintf(line, lineLen, "%08x  ", ii);
      snprintf(prnt, prntLen, " |");
    }

    snprintf(adder, 7, "%02x ", (int)*d);
    strncat(line, adder, (lineLen - strlen(line)));

    if (isprint(*d))  snprintf(adder, 7, "%c", *d);
    else              snprintf(adder, 7, ".");
    strncat(prnt, adder, (prntLen - strlen(prnt)));
    
    ++posn;
    ++d;

    if (posn == 8)
    {
      strncat(line, " ", (lineLen - strlen(line)));
      strncat(prnt, " ", (prntLen - strlen(prnt)));
    }

    if (posn == 16)
    {
      posn = 0;
      strncat(prnt, "|", (prntLen - strlen(prnt)));
      strncat(line, prnt, (lineLen - strlen(line)));
      strncat(ret, line, (2047 - strlen(ret)));
      strncat(ret, "\r\n", (2047 - strlen(ret)));
    }
  }

  if (posn)
  {
    strncat(line, " ", (lineLen - strlen(line)));
    size_t spacer = 16 - posn;
    for (size_t ii = 0; ii < spacer; ++ii)
    {
      strncat(line, "   ", (lineLen - strlen(line)));
      strncat(prnt, " ", (prntLen - strlen(prnt)));
    }
    if (spacer < 8)
    {
      strncat(line, " ", (lineLen - strlen(line)));
    }

    strncat(prnt, "|", (prntLen - strlen(prnt)));
    strncat(line, prnt, (lineLen - strlen(line)));
    strncat(ret, line, (2047 - strlen(ret)));
    strncat(ret, "\r\n", (2047 - strlen(ret)));
  }

  return (ret);
}


typedef struct __attribute__((packed))
{
  uint8_t element_id;   // element ID
  uint8_t length;       // length of element
  uint8_t data[0];      // element data
} wifi_ieee80211_beacon_element_t;

typedef struct __attribute__((packed))
{
  uint8_t timestamp[8];     // microseconds of uptime
  uint16_t interval;        // time units between beacons (TU - 1.024 microseconds)
  uint16_t capabilityInfo;  // bitmap
  wifi_ieee80211_beacon_element_t element;
} wifi_ieee80211_beacon_t;

typedef struct __attribute__((packed))
{
  uint16_t frame_ctrl;
  uint16_t duration_id;
  uint8_t addr1[6]; // receiver address
  uint8_t addr2[6]; // sender address
  uint8_t addr3[6]; // filtering address
  uint16_t sequence_ctrl;
  uint8_t frame[0]; // data
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void wifi_pkt_hndlr(void* buff, wifi_promiscuous_pkt_type_t type)
{
  // we only want management packets
  if (type == WIFI_PKT_MGMT)// && scanning)
  {
    const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buff;
    const wifi_ieee80211_mac_hdr_t *hdr = (wifi_ieee80211_mac_hdr_t *)ppkt->payload;
    
    uint8_t fc0   = hdr->frame_ctrl & 0xFF;
    uint8_t stype = (fc0 >> 4) & 0x0F;
    uint8_t ftype = (fc0 >> 2) & 0x03;

    if (stype == 8)
    {
      const wifi_ieee80211_beacon_t* beacon = (wifi_ieee80211_beacon_t*)hdr->frame;
      const wifi_ieee80211_beacon_element_t* element = &beacon->element;
      found_beacon_t bcn;
      memset(bcn.ssid, 0, 33);

      // this is a beacon, so the first element should be SSID - this is a sanity check
      if (element->element_id == 0)
      {
        if (element->length)
        {
          memcpy(bcn.ssid, element->data, element->length);
        }
        else
        {
          strncpy(bcn.ssid, "<HIDDEN>", 32);
        }

        bcn.channel = channels[channelInx];
        bcn.rssi = ppkt->rx_ctrl.rssi;
        memcpy(bcn.mac, hdr->addr2, 6);

        // generate a key for the std::map - this will be the md5 hash of the found beacon struct
        md5_init(&ctx);
        md5_digest(&ctx, &bcn, sizeof(bcn) - 1);  // do not include RSSI in key 
        md5_output(&ctx, md5sum);

        uint32_t key = ((uint32_t)md5sum[0] << 24) | ((uint32_t)md5sum[1] << 16) | ((uint32_t)md5sum[2] << 8) | ((uint32_t)md5sum[3]);
        
        // have we seen this one already?
        std::map<uint32_t, found_beacon_t>::iterator it = beacons.find(key);
        if (it == beacons.end())
        {
          beacons[key] = bcn;
        }
      }
    }
    return;
  }
}

void SCANNER::begin()
{
  channelInx = 0;
  scanning = false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  //esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_pkt_hndlr);
  esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE); 
  
  esp_wifi_set_promiscuous(false);   
}

void SCANNER::survey()
{
  uint32_t msnow = millis();

  Serial.printf(CLI_CYA "Survey starting\r\n" CLI_RESET);
  channelInx = 0;
  scanning = true;

  esp_wifi_set_promiscuous(true);
  Serial.printf(CLI_YEL "Setting channel %d" CLI_RESET, channels[channelInx]);
  esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE);    

  once = false;

  while (scanning)
  {
    if ((millis() - msnow) > 500)
    {
      msnow = millis();
      ++channelInx;
      channelInx %= channelCount;

      if (!channelInx)
      {
        scanning = false;
        esp_wifi_set_promiscuous(false);
        Serial.printf("\r\n");
      }
      else
      {
        Serial.printf(CLI_YEL "...%d" CLI_RESET, channels[channelInx]);
        esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE); 
      }
    }
  }

  Serial.printf(CLI_YEL "Found beacons:\r\n" CLI_RESET);
  for (std::map<uint32_t, found_beacon_t>::const_iterator it = beacons.begin(); it != beacons.end(); ++it)
  {
    Serial.printf(CLI_YEL "MAC:" CLI_GRN "%02x:%02x:%02x:%02x:%02x:%02x" CLI_YEL ", CH:" CLI_GRN "%d" 
                  CLI_YEL ", SSID:" CLI_GRN "%s" CLI_YEL ", RSSI:" CLI_GRN "%d\r\n" CLI_RESET,
                  it->second.mac[0], it->second.mac[1], it->second.mac[2], it->second.mac[3], it->second.mac[4], it->second.mac[5], 
                  it->second.channel, it->second.ssid, it->second.rssi);
  }

  Serial.printf(CLI_CYA "Survey done\r\n" CLI_RESET);
}
