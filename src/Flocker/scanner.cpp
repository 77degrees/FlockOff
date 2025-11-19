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

// 2G WiFi channels
static const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                   36, 40, 44, 48, 149, 153, 157, 161, 165};

const size_t channelCount = sizeof(channels) / sizeof(channels[0]);
static uint16_t channelInx = 0;   
static bool scanning = false;
static md5_context ctx;
static uint8_t md5sum[16];
static std::map<uint32_t, found_beacon_t> beacons;

/*************************************************
* Promiscuous mode packet callback handler
*
**************************************************
* If an 802.11 packet of the WIFI_MANAGEMENT is is
* seen, and if that packet is of subtype BEACON,
* then add it to a std::map of results.  
*
* Note, before adding, check to see if that one
* is already in the map to prevent dups
*
* Frankly, it blows my mind that a tiny six buck
* microcontroller has the resources to handle 
* STL containers (with iterators, no less!).
*************************************************/
void wifi_pkt_hndlr(void* buff, wifi_promiscuous_pkt_type_t type)
{
  // we only want management packets
  if (type == WIFI_PKT_MGMT)// && scanning)
  {
    // make some sense of the buffer
    const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buff;
    const wifi_ieee80211_mac_hdr_t *hdr = (wifi_ieee80211_mac_hdr_t *)ppkt->payload;
    
    // IEEE 802.11 Wifi header we begin with frame type and subtype
    uint8_t fc0   = hdr->frame_ctrl & 0xFF;
    uint8_t stype = (fc0 >> 4) & 0x0F;
    uint8_t ftype = (fc0 >> 2) & 0x03;

    // at this point, we are only looking for management frames that are BEACONS
    if (stype == 8)
    {
      // make sense out of the payload
      const wifi_ieee80211_beacon_t* beacon = (wifi_ieee80211_beacon_t*)hdr->frame;
      const wifi_ieee80211_beacon_element_t* element = &beacon->element;
      
      // a place to store the results
      found_beacon_t bcn;
      memset(bcn.ssid, 0, 33);

      // this is a beacon, so the first element should be SSID - this is a sanity check
      if (element->element_id == 0)
      {
        // element length is the length of the SSID char array.  NOT ZERO TERMED, max 32 chars
        if (element->length)
        {
          memcpy(bcn.ssid, element->data, element->length);
        }
        else
        {
          // does someone feel smart?  :/
          strncpy(bcn.ssid, "<HIDDEN>", 32);
        }

        // populate the rest of the the results
        bcn.channel = channels[channelInx]; // wifi channel
        bcn.rssi = ppkt->rx_ctrl.rssi;      // signal strength
        memcpy(bcn.mac, hdr->addr2, 6);     // MAC of the WiFi AP that sent the beacon

        // generate a key for the std::map - this will be the md5 hash of the found beacon struct
        md5_init(&ctx);
        md5_digest(&ctx, &bcn, sizeof(bcn) - 1);  // do not include RSSI in key; we'll get multiple reads for the 
        md5_output(&ctx, md5sum);                 // beacon, and end up with dups in the map due to varying signal

        // kinda hokey, the key are the first 4 bytes of the MD5 hash of the struct.  Shouldn't be collisions....
        uint32_t key = ((uint32_t)md5sum[0] << 24) | ((uint32_t)md5sum[1] << 16) | ((uint32_t)md5sum[2] << 8) | ((uint32_t)md5sum[3]);
        
        // have we seen this one already?
        std::map<uint32_t, found_beacon_t>::iterator it = beacons.find(key);
        if (it == beacons.end())
        {
          // no?  then add it
          beacons[key] = bcn;
        }
      }
    }
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

/*****************************************************
* Do a single pass through all WiFi channels to see
* what we can find out there.
*****************************************************/
void SCANNER::survey(uint32_t timing)
{
  uint32_t msnow = millis();
  beacons.clear();

  Serial.printf(CLI_CYA "Survey starting\r\n" CLI_RESET);
  channelInx = 0;
  scanning = true;

  esp_wifi_set_promiscuous(true);
  Serial.printf(CLI_YEL "Setting channel %d" CLI_RESET, channels[channelInx]);
  esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE);    

  while (scanning)
  {
    if ((millis() - msnow) > timing)
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
