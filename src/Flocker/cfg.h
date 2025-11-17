#ifndef CFG_H_
#define CFG_H_

#include <ArduinoJson.h>

class CONFIG
{
public:
  CONFIG() {}
  ~CONFIG() {}

  void begin();
  bool buildDefualtConfig();
  void outputJson();

private:
  JsonDocument cfg;
};

#endif //CFG_H_