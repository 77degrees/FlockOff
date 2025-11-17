#include "mbfs.h"
#include "cstring"
#include <esp_psram.h>

#define LISTING_LEN 2048

bool MBFS::begin()
{
  if (!SPIFFS.begin(true))
  {
    Serial.printf("Could not init file system\r\n");
    return (false);
  }

  return (true);
}

const char* MBFS::list()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    char* listing = (char*)ps_malloc(LISTING_LEN + 1);
    size_t len = LISTING_LEN;
    
    if (listing)
    {
        memset(listing, 0, len + 1);

        while(file)
        {
            char line[64] = {0};
            snprintf(line, 63, "%8d %s\r\n", file.size(), file.name());
            strncat(listing, line, len);
            len -= strlen(listing);
            file = root.openNextFile();
        }
        free (listing);
    }
    else
    {
        return ("DID NOT ALLOCATE LISTING BUFFER\r\n");
    }

    return (listing);
}

void MBFS::getInfo(size_t* cap, size_t* used)
{
    *cap = SPIFFS.totalBytes();
    *used = SPIFFS.usedBytes();
}

size_t MBFS::readFile(const char* path, uint8_t* buff, size_t len)
{    
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = SPIFFS.open(fpath);
    if(!file || file.isDirectory())
    {
        return (-1);
    }

    size_t read = file.read(buff, len);

    file.close();
    return (read);  
}

size_t MBFS::writeFile(const char* path, const uint8_t* buff, size_t len)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = SPIFFS.open(fpath, FILE_WRITE);
    if(!file)
    {
        Serial.println("- failed to open file for writing");
        return (false);
    }

    size_t written = file.write(buff, len);

    file.close(); 
    return (written); 
}

size_t MBFS::appendFile(const char* path, const uint8_t* buff, size_t len)
{
    File file = SPIFFS.open(path, FILE_APPEND);
    if(!file)
    {
        return (-1);
    }

    size_t written = file.write(buff, len);

    file.close();
    return (written);
}

size_t MBFS::getFileSize(const char* path)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = SPIFFS.open(fpath);
    if (!file)
    {
        return (-1);
    }

    size_t ret = file.size();
    file.close();
    return (ret);
}

bool MBFS::renameFile(const char* src, const char* dst)
{
    char spath[65] = {0};
    char dpath[65] = {0};

    snprintf(spath, 64, "/%s", src);
    snprintf(dpath, 64, "/%s", dst);

    if (SPIFFS.rename(spath, dpath))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}


bool MBFS::copyFile(const char* src, const char* dst)
{
    bool ret = false;
    char spath[65] = {0};
    char dpath[65] = {0};

    snprintf(spath, 64, "/%s", src);
    snprintf(dpath, 64, "/%s", dst);

    size_t copyLen = this->getFileSize(src);
    if (copyLen == -1)
    {
        return (ret);
    }

    uint8_t* buf = (uint8_t*)ps_malloc(copyLen);
    if (buf)
    {
        size_t read = this->readFile(src, buf, copyLen);
        if (read != copyLen)
        {
            Serial.printf("copyFile() error - bad read\r\n");
        }
        else
        {
            size_t written = this->writeFile(dst, buf, copyLen);
            if (written != copyLen)
            {
                Serial.printf("copyFile() error - bad write\r\n");
            }
            else
            {
                ret = true;
            }
        }
        free (buf);
    }
    else
    {
        Serial.printf("copyFile() error - unable to allocate %d bytes for copy\r\n", copyLen);
    }

    return (ret);
}

bool MBFS::deleteFile(const char* path)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);

    if(SPIFFS.remove(fpath))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}