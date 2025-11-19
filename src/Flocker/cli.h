#ifndef CLI_H_
#define CLI_H_

#define CLI_RESET "\x1b[0m"
#define CLI_CLEAR "\x1b[2J"

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

bool setupCLI();
void updateCLI();
void holdCLI(bool hold);

#endif  // CLI_H_