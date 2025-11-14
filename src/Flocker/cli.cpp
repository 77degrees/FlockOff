#include <Arduino.h>
#include <esp_psram.h>
#include "esp_task_wdt.h"

#include "cli.h"
#include "gps.h"
#include "mbfs.h"

extern NMEAGPS gps;

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"
#include "cli_impl.h"

static EmbeddedCli *cli = NULL;
static CLI_UINT* cliBuffer = NULL; //[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

bool setupCLI()
{
  EmbeddedCliConfig *config = embeddedCliDefaultConfig();
  bool usePSRAM = false;

  if (esp_psram_init() == ESP_OK)
  {
    Serial.printf("Found PSRAM, will use for CLI....\r\n");
    usePSRAM = true;

    cliBuffer = (CLI_UINT*)ps_malloc(BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE_PS));
    if (!cliBuffer)
    {
      Serial.printf("Failed to ps_malloc cliBuffer!\r\n");
      return (false);
    }

    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE_PS;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE_PS;
    config->cmdBufferSize = CLI_CMD_BUFFER_SIZE_PS;
    config->historyBufferSize = CLI_HISTORY_SIZE_PS;
    config->maxBindingCount = CLI_BINDING_COUNT_PS;    
  }
  else
  {

    Serial.printf("No PSRAM found, will use internal SRAM for CLI....\r\n");
    cliBuffer = (CLI_UINT*)malloc(BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE_PS));
    if (!cliBuffer)
    {
      Serial.printf("Failed to malloc cliBuffer!\r\n");
      return (false);
    }

    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    config->historyBufferSize = CLI_HISTORY_SIZE;
    config->maxBindingCount = CLI_BINDING_COUNT; 
  }

  cli = embeddedCliNew(config, usePSRAM);

  if (cli == NULL)
  {
      Serial.printf("Cli was not created. Check sizes!\r\n");
      return (false);
  }

  // add command bindings
  for (size_t ii = 0; ii < bindingCount; ++ii)
  {
    embeddedCliAddBinding(cli, bindings[ii]);
  }

  cli->onCommand = onCommand;
  cli->writeChar = writeChar;

  Serial.printf("\r\n");
  Serial.printf("   /$$$$$$$$ /$$                     /$$              /$$$$$$   /$$$$$$   /$$$$$$\r\n");
  Serial.printf("  | $$_____/| $$                    | $$             /$$__  $$ /$$__  $$ /$$__  $$\r\n");
  Serial.printf("  | $$      | $$  /$$$$$$   /$$$$$$$| $$   /$$      | $$  \\ $$| $$  \\__/| $$  \\__/\r\n");
  Serial.printf("  | $$$$$   | $$ /$$__  $$ /$$_____/| $$  /$$/      | $$  | $$| $$$$    | $$$$    \r\n");
  Serial.printf("  | $$__/   | $$| $$  \\ $$| $$      | $$$$$$/       | $$  | $$| $$_/    | $$_/    \r\n");
  Serial.printf("  | $$      | $$| $$  | $$| $$      | $$_  $$       | $$  | $$| $$      | $$      \r\n");
  Serial.printf("  | $$      | $$|  $$$$$$/|  $$$$$$$| $$ \\  $$      |  $$$$$$/| $$      | $$      \r\n");
  Serial.printf("  |__/      |__/ \\______/  \\_______/|__/  \\__/       \\______/ |__/      |__/      \r\n");

  Serial.printf("(c) 2025 M.Brugman\r\n\r\n\r\n");         
  Serial.printf("CLI Enabled.  'Help' for help\r\n");                                                                       

  return (true);
}

void updateCLI()
{
  while (Serial.available())
  {
     embeddedCliReceiveChar(cli, Serial.read());
  }

  embeddedCliProcess(cli);
}



