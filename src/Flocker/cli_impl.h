#ifndef CLI_IMPL_H_
#define CLI_IMPL_H_

#define CLI_BUFFER_SIZE_PS 8192
#define CLI_RX_BUFFER_SIZE_PS 1024
#define CLI_CMD_BUFFER_SIZE_PS 1024
#define CLI_HISTORY_SIZE_PS 64
#define CLI_BINDING_COUNT_PS 20

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);
void writeChar(EmbeddedCli *embeddedCli, char c);

/********************************************************
* Command line handler callback functions:
********************************************************/
// perform a manual device wireless survey
void onSurvey(EmbeddedCli* cli, char* args, void* context);
const char* helpSurvey = "survey [OPTION]\r\n"
                         "Perform a WiFi and/or Bluetooth survey.  Displays all results, not just those that match scan criteria.\r\n\r\n"
                         "  -i <INTERVAL>   interval in milliseconds to scan each WiFi channel\r\n"
                         "  -w              scan WiFi for broadcasters\r\n"
                         "  -b              scan for Bluetooth broadcasters\r\n"
                         "  -f <FILENAME>   save results to FILENAME\r\n"
                         "  -j <NOTES>      save results as JSON, with NOTES added for reference\r\n\r\n"
                         "If neither the -b or -w parameter is set, a survey of both will be performed.  If data is saved to file, but the -j parameter is not supplied, the data will be in CSV format.\r\n\r\n";


// clear terminal
void onClear(EmbeddedCli* cli, char* args, void* context);
// reset device (close filesystem first)
void onReset(EmbeddedCli* cli, char* args, void* context);
// filesystem utils; ls, rm, mv, cp, cat
void onLs(EmbeddedCli* cli, char* args, void* context);
void onDel(EmbeddedCli* cli, char* args, void* context);
void onMv(EmbeddedCli* cli, char* args, void* context);
void onCp(EmbeddedCli* cli, char* args, void* context);
void onCat(EmbeddedCli* cli, char* args, void* context);
// write test file
void onWrite(EmbeddedCli* cli, char* args, void* context);
// get system status
void onStatus(EmbeddedCli* cli, char* args, void* context);
// interactive config stuff
void onConfig(EmbeddedCli* cli, char* args, void* context);
const char* helpConfig = "config [OPTION]\r\n"
                         "Set or display configuration parameters\r\n\r\n"
                         "  -l      display current configuration (dumps the raw JSON in pretty-print format)\r\n\r\n"
                         "If -l not given, show system configuration menu.";

/*******************************************************
* Binding struct for each command:
 struct CliCommandBinding {
    const char *name;       <--- command as entered on CLI
    const char *shortHelp;  <--- description shown with help command
    const char *help;       <--- detaild help shown with -h parameter on command
    bool tokenizeArgs;      <--- true if there are args
    void *context;          <--- data passed to handler if no args (not used here)
    void (*binding)(EmbeddedCli *cli, char *args, void *context);  <--- callback/command handler
  };
*******************************************************/
const struct CliCommandBinding bindings[] = {
  {"survey", "Perform Wifi survey", helpSurvey, true, nullptr, onSurvey},
  {"clear", "Clear the console", "Clear the serial console screen", false, nullptr, onClear},
  {"reset", "Reboot the device", "Closes filesystems and resets board", false, nullptr, onReset},
  {"ls", "List files", "List files in filesystem", false, nullptr, onLs},
  {"rm", "Delete file", "<filename> to delete", true, nullptr, onDel},
  {"write", "Write test file", "-f <filename> -d <string to write to file>", true, nullptr, onWrite},
  {"mv", "Rename file", "<original filename> <new filename>", true, nullptr, onMv},
  {"cp", "Copy file", "<source filename> <destination filename>", true, nullptr, onCp},
  {"cat", "Cat file", "<filename> file to read", true, nullptr,  onCat},
  {"status", "Get system status", "Get system status", false, nullptr, onStatus},
  {"config", "Get/Set config values", helpConfig, true, nullptr, onConfig}};

const size_t bindingCount = sizeof(bindings) / sizeof(bindings[0]);

/******************************************************
* onCommand()
*******************************************************
* Command handler for unknown commands.  Just output
* an error message and remind user of 'help'
******************************************************/
void onCommand(EmbeddedCli *embeddedCli, CliCommand *command)
{
  Serial.printf("%sCommand not found: '%s'%s.  Use 'help' for command list.\r\n",
      CLI_BOLD_RED, command->name, CLI_RESET);

  flockLog.addLogLine("CLI", "Unknown command >%s<\r\n", command->name);
}

/******************************************************
* writeChar()
*******************************************************
* callback for CLI loop to know how to send chars back
* to user.  Serial, in this case
******************************************************/
void writeChar(EmbeddedCli *embeddedCli, char c)
{
  Serial.write(c);
}

void onConfig(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 1)
  {
    const char* cmd = embeddedCliGetToken(args, 1);
    if (!strcmp("-l", cmd))
    {
      flockCfg.outputJson();
    }
  }
  else
  {
    flockCfg.setConfigValues();
  }
}

/******************************************************
* onSurvey()
*******************************************************
* Command to perform a single scan for wireless devices
* both WiFi and BLE.  Note that this will populate a
* results set with EVERYTHING seen, not just known
* Flock/survellance devices
*
* Parameters:
*   -h - show help
*   -b - do Bluetooth LE scan
*   -w - do WiFi scan
*   -i <interval> - how much time (in ms) to spend on
*                   each WiFi channel and on BLE
*   -f <filename> - save results to specified filename
*   -j <notes>    - save file as JSON (requires -f paramter)
*
******************************************************/
void onSurvey(EmbeddedCli* cli, char* args, void* context)
{ 
  bool paramErr = false;
  bool doFile = false;
  bool doJson = false;
  bool doBT = false;
  bool doWiFi = false;
  uint32_t interval = 1000; 
  char fname[64] = {0};
  char notes[128] = {0};
  size_t argc = embeddedCliGetTokenCount(args);

  if (argc > 0)
  {
    for (size_t ii = 1; ii <= argc; ++ii)
    {
      const char* argv = embeddedCliGetToken(args, ii);

      if (strlen(argv) > 1)
      {
        if (argv[0] == '-')
        {
          if (argv[1] == 'b')
          {
            doBT = true;
          }
          else if (argv[1] == 'w')
          {
            doWiFi = true;
          }
          else if (argv[1] == 'j')
          {
            doJson = true;          
            ++ii;
            if (ii <= argc)
            {
              argv = embeddedCliGetToken(args, ii);
              if (argv[0] == '-')
              {
                paramErr = true;
                break;
              }
              strncpy(notes, embeddedCliGetToken(args, ii), 127);
            }
            else
            {
              paramErr = true;
              break;
            }
          } // extracting JSON (and notes)
          else if (argv[1] == 'i')
          {
            ++ii;
            if (ii <= argc)
            {
              argv = embeddedCliGetToken(args, ii);
              if (argv[0] == '-')
              {
                paramErr = true;
                break;
              }
              interval = atoi(argv);
            }
            else
            {
              paramErr = true;
              break;
            }
          } // extracting interval
          else if (argv[1] == 'f')
          {
            ++ii;
            if (ii <= argc)
            {
              argv = embeddedCliGetToken(args, ii);
              if (argv[0] == '-')
              {
                paramErr = true;
                break;
              }
              strncpy(fname, embeddedCliGetToken(args, ii), 63);
            }
            else
            {
              paramErr = true;
              break;
            }
          } // extracting filename
          else
          {
            paramErr = true;
            break;
          }
        }
      }
      else
      {
        paramErr = true;
        break;
      }
    }
  }
  else
  {
    paramErr = true;
  }

  if (paramErr)
  {
    Serial.printf(CLI_BOLD_RED "Bad parameter." CLI_YEL "Usage: survey [-i <interval>] [-f <filename>] [-j <notes>].\r\n" CLI_RESET);
    return;
  }

  if (!doBT && !doWiFi)
  {
    doBT = true;
    doWiFi = true;
  }

  flockLog.addLogLine("CLI", "onSurvey()\r\n");
  flockScan.survey(interval, doWiFi, doBT, fname, doJson, notes);
}

/******************************************************
* onClear()
*******************************************************
* Command to clear the user's terminal (by sending
* ASCII code for that)
*
* Parameters:
*   None
*
******************************************************/
void onClear(EmbeddedCli *cli, char *args, void *context)
{
  flockLog.addLogLine("CLI", "onClear()\r\n");
  Serial.printf(CLI_CLEAR);
}

/******************************************************
* onLs()
*******************************************************
* Command to list files in the filesystem
*
* Parameters:
*   None
*
******************************************************/
void onLs(EmbeddedCli *cli, char *args, void *context)
{
  std::vector<std::string>files;
  std::vector<std::string>::const_iterator filesCit;
  size_t count = flockfs.list(files);

  flockLog.addLogLine("CLI", "onLs()\r\n");

  for (filesCit = files.begin(); filesCit != files.end(); ++filesCit)
  {
    Serial.printf(CLI_GRN "\t%6d  %s\r\n" CLI_RESET, flockfs.getFileSize(filesCit->c_str()), filesCit->c_str());
  }
}

/******************************************************
* onWrite()
*******************************************************
* Command to create a test file in the filesystem
*
* Parameters (positional):
*   <filename> <string to write to file>
*
* Example:
*   write test.txt "abc def ghi"
*
******************************************************/
void onWrite(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fname = embeddedCliGetToken(args, 1);
    ssize_t len = flockfs.writeFile(fname, (uint8_t*)embeddedCliGetToken(args, 2), strlen(embeddedCliGetToken(args, 2)));

    if (len == -1)
    {
      Serial.printf(CLI_BOLD_RED "Error writing to %s\r\n" CLI_RESET, fname);
      flockLog.addLogLine("CLI", "onWrite() error writing to %s\r\n", fname);
    }
    else
    {
      Serial.printf(CLI_YEL "Wrote " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL " bytes to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, len, fname);
      flockLog.addLogLine("CLI", "onWrite() wrote %d bytes to %s\r\n", len, fname);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename or data. " CLI_RESET " " CLI_YEL "Usage: write <filename> <string data to write>\r\n" CLI_RESET);
    flockLog.addLogLine("CLI", "onWrite() bad invocation\r\n");
  }
}

/******************************************************
* onDel()
*******************************************************
* Command to delete a file from the filesystem
*
* Parameters:
*   <file_to_delete>
*
******************************************************/
void onDel(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 1)
  {
    const char* fname = embeddedCliGetToken(args, 1);
    if (flockfs.deleteFile(fname))
    {
      Serial.printf(CLI_YEL "Deleted " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fname);
      flockLog.addLogLine("CLI", "onDel() deleted %s\r\n", fname);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to delete " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fname);
      flockLog.addLogLine("CLI", "onDel() error deleting %s\r\n", fname);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename. " CLI_YEL "Usage: rm <filename>\r\n" CLI_RESET);
    flockLog.addLogLine("CLI", "onDel() invocation error\r\n");
  }
}

/******************************************************
* onMv()
*******************************************************
* Command to rename a file in the filesystem
*
* Parameters:
*   <original_name> <new_name>
*
******************************************************/
void onMv(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fromName = embeddedCliGetToken(args, 1);
    const char* toName = embeddedCliGetToken(args, 2);

    if (flockfs.renameFile(fromName, toName))
    {
      Serial.printf(CLI_YEL "Renamed " CLI_BOLD_GRN "%s" CLI_YEL " to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName, toName);
      flockLog.addLogLine("CLI", "onMv() moved %s to %s\r\n", fromName, toName);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to rename " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName);
      flockLog.addLogLine("CLI", "onMv() error in rename\r\n");
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename(s). " CLI_YEL "Usage: rm <old file name> <new file name>\r\n" CLI_RESET);
    flockLog.addLogLine("CLI", "onMv() invocation error\r\n");
  }
}

/******************************************************
* onCp()
*******************************************************
* Command to copy a file in the filesystem
*
* Parameters:
*   <original_name> <copied_name>
*
******************************************************/
void onCp(EmbeddedCli* cli, char* args, void* context)
{
  if (embeddedCliGetTokenCount(args) == 2)
  {
    const char* fromName = embeddedCliGetToken(args, 1);
    const char* toName = embeddedCliGetToken(args, 2);

    if (flockfs.copyFile(fromName, toName))
    {
      Serial.printf(CLI_YEL "Copied " CLI_BOLD_GRN "%s" CLI_YEL " to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName, toName);
      flockLog.addLogLine("CLI", "onCp() copied %s to %s\r\n", fromName, toName);
    }
    else
    {
      Serial.printf(CLI_BOLD_RED "Failed to copy " CLI_BOLD_GRN "%s\r\n" CLI_RESET, fromName);
      flockLog.addLogLine("CLI", "onCp() failed to copy moved %s to %s\r\n", fromName, toName);
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename(s). " CLI_YEL "Usage: cp <source file name> <destination file name>\r\n" CLI_RESET);
    flockLog.addLogLine("CLI", "onCp() invocation error\r\n");
  }
}

/******************************************************
* onCat()
*******************************************************
* Command to list a file's contents
*
* Parameters:
*   <filename>
*
******************************************************/
void onCat(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) == 1)
  {
    const char* fname = embeddedCliGetToken(args, 1);

    ssize_t fileLen = flockfs.getFileSize(fname);
    if (fileLen == -1)
    {
      Serial.printf(CLI_BOLD_RED "File not found.\r\n" CLI_RESET, fname);
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
      flockLog.addLogLine("CLI", "onCat() listed contents of %s (%d bytes)\r\n", fname, read);
    }
    else
    {
      Serial.printf("DID NOT ALLOCATE PSRAM\r\n");
      flockLog.addLogLine("CLI", "onCat() failed to allocate psram\r\n");
    }
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "Missing filename." CLI_YEL " Usage: cat <filename>\r\n" CLI_RESET);
    flockLog.addLogLine("CLI", "onMv() invocation error\r\n");
  }
}

/******************************************************
* onReset()
*******************************************************
* Command to reboot the system.  Will unmount filesytem
* before rebooting, so there is a slight delay
*
* Parameters:
*   None
*
******************************************************/
void onReset(EmbeddedCli *cli, char *args, void *context)
{
  flockLog.addLogLine("CLI", "onReset() shutting down!\r\n");
  flockLog.flushNow();
  LittleFS.end();
  delay(1000);

  ESP.restart();
}

/******************************************************
* onStatus()
*******************************************************
* Command to display system status.  This includes
*   GPS status
*   Filesystem status
*   Memory (heaps) status
*   Real time clock and timezone status
*
* Parameters:
*   None
*
******************************************************/
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
    
    Serial.printf(CLI_YEL "\tNumber of satellites currently tracked: " CLI_BOLD_GRN "%d\r\n" CLI_RESET, gps.getSatelliteCount());
  }
  else
  {
    Serial.printf(CLI_BOLD_RED "\tNo GPS position fix\r\n" CLI_RESET);
  }

  Serial.printf(CLI_CYA "->Filesystem:\r\n" CLI_RESET);
  size_t cap;
  size_t used;
  flockfs.getInfo(&cap, &used);
  Serial.printf(CLI_YEL "\tTotal capacity " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "bytes, " 
                CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "used (" CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "KiB free)\r\n" CLI_RESET,
                cap, used, (cap - used) / 1024);

  Serial.printf(CLI_CYA "->Memories:\r\n" CLI_RESET);
  Serial.printf(CLI_YEL "\tInternal total heap " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "bytes, " CLI_BOLD_GRN "%d" 
                CLI_RESET " " CLI_YEL "used (" CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "KiB free)\r\n" CLI_RESET,
                ESP.getHeapSize(), (ESP.getHeapSize() - ESP.getFreeHeap()), ESP.getFreeHeap() / 1024);
  Serial.printf(CLI_YEL "\tPSRAM total heap " CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "bytes, " CLI_BOLD_GRN "%d"
                CLI_RESET " " CLI_YEL "used (" CLI_BOLD_GRN "%d" CLI_RESET " " CLI_YEL "KiB free)\r\n" CLI_RESET,
                ESP.getPsramSize(), (ESP.getPsramSize() - ESP.getFreePsram()), ESP.getPsramSize() / 1024);

  Serial.printf(CLI_CYA "->Wall clock:\r\n" CLI_RESET);
  char tstring[64] = {0};

  time_t t = time(NULL);
  tm *tmp;
  tmp = localtime(&t);

  strftime(tstring, 63, "%a, %d %b %Y %T %z", tmp);
  Serial.printf(CLI_YEL "\tCurrent wall clock time is " CLI_BOLD_GRN "%s\r\n" CLI_RESET, tstring);
  Serial.printf(CLI_YEL "\tTimezone is set to " CLI_BOLD_GRN "%s\r\n" CLI_RESET, flockCfg.getTimeZone());

  flockLog.addLogLine("CLI", "onStatus()\r\n");
}


#endif // CLI_IMPL_H_
