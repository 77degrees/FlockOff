#include "Arduino.h"
#include "gps.h"

#include "globals.h"

bool NMEAGPS::begin(uint32_t baud, int8_t rxPin, int8_t txPin)
{
  parsing = false;
  parsInx = 0;

  dataValid = false;
  fixQuality = 0;

  course = 0.0;
  latitude = 0.0;
  longitude = 0.0;
  speed = 0.0;

  gpsPort.begin(baud, SWSERIAL_8N1, txPin, rxPin);

  // GPIO1 for red, 2 for green, 3 for blue
  leds.begin(1, 2, 3);
  lastMsgOffset = millis();

  return(gpsPort);
}

void NMEAGPS::parseSentence() 
{
  static bool once = false;
  static uint32_t offset = millis();

  switch (minmea_sentence_id(sentence, false)) 
  {
    case MINMEA_SENTENCE_RMC:
    {
      static bool timeIsSet = false;

      struct minmea_sentence_rmc frame;
      if (minmea_parse_rmc(&frame, sentence)) 
      {
        this->latitude = minmea_tocoord(&frame.latitude);
        this->longitude = minmea_tocoord(&frame.longitude);
        this->speed = minmea_tofloat(&frame.speed);
        this->course = minmea_tofloat(&frame.course);

        minmea_getdatetime(&this->localtm, &frame.date, &frame.time);
        once = true;

        if (!timeIsSet)
        {
          setenv("TZ", "GMT0", 1);
          tzset();

          timeval setTime;
          setTime.tv_sec = mktime(&this->localtm);
          setTime.tv_usec = 0;
          settimeofday(&setTime, NULL);

          flockCfg.setTimeZone();
          timeIsSet = true;
        }
      }
      else
      {
        Serial.printf("Error parsing RMC\r\n");
      }
    }  break;

    case MINMEA_SENTENCE_GGA: 
    {
      struct minmea_sentence_gga frame;
      if (minmea_parse_gga(&frame, sentence)) 
      {
        this->fixQuality = frame.fix_quality;
        once = true;
      }
      else
      {
        Serial.printf("Error parsing GGA\r\n");
      }
    }  break;

    case MINMEA_SENTENCE_GLL:
    {
      struct minmea_sentence_gll frame;
      if (minmea_parse_gll(&frame, sentence))
      {
        if (frame.status == MINMEA_GLL_STATUS_DATA_VALID)
        {
          dataValid = true;
        }
        else
        {
          dataValid = false;
        }
        once = true;
      }
      else
      {
        Serial.printf("Error parsing GLL\r\n");
      }
    }  break;

    case MINMEA_UNKNOWN:
    //case MINMEA_INVALID:
    case MINMEA_SENTENCE_TXT:
    {
      Serial.print(sentence);
    }  break;
  }

  if (once)
  {
    if ((fixQuality > 0) || dataValid)
    {
      if ((millis() - offset) > 750)
      {
        offset = millis();
        leds.pulseGrn(2);
        leds.stopBlu();
      }
    }
    else if (!leds.isBluActive())
    {
      leds.cycleBlu(2600, 2);
    }
  }
}

void NMEAGPS::update()
{
  if (gpsPort.available())
  {
    char c = gpsPort.read();

    if (!parsing) 
    {
      if (c == '$') 
      {
        parsInx = 0;
        memset(sentence, 0x00, MINMEA_MAX_SENTENCE_LENGTH + 1);
        parsing = true;
      } // leading $
    } // not parsing

    if (parsing) 
    {
      if (parsInx >= MINMEA_MAX_SENTENCE_LENGTH)
      {
        parsing = false;
        return;
      }

      sentence[parsInx++] = c;

      if (c == '\n') 
      {
        lastMsgOffset = millis();
        this->parseSentence();
        leds.pulseGrn(10);
      } // final newline
    } // parsing == true
  }  // char available

  if ((millis() - lastMsgOffset) > 10000)
  {
    lastMsgOffset = millis();
    leds.pulseRed(3);
    leds.stopBlu();
  }

  leds.update();
}  // update()
