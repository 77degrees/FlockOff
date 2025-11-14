#ifndef MBFS_H_
#define MBFS_H_

#include <Arduino.h>
#include "SPIFFS.h"

bool initFS();

void listDir(const char* dir, uint8_t levels);
void readFile(const char * path);
void writeFile(const char * path, const char * message);
void appendFile(const char * path, const char * message);
void renameFile(const char * path1, const char * path2);
void deleteFile(const char * path);

#endif // MBFS_H_