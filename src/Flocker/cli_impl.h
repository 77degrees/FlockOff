#ifndef CLI_IMPL_H_
#define CLI_IMPL_H_

#define CLI_BUFFER_SIZE_PS 8192
#define CLI_RX_BUFFER_SIZE_PS 1024
#define CLI_CMD_BUFFER_SIZE_PS 1024
#define CLI_HISTORY_SIZE_PS 64
#define CLI_BINDING_COUNT_PS 20

#define CLI_ERROR(x)  CLI_BOLD_RED x CLI_RESET

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);
void writeChar(EmbeddedCli *embeddedCli, char c);

void onSurvey(EmbeddedCli* cli, char* args, void* context);
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
void onTimeZone(EmbeddedCli* cli, char* args, void* context);

const struct CliCommandBinding bindings[] = {{"survey", "Perform Wifi survey", "survey [interval] - channel hop interval in milliseconds", true, nullptr, onSurvey},
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
                                             {"timezone", "Set time zone", "Set time zone, updates config structs", false, nullptr, onTimeZone}};

const size_t bindingCount = sizeof(bindings) / sizeof(bindings[0]);

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command)
{
  Serial.printf("%sCommand not found: '%s'%s.  Use 'help' for command list.\r\n",
      CLI_BOLD_RED, command->name, CLI_RESET);
}

void writeChar(EmbeddedCli *embeddedCli, char c)
{
  Serial.write(c);
}

void onSurvey(EmbeddedCli* cli, char* args, void* context)
{ 
  uint32_t timing = 1000; 
  if (embeddedCliGetTokenCount(args) == 1)
  {
    timing = atoi(embeddedCliGetToken(args, 1));
  }

  flockScan.survey(timing);
}

void onClear(EmbeddedCli *cli, char *args, void *context)
{
    Serial.printf(CLI_CLEAR);
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
  std::vector<const char*>files;
  size_t count = flockfs.list(files);

  Serial.printf(CLI_CYA "Filesystem is holding %d files:\r\n" CLI_RESET, count);

  for (std::vector<const char*>::iterator it = files.begin(); it != files.end(); ++it)
  {
    char f[128] = {0};
    strncpy(f, *it, 127);
    Serial.printf(CLI_GRN "  %s\r\n" CLI_RESET, *it);
  }
}

void onWrite(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fname = embeddedCliGetToken(args, 1);
    ssize_t len = flockfs.writeFile(fname, (uint8_t*)embeddedCliGetToken(args, 2), strlen(embeddedCliGetToken(args, 2)));

    if (len == -1)
    {
      Serial.printf(CLI_BOLD_RED "Error writing to %s\r\n" CLI_RESET, fname);
    }
    else
    {
      Serial.printf(CLI_YEL "Wrote " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " bytes to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, len, fname);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename or data. " CLI_RESET " " CLI_YEL "Usage: write <filename> <string data to write>\r\n" CLI_RESET);
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

    ssize_t fileLen = flockfs.getFileSize(fname);
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
  LittleFS.end();
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
  Serial.printf(CLI_YEL "\tInternal total heap " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " bytes, " CLI_BOLD_GRN "%d" 
                CLI_RESET " " CLI_YEL " used (" CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " KiB free)\r\n" CLI_RESET,
                ESP.getHeapSize(), (ESP.getHeapSize() - ESP.getFreeHeap()), ESP.getFreeHeap() / 1024);
  Serial.printf(CLI_YEL "\tPSRAM total heap " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " bytes, " CLI_BOLD_GRN "%d"
                CLI_RESET " " CLI_YEL " used (" CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " KiB free)\r\n" CLI_RESET,
                ESP.getPsramSize(), (ESP.getPsramSize() - ESP.getFreePsram()), ESP.getPsramSize() / 1024);

  Serial.printf(CLI_CYA "->Wall clock:\r\n" CLI_RESET);
  char tstring[64] = {0};

  time_t t = time(NULL);
  tm *tmp;
  tmp = localtime(&t);

  strftime(tstring, 63, "%a, %d %b %Y %T %z", tmp);
  Serial.printf(CLI_YEL "\tCurrent wall clock time is " CLI_BOLD_GRN "%s\r\n" CLI_RESET, tstring);
  Serial.printf(CLI_YEL "\tTimezone is set to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, flockCfg.getTimeZone());  
}

void onTimeZone(EmbeddedCli* cli, char* args, void* context)
{
  flockCfg.selectTimeZone();
}

#endif // CLI_IMPL_H_
