#ifndef MBFS_H_
#define MBFS_H_

#include <vector>

#include "LittleFS.h"

class MBFS
{
public:
  MBFS() {}
  ~MBFS() {}

  bool begin();

  void getInfo(size_t* cap, size_t* used);

  size_t list(std::vector<const char*>& files);
  ssize_t readFile(const char* path, uint8_t* buff, size_t len);
  ssize_t writeFile(const char* path, const uint8_t* buff, size_t len);
  ssize_t appendFile(const char* path, const uint8_t* buff, size_t len);
  ssize_t getFileSize(const char* path);
  bool renameFile(const char* src, const char* dst);
  bool copyFile(const char* src, const char* dst);
  bool deleteFile(const char* path);
  bool fileExists(const char* path);

private:
};


#endif // MBFS_H_