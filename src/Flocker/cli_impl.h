#ifndef CLI_IMPL_H_
#define CLI_IMPL_H_

#define CLI_BUFFER_SIZE_PS 8192
#define CLI_RX_BUFFER_SIZE_PS 1024
#define CLI_CMD_BUFFER_SIZE_PS 1024
#define CLI_HISTORY_SIZE_PS 64
#define CLI_BINDING_COUNT_PS 20

#define CLI_RESET "\x1b[0m"

#define CLI_BLK "\x1b[30m"
#define CLI_RED "\x1b[31m"
#define CLI_GRN "\x1b[32m"
#define CLI_YEL "\x1b[33m" 
#define CLI_BLU "\x1b[34m"
#define CLI_PUR "\x1b[35m"
#define CLI_CYA "\x1b[36m"
#define CLI_WHT "\x1b[37m"

#define CLI_BOLD "\x1b[1m"
#define CLI_BOLD_BLK "\x1b[1;30m"
#define CLI_BOLD_RED "\x1b[1;31m"
#define CLI_BOLD_GRN "\x1b[1;32m"
#define CLI_BOLD_YEL "\x1b[1;33m" 
#define CLI_BOLD_BLU "\x1b[1;34m"
#define CLI_BOLD_PUR "\x1b[1;35m"
#define CLI_BOLD_CYA "\x1b[1;36m"
#define CLI_BOLD_WHT "\x1b[1;37m"

#define CLI_ERROR(x)  CLI_BOLD_RED x CLI_RESET

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);
void writeChar(EmbeddedCli *embeddedCli, char c);

void onGPS(EmbeddedCli* cli, char* args, void* context);
void onClear(EmbeddedCli* cli, char* args, void* context);
void onReset(EmbeddedCli* cli, char* args, void* context);
void onFSinfo(EmbeddedCli* cli, char* args, void* context);
void onLs(EmbeddedCli* cli, char* args, void* context);
void onDel(EmbeddedCli* cli, char* args, void* context);
void onWrite(EmbeddedCli* cli, char* args, void* context);
void onMv(EmbeddedCli* cli, char* args, void* context);
void onCp(EmbeddedCli* cli, char* args, void* context);
void onCat(EmbeddedCli* cli, char* args, void* context);
void onStatus(EmbeddedCli* cli, char* args, void* context);
void onSetupCfg(EmbeddedCli* cli, char* args, void* context);
void onPrintCfg(EmbeddedCli* cli, char* args, void* context);

const struct CliCommandBinding bindings[] = {{"gps", "Perform GPS functions", "-t for time, -p for position, -q for quality", true, nullptr, onGPS},
                                             {"clear", "Clear the console", "Clear the serial console screen", false, nullptr, onClear},
                                             {"reset", "Reboot the device", "Closes filesystems and resets board", false, nullptr, onReset},
                                             {"fsinfo", "Get filesystem information", "Get total and free space in filesystem", false, nullptr, onFSinfo},
                                             {"ls", "List files", "List files in filesystem", false, nullptr, onLs},
                                             {"rm", "Delete file", "<filename> to delete", true, nullptr, onDel},
                                             {"write", "Write test file", "-f <filename> -d <string to write to file>", true, nullptr, onWrite},
                                             {"mv", "Rename file", "<original filename> <new filename>", true, nullptr, onMv},
                                             {"cp", "Copy file", "<source filename> <destination filename>", true, nullptr, onCp},
                                             {"cat", "Cat file", "<filename> file to read", true, nullptr,  onCat},
                                             {"status", "Get system status", "Get system status", false, nullptr, onStatus},
                                             {"cfgInit", "init config", "init config", false, nullptr, onSetupCfg},
                                             {"cfgPrint", "print config", "print config", false, nullptr, onPrintCfg}};

const size_t bindingCount = sizeof(bindings) / sizeof(bindings[0]);



void onSetupCfg(EmbeddedCli* cli, char* args, void* context)
{
  flockCfg.buildDefualtConfig();
}

void onPrintCfg(EmbeddedCli* cli, char* args, void* context)
{
  flockCfg.outputJson();
}



void onCommand(EmbeddedCli *embeddedCli, CliCommand *command)
{
  Serial.printf("%sCommand not found: '%s'%s.  Use 'help' for command list.\r\n",
      CLI_BOLD_RED, command->name, CLI_RESET);
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
          Serial.printf(CLI_YEL "GPS fix quality is " CLI_BOLD_GRN "%s" CLI_RESET "\r\n", goodFix ? "good" : "bad");
        }  break;

        case 'p':
        {
          Serial.printf(CLI_YEL "GPS current coordinates are " CLI_BOLD_GRN "%02.5f, %03.5f\r\n" CLI_RESET, 
              gps.getLatitude(), gps.getLongitude());
        }  break;

        case 't':
        {
          struct tm tm_;
          gps.getTime(&tm_);

          Serial.printf(CLI_YEL "GPS current time/date (gmt) is " CLI_BOLD_GRN "%02d:%02d:%02d %d/%d/%d\r\n" CLI_RESET, 
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

void onFSinfo(EmbeddedCli *cli, char *args, void *context)
{
  size_t cap;
  size_t used;
  flockfs.getInfo(&cap, &used);
  Serial.printf(CLI_YEL "Total file system size " CLI_BOLD_GRN "%d" CLI_YEL " bytes, " 
                CLI_BOLD_GRN "%d" CLI_YEL " used (" CLI_BOLD_GRN "%d" CLI_YEL " KiB free)\r\n" CLI_RESET,
            cap, used, (cap - used) / 1024);

}

void onLs(EmbeddedCli *cli, char *args, void *context)
{
  Serial.printf(CLI_YEL "%s" CLI_RESET, flockfs.list());
}

void onWrite(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fname = embeddedCliGetToken(args, 1);
    size_t len = flockfs.writeFile(fname, (uint8_t*)embeddedCliGetToken(args, 2), strlen(embeddedCliGetToken(args, 2)));

    Serial.printf(CLI_YEL "Wrote " CLI_BOLD_GRN "%d" CLI_YEL " bytes to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, len, fname);
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename or data. " CLI_YEL "Usage: write <filename> <string data to write>\r\n" CLI_RESET);
  }
}

void onDel(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 1)
  {
    const char* fname = embeddedCliGetToken(args, 1);
    if (flockfs.deleteFile(fname))
    {
      Serial.printf(CLI_YEL "Deleted " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fname);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to delete " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fname);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename to delete.\r\n" CLI_RESET);
  }
}

void onMv(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fromName = embeddedCliGetToken(args, 1);
    const char* toName = embeddedCliGetToken(args, 2);

    if (flockfs.renameFile(fromName, toName))
    {
      Serial.printf(CLI_YEL "Renamed " CLI_BOLD_GRN "%s" CLI_YEL " to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName, toName);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to rename " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename(s). " CLI_YEL "Usage: rm <old file name> <new file name>\r\n" CLI_RESET);
  }
}

void onCp(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fromName = embeddedCliGetToken(args, 1);
    const char* toName = embeddedCliGetToken(args, 2);

    if (flockfs.copyFile(fromName, toName))
    {
      Serial.printf(CLI_YEL "Copied " CLI_BOLD_GRN "%s" CLI_YEL " to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName, toName);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to copy " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename(s). " CLI_YEL "Usage: cp <source file name> <destination file name>\r\n" CLI_RESET);
  }
}
void onCat(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) == 1)
  {
    const char* fname = embeddedCliGetToken(args, 1);

    size_t fileLen = flockfs.getFileSize(fname);
    if (fileLen == -1)
    {
      Serial.printf("Unable to open %s\r\n", fname);
      return;
    }

    char* buf = (char*)ps_malloc(fileLen + 1);
    memset(buf, 0, fileLen + 1);
    
    if (buf)
    {
      size_t read = flockfs.readFile(fname, (uint8_t*)buf, fileLen);
      Serial.printf(CLI_CYA);
      Serial.printf("%s", (char*)buf);
      Serial.printf(CLI_RESET "\r\n");

      free(buf);
    }
    else
    {
      Serial.printf("DID NOT ALLOCATE PSRAM\r\n");
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename to cat.\r\n" CLI_RESET);
  }
}

void onReset(EmbeddedCli *cli, char *args, void *context)
{
  SPIFFS.end();
  delay(1000);

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

void onStatus(EmbeddedCli* cli, char* args, void* context)
{
  Serial.printf(CLI_CYA "->GPS:\r\n" CLI_RESET);
  if (gps.getFixQuality() > 0)
  {
    struct tm tm_;
    gps.getTime(&tm_);
    Serial.printf(CLI_YEL "\tGPS current coordinates are " CLI_BOLD_GRN "%02.5f, %03.5f\r\n" CLI_RESET, 
              gps.getLatitude(), gps.getLongitude());
    
    Serial.printf(CLI_YEL "\tGPS current time/date (gmt) is " CLI_BOLD_GRN "%02d:%02d:%02d %d/%d/%d\r\n" CLI_RESET, 
              tm_.tm_hour, tm_.tm_min, tm_.tm_sec,
              tm_.tm_mon, tm_.tm_mday, tm_.tm_year + 1900);  
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "\tNo GPS position fix\r\n" CLI_RESET);
  }

  Serial.printf(CLI_CYA "->Filesystem:\r\n\t" CLI_RESET);
  onFSinfo(NULL, NULL, NULL);

  Serial.printf(CLI_CYA "->Memories:\r\n" CLI_RESET);
  Serial.printf(CLI_YEL "\tInternal total heap " CLI_BOLD_GRN "%d" CLI_YEL " bytes, " CLI_BOLD_GRN "%d" 
                CLI_YEL " used (" CLI_BOLD_GRN "%d" CLI_YEL " KiB free)\r\n" CLI_RESET,
                ESP.getHeapSize(), (ESP.getHeapSize() - ESP.getFreeHeap()), ESP.getFreeHeap() / 1024);
  Serial.printf(CLI_YEL "\tPSRAM total heap " CLI_BOLD_GRN "%d" CLI_YEL " bytes, " CLI_BOLD_GRN "%d"
                CLI_YEL " used (" CLI_BOLD_GRN "%d" CLI_YEL " KiB free)\r\n" CLI_RESET,
                ESP.getPsramSize(), (ESP.getPsramSize() - ESP.getFreePsram()), ESP.getPsramSize() / 1024);
}

#endif // CLI_IMPL_H_
