#ifndef CLI_IMPL_H_
#define CLI_IMPL_H_

#define CLI_BUFFER_SIZE 1024
#define CLI_RX_BUFFER_SIZE 128
#define CLI_CMD_BUFFER_SIZE 128
#define CLI_HISTORY_SIZE 32
#define CLI_BINDING_COUNT 3

#define CLI_BUFFER_SIZE_PS 8192
#define CLI_RX_BUFFER_SIZE_PS 1024
#define CLI_CMD_BUFFER_SIZE_PS 1024
#define CLI_HISTORY_SIZE_PS 64
#define CLI_BINDING_COUNT_PS 10

#define CLI_RESET "\x1b[0m"
#define CLI_BOLD "\x1b[1m"
#define CLI_BOLD_RED "\x1b[1;31m"

#define CLI_ERROR(x)  CLI_BOLD_RED x CLI_RESET

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);
void writeChar(EmbeddedCli *embeddedCli, char c);

void onGPS(EmbeddedCli *cli, char *args, void *context);
void onClear(EmbeddedCli *cli, char *args, void *context);
void onReset(EmbeddedCli *cli, char *args, void *context);
void onLs(EmbeddedCli *cli, char *args, void *context);
void onWrite(EmbeddedCli *cli, char *args, void *context);
void onCat(EmbeddedCli *cli, char *args, void *context);

const struct CliCommandBinding bindings[] = {{"gps", "Perform GPS functions", true, nullptr, onGPS},
                                             {"clear", "Clear the console", false, nullptr, onClear},
                                             {"reset", "Reboot the device", false, nullptr, onReset},
                                             {"ls", "list files", false, nullptr, onLs},
                                             {"write", "write test file", false, nullptr, onWrite},
                                             {"cat", "cat test file", false, nullptr,  onCat}};

const size_t bindingCount = sizeof(bindings) / sizeof(bindings[0]);



void onCommand(EmbeddedCli *embeddedCli, CliCommand *command)
{
  Serial.printf(CLI_ERROR("Uknown command") ", use 'help' for command list\r\n");
}

void writeChar(EmbeddedCli *embeddedCli, char c)
{
  Serial.write(c);
}

void onGPS(EmbeddedCli *cli, char *args, void *context)
{
  bool err = false;
  bool goodFix = false;
  if (gps.getFixQuality() > 0)  goodFix = true;

  uint16_t argc = embeddedCliGetTokenCount(args);
  for (uint16_t ii = 1; ii <= argc; ++ii)
  {
    const char* a = embeddedCliGetToken(args, ii);

    if (a[0] == '-' && strlen(a) > 1)
    {
      switch(a[1])
      {
        case 'q': 
        {
          Serial.printf("GPS fix quality is %s (%d)\r\n", goodFix ? "good" : "bad", gps.getFixQuality());
        }  break;

        case 'p':
        {
          Serial.printf("GPS current coordinates are %02.5f, %03.5f\r\n", 
              gps.getLatitude(), gps.getLongitude());
        }  break;

        case 't':
        {
          struct tm tm_;
          gps.getTime(&tm_);

          Serial.printf("GPS current time (gmt) is %02d:%02d:%02d  %d/%d/%d\r\n", 
              tm_.tm_hour, tm_.tm_min, tm_.tm_sec,
              tm_.tm_mon, tm_.tm_mday, tm_.tm_year + 1900);         
        }  break;

        default: err = true;  
      }
    }
  
    if (err)
    {
      Serial.printf("gps command parameters:\r\n\t'q' - get GPS fix quality"
                    "\r\n\t'p' - get GPS coordinates\r\n\t't' - get GPS time\r\n");

      return;
    }
  }
}

void onClear(EmbeddedCli *cli, char *args, void *context)
{
    Serial.printf("\33[2J");
}

void onLs(EmbeddedCli *cli, char *args, void *context)
{
  listDir("/", 3);
}

void onWrite(EmbeddedCli *cli, char *args, void *context)
{
  const char* data = "This is a radio clash on pirate satellite";
  writeFile("/test.txt", data);
}

void onCat(EmbeddedCli *cli, char *args, void *context)
{
  readFile("/test.txt");
}

void onReset(EmbeddedCli *cli, char *args, void *context)
{
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 100,                 // Convertin ms
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Bitmask of all cores, https://github.com/espressif/esp-idf/blob/v5.2.2/examples/system/task_watchdog/main/task_watchdog_example_main.c
    .trigger_panic = true                             // Enable panic to restart ESP32
  };
  // WDT Init
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
  while (true);
}

#endif // CLI_IMPL_H_
