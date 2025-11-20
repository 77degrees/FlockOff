#ifndef CFG_H_
#define CFG_H_

#include <ArduinoJson.h>

class CONFIG
{
public:
  CONFIG() {}
  ~CONFIG() {}

  bool begin();
  bool buildDefualtConfig();
  void outputJson();
  void selectTimeZone();
  void setTimeZone();
  const char* getTimeZone();
  //const char** getWiFiAPs();

private:
  bool writeConfig();
  JsonDocument cfg;

};

#endif //CFG_H_