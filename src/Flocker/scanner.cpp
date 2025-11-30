#include <map>

#include <WiFi.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <MD5Builder.h>
#include <ArduinoJson.h>

#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "scanner.h"

#include "globals.h"

// We're only looking at management 802.11 frames.  There are 16
// subtypes of management frames (as #defined here).  The packet
// structure is different for each subtype, but in general:
//   HEADER (fixed size)
//   FIXED DATA (variable number of bytes, zero or more)
//   TAGGED DATA (variable number of tagged parameters, zero or more)
//   CRC32 (4 bytes)
#define MGMT_FRAME_SUBTYPE_ASSOCIATION_REQUEST 0x00
#define MGMT_FRAME_SUBTYPE_ASSOCIATION_RESPONSE 0x01
#define MGMT_FRAME_SUBTYPE_REASSOCIATION_REQUEST 0x02
#define MGMT_FRAME_SUBTYPE_REASSOCIATION_RESPONSE 0x03
#define MGMT_FRAME_SUBTYPE_PROBE_REQUEST 0x04
#define MGMT_FRAME_SUBTYPE_PROBE_RESPONSE 0x05
#define MGMT_FRAME_SUBTYPE_TIMING_ADVERTISEMENT 0x06
#define MGMT_FRAME_SUBTYPE_RESERVED_1 0x07
#define MGMT_FRAME_SUBTYPE_BEACON 0x08
#define MGMT_FRAME_SUBTYPE_ATIM 0x09
#define MGMT_FRAME_SUBTYPE_DISASSOCIATION 0x0a
#define MGMT_FRAME_SUBTYPE_AUTHENTICATION 0x0b
#define MGMT_FRAME_SUBTYPE_DEAUTHENTICATION 0x0c
#define MGMT_FRAME_SUBTYPE_ACTION 0x0d
#define MGMT_FRAME_SUBTYPE_ACTION_NO_ACK 0x0e
#define MGMT_FRAME_SUBTYPE_RESERVED2 0x0f

// Each of the management frame subtypes (defined above) has zero or
// more fixed bytes of parameter data before getting to the zero or
// more tagged parameters.  This array holds the number of bytes
// for each subtype
uint8_t fixedParameterLength[] = {
  4,  // 0x00 - length of fixed parameters in assoc request subtype
  6,  // 0x01 - length of fixed parameters in assoc response subtype
  10, // 0x02 - length of fixed parameters in reassoc request subtype
  6,  // 0x03 - length of fixed parameters in reassoc response subtype
  0,  // 0x04 - length of fixed parameters in probe request subtype
  12, // 0x05 - length of fixed parameters in probe response subtype
  0,  // 0x06 - timing advertisement
  0,  // 0x07 - reserved subtype
  12, // 0x08 - length of fixed parameters in beacon subtype
  0,  // 0x09 - ATIM (announcement traffic indication message - something else entirely)
  2,  // 0x0a - length of fixed parameters in dissociation subtype
  6,  // 0x0b - length of fixed parameters in authentication subtype
  2,  // 0x0c - length of fixed parameters in deauthentication subtype
  17, // 0x0d - length of fixed parameters in action subtype
  0,  // 0x0e - length of fixed parameters in action no-ack subtype
  0   // 0x07 - reserved subtype
};

// structure to hold details about every device found; not just
// Flock type things at this point (found by WiFi)
struct __attribute__((packed)) found_wifi_t
{
  FLOCK_DISCOVERY_METHOD method;
  uint8_t subtype;
  uint8_t channel;
  char ssid[SSID_LEN + 1];
  uint8_t mac[6];
  int8_t rssi;
};

// structure to hold details about every device found; not just
// Flock type things at this point (found by Bluetooth LE)
struct __attribute__((packed)) found_ble_t
{
  FLOCK_DISCOVERY_METHOD method;
  char name[SSID_LEN + 1];
  uint8_t addr[6];
  char mfg[1001];
  int8_t rssi;
};

// variable length tagged parameter in a management packet.  A tagged parameter
// is made up of the ID, length of the data, and the data itself.  For 
// 802.11 packets, parameter 0 is the SSID, up to 32 bytes long
struct __attribute__((packed)) wifi_ieee80211_mgmt_tagged_parameters_t
{
  uint8_t parameter_id; // parameter ID
  uint8_t length;       // length of parameter
  uint8_t data[0];      // parameter data
};

// 802.11 header
struct __attribute__((packed)) wifi_ieee80211_mac_hdr_t
{
  uint16_t frame_ctrl;
  uint16_t duration_id;
  uint8_t addr1[6]; // receiver address
  uint8_t addr2[6]; // sender address
  uint8_t addr3[6]; // filtering address
  uint16_t sequence_ctrl;
  uint8_t frame[0]; // frame data - start of fixed data (if any) and then tagged parameters (if any)
};

// WiFi channels
static const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                   36, 40, 44, 48, 149, 153, 157, 161, 165};

const size_t channelCount = sizeof(channels) / sizeof(channels[0]);
static uint16_t channelInx = 0;   
static bool scanning = false;
static MD5Builder hasher;
static NimBLEScan* btScanner = nullptr;
static uint8_t md5sum[16];
static std::map<uint32_t, found_wifi_t> wifiDevices;
static std::map<uint32_t, found_wifi_t>::const_iterator citWifiDevices;
static std::map<uint32_t, found_ble_t> bleDevices;
static std::map<uint32_t, found_ble_t>::const_iterator citBleDevices;

/*************************************************
* Promiscuous mode packet callback handler
*
**************************************************
* If an 802.11 packet of the WIFI_MANAGEMENT is
* seen, add it to a std::map of results.  
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
  // make some sense of the buffer.  The passed buffer is of type promiscuous packet;
  // we dont' care about most of that (other than the packet length), so just skip
  // right to the payload, which is the start of the header
  const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buff;
  const wifi_ieee80211_mac_hdr_t *hdr = (wifi_ieee80211_mac_hdr_t *)ppkt->payload;
  
  // get the length of the packet (minus header stuffs and CRC32);
  size_t pktLen = ppkt->rx_ctrl.sig_len - sizeof(*hdr) - 4; // last 4 is the CRC32

  // IEEE 802.11 Wifi header - get the subtype.  We know these are only management
  // frames because we set the promiscuous mode filter for them
  uint8_t stype = ((hdr->frame_ctrl & 0xFF) >> 4) & 0x0F;

  // remaining packet length, reduced by count of fixed data bytes
  if (pktLen > fixedParameterLength[stype])
  {
    pktLen -= fixedParameterLength[stype];
  }
  else
  {
    return; // this is one of the short management packets without any tagged data
  }

  // make sense out of the payload.  This depends a lot on the frame subtype; each subtype
  // has zero or more bytes if "fixed data", followed by zero or more tagged parameters.
  // skip past the fixed data bytes to get to the first tagged parameter
  const wifi_ieee80211_mgmt_tagged_parameters_t* taggedPar = 
        (wifi_ieee80211_mgmt_tagged_parameters_t*)&hdr->frame[fixedParameterLength[stype]];

  // a place to store the results
  found_wifi_t wifi;
  wifi.method = WIFI_DISCOVERY;         // we found this device through wifi
  wifi.subtype = stype;                 // keep track of the frame subtype
  memset(wifi.ssid, 0, SSID_LEN + 1);   // clear buffer for ssid

  // try to get SSID tagged parameter
  if (pktLen > sizeof(wifi_ieee80211_mgmt_tagged_parameters_t))
  
  {
    // is this parameter an SSID?
    if (taggedPar->parameter_id == 0)
    {
      // parameter length is the length of the SSID char array.  
      // NOT ZERO TERMED, max 32 chars
      if (taggedPar->length && (taggedPar->length < SSID_LEN))
      {
        memcpy(wifi.ssid, taggedPar->data, taggedPar->length);
      }
      else
      {
        // does someone feel smart?  :/
        strncpy(wifi.ssid, "<HIDDEN>", SSID_LEN);
      }

      // populate the rest of the the results
      wifi.channel = channels[channelInx]; // wifi channel
      wifi.rssi = ppkt->rx_ctrl.rssi;      // signal strength
      memcpy(wifi.mac, hdr->addr3, 6);     // MAC of the WiFi AP that sent the packet

      // generate a key for the std::map - this will be the md5 hash of the found packet struct
      // Note, do not include the RSSI in the hash, or we'll end up with dups in the map
      hasher.begin();
      hasher.add((uint8_t*)&wifi, sizeof(wifi) - 1);
      hasher.calculate();
      hasher.getBytes(md5sum);

      // kinda hokey, the key are the first 4 bytes of the MD5 hash of the struct.  Shouldn't be collisions....
      uint32_t key = ((uint32_t)md5sum[0] << 24) | ((uint32_t)md5sum[1] << 16) | 
                     ((uint32_t)md5sum[2] << 8) | ((uint32_t)md5sum[3]);
      
      // have we seen this one already?
      citWifiDevices = wifiDevices.find(key);
      if (citWifiDevices == wifiDevices.end())
      {
        // no?  then add it
        wifiDevices[key] = wifi;
        //flockLED.pulseRed(LEDS::LED_COMMS, 10);
      }
    } // this element is an SSID
  } // This is an element
}


class btAdvertisedCBs : public NimBLEScanCallbacks
{
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice)
  {
    btDevice.method = BTLE_DISCOVERY;
    btDevice.rssi = advertisedDevice->getRSSI();

    payload = advertisedDevice->getPayload();
    citPayload = payload.begin();
    if (citPayload == payload.end())
    {
      return;
    }

    for (size_t ii = 0; ii < 6; ++ii)
    {
      btDevice.addr[ii] = *citPayload;
      ++citPayload;
    }

    //memcpy(btDevice.addr, advertisedDevice->getAddress().getVal(), 6);

    /*
    uint8_t mac[6];
    NimBLEAddress addr = advertisedDevice->getAddress();
    String macStr = addr.toString().c_str();
    if (!parseMac6(macStr, mac)) 
    {
      return;
    }
    */

    if (advertisedDevice->haveName())
    {
      std::string nimbleName = advertisedDevice->getName();
      size_t nameInx = 0;
      if (nimbleName.length() > 0)
      {
        memset(btDevice.name, 0, SSID_LEN + 1);

        for (size_t ii = 0; ii < nimbleName.length() && ii < 31; ++ii)
        {
          uint8_t c = (uint8_t)nimbleName[ii];
          if (isprint(c))
          {
            btDevice.name[nameInx++] = (char)c;
          }
        }

        // all non-printable characters in BLE device name?
        if (!strlen(btDevice.name))
        {
            strncpy(btDevice.name, "<UNKNOWN>", SSID_LEN);
        }
      }
      else
      {
        strncpy(btDevice.name, "<UNKNOWN>", SSID_LEN);
      }
    }

    strncpy(btDevice.mfg, advertisedDevice->toString().c_str(), 1000);

    hasher.begin();
    hasher.add((uint8_t*)&btDevice, sizeof(btDevice) - 1);
    hasher.calculate();
    hasher.getBytes(md5sum);

    // kinda hokey, the key are the first 4 bytes of the MD5 hash of the struct.  Shouldn't be collisions....
    uint32_t key = ((uint32_t)md5sum[0] << 24) | ((uint32_t)md5sum[1] << 16) | 
                    ((uint32_t)md5sum[2] << 8) | ((uint32_t)md5sum[3]);
    
    // have we seen this one already?
    citBleDevices = bleDevices.find(key);
    if (citBleDevices == bleDevices.end())
    {
      // no?  then add it
      bleDevices[key] = btDevice;
    }

  }

private:
  found_ble_t btDevice;
  std::vector<uint8_t> payload;
  std::vector<uint8_t>::const_iterator citPayload;
};



bool SCANNER::begin()
{
  channelInx = 0;
  scanning = false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // turn off promiscuity for now
  esp_wifi_set_promiscuous(false);

  // we're only interested in MANAGEMENT frames
	const wifi_promiscuous_filter_t filt = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
	esp_wifi_set_promiscuous_filter(&filt); 

  // set callback
  esp_wifi_set_promiscuous_rx_cb(&wifi_pkt_hndlr);

  // start with first channel
  esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE); 
  
  return (true);
}


void SCANNER::startBLE()
{
  if (btScanner)
  {
    Serial.printf(CLI_CYA "SCANNER::startBLE() - scanner was non-null\r\n" CLI_RESET);
    this->stopBLE();
    delay(100);
  }

  BLEDevice::init("");
  btScanner = BLEDevice::getScan();
  btScanner->setScanCallbacks(new btAdvertisedCBs(), true);
  btScanner->setActiveScan(true);
  btScanner->setInterval(100);
  btScanner->setWindow(99);
  btScanner->setDuplicateFilter(false);
  btScanner->start(0, false);
}


void SCANNER::stopBLE()
{
  if (btScanner)
  {
    Serial.printf(CLI_CYA "SCANNER::stopBLE() - scanner was non-null\r\n" CLI_RESET);
    btScanner->stop();
    delay(200);
    btScanner->clearResults();
    NimBLEDevice::deinit(true);
    btScanner = nullptr;
  }
}

/*****************************************************
* Do a single pass through all WiFi channels to see
* what we can find out there.
*****************************************************/
void SCANNER::survey(uint32_t interval, bool doWiFi, bool doBT, const char* fname, bool doJson, const char* notes)
{
  uint32_t msnow = millis();
  bleDevices.clear();
  wifiDevices.clear();

  flockLED.alertBlu(LEDS::LED_COMMS);
  flockLED.alertGrn(LEDS::LED_COMMS);

  Serial.printf(CLI_CYA "Survey starting.\r\n" CLI_RESET);
  channelInx = 0;
  scanning = doWiFi;

  if (scanning)
  { 
    Serial.printf(CLI_CYA "Starting WiFi.\r\n" CLI_RESET);
    esp_wifi_set_promiscuous(true);
    Serial.printf(CLI_YEL "Setting channel %d" CLI_RESET, channels[channelInx]);
    esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE);   

    while (scanning)
    {
      if ((millis() - msnow) > interval)
      {
        msnow = millis();
        ++channelInx;
        channelInx %= channelCount;

        if (!channelInx)
        {
          scanning = false;
          esp_wifi_set_promiscuous(false);
          Serial.printf(CLI_CYA "\r\nWiFi done.\r\n" CLI_RESET);
        }
        else
        {
          Serial.printf(CLI_YEL "...%d" CLI_RESET, channels[channelInx]);
          esp_wifi_set_channel(channels[channelInx], WIFI_SECOND_CHAN_NONE); 
        }
      }

      flockLED.update();
    }
  }

  if (doBT)
  {
    Serial.printf(CLI_CYA "Starting BLE\r\n");
    this->startBLE();
    delay(interval * 5);
    this->stopBLE();
    Serial.printf(CLI_CYA "BLE Done, survey complete\r\n");
  }

  JsonDocument sur;
  if (notes && strlen(notes))     sur["SurveyNotes"] = notes;
  else                            sur["SurveyNotes"] = "Generic survey";

  JsonArray location = sur["LocationLongLat"].to<JsonArray>();
  location.add(gps.getLongitude());
  location.add(gps.getLatitude());

  time_t t = time(NULL);
  tm *tmp;
  tmp = localtime(&t);
  char tstring[64];
  strftime(tstring, 63, "%F %T", tmp);
  sur["DateTime"] = tstring;
  sur["Timezone"] = flockCfg.getTimeZone();

  JsonArray devs = sur["Devices"].to<JsonArray>();
  JsonDocument dev;

  for (citWifiDevices = wifiDevices.begin(); citWifiDevices != wifiDevices.end(); ++citWifiDevices)
  {
    dev.clear();
    dev["Method"] = discoveryToText(citWifiDevices->second.method);
    dev["Subtype"] = mgmtSubtypeToText(citWifiDevices->second.subtype);
    dev["BSSID"] = macToText(citWifiDevices->second.mac);
    dev["Channel"] = citWifiDevices->second.channel;
    dev["SSID"] = citWifiDevices->second.ssid;
    dev["RSSSI"] = citWifiDevices->second.rssi;
    //dev["MfgData"] = citWifiDevices->second.mfg;

    devs.add(dev);
  }

  Serial.printf(CLI_CYA "Found " CLI_GRN "%d" CLI_CYA " devices:\r\n" CLI_RESET, wifiDevices.size());
  serializeJsonPretty(sur, Serial);
  Serial.printf("\r\n");

  if (fname && strlen(fname))
  {
    Serial.printf(CLI_CYA "Saving survey results to " CLI_GRN "%s" CLI_CYA ", format " CLI_YEL "%s\r\n" CLI_RESET,
      fname, doJson ? "JSON" : "text");

    char* output = (char*)ps_malloc(4097);
    memset(output, 0, 4097);

    //if (doJson)
    {
 
      serializeJson(sur, output, 4096);
      
      size_t written = flockfs.writeFile(fname, (uint8_t*)output, strlen(output));
      Serial.printf(CLI_CYA "Wrote " CLI_GRN "%d" CLI_CYA " bytes to file\r\n" CLI_RESET, written);
    }

    free (output);
  }

  flockLED.stopBlu(LEDS::LED_COMMS);
  flockLED.stopGrn(LEDS::LED_COMMS);
}

const char* discoveryToText(FLOCK_DISCOVERY_METHOD meth)
{
  switch (meth)
  {
    case NO_DISCOVERY:      return ("Unknown");
    case WIFI_DISCOVERY:    return ("WiFi");
    case BTLE_DISCOVERY:    return ("BTLE");
  }

  return ("");
}

const char* mgmtSubtypeToText(uint8_t stype)
{
  switch (stype)
  {
    case MGMT_FRAME_SUBTYPE_ASSOCIATION_REQUEST:      return ("associaton request");
    case MGMT_FRAME_SUBTYPE_ASSOCIATION_RESPONSE:     return ("association response");
    case MGMT_FRAME_SUBTYPE_REASSOCIATION_REQUEST:    return ("reassociation request");
    case MGMT_FRAME_SUBTYPE_REASSOCIATION_RESPONSE:   return ("reassociation response");
    case MGMT_FRAME_SUBTYPE_PROBE_REQUEST:            return ("probe request");
    case MGMT_FRAME_SUBTYPE_PROBE_RESPONSE:           return ("probe response");
    case MGMT_FRAME_SUBTYPE_TIMING_ADVERTISEMENT:     return ("timing advertisement");
    case MGMT_FRAME_SUBTYPE_BEACON:                   return ("beacon");
    case MGMT_FRAME_SUBTYPE_ATIM:                     return ("ATIM");
    case MGMT_FRAME_SUBTYPE_DISASSOCIATION:           return ("disassociation");
    case MGMT_FRAME_SUBTYPE_AUTHENTICATION:           return ("authentication");
    case MGMT_FRAME_SUBTYPE_DEAUTHENTICATION:         return ("deauthentication");
    case MGMT_FRAME_SUBTYPE_ACTION:                   return ("action");
    case MGMT_FRAME_SUBTYPE_ACTION_NO_ACK:            return ("action no-ack");
  }

  return ("");
}

const char* btleAdvertisedTypeToText(uint8_t type)
{
  return ("test");
}

const char* macToText(const uint8_t* mac)
{
  static char ret[20];
  if (!mac)
  {
    return (NULL);
  }

  snprintf(ret, 19, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return (ret);
}