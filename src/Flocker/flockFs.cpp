#include "flockFs.h"
#include "cstring"
#include <esp_psram.h>

#include "globals.h"

bool MBFS::begin()
{
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.printf("Could not init file system\r\n");
    return (false);
  }

  return (true);
}

size_t MBFS::list(std::vector<const char*>& files)
{
    File root = LittleFS.open("/");
    File file = root.openNextFile();

    files.clear();
    
    while(file)
    {
        files.push_back(file.name());
        file = root.openNextFile();
    }
    return (files.size());
}

bool MBFS::fileExists(const char* path)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);

    // try to open the file for reading, do not create if it doesnt exist
    File file = LittleFS.open(fpath, "r", false);

    if (file)
    {
        file.close();
        return (true);
    }
    return (false);
}

void MBFS::getInfo(size_t* cap, size_t* used)
{
    *cap = LittleFS.totalBytes();
    *used = LittleFS.usedBytes();
}

ssize_t MBFS::readFile(const char* path, uint8_t* buff, size_t len)
{    
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = LittleFS.open(fpath);

    if(!file || file.isDirectory())
    {
        return (-1);
    }

    ssize_t read = file.read(buff, len);

    file.close();
    return (read);  
}

ssize_t MBFS::writeFile(const char* path, const uint8_t* buff, size_t len)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = LittleFS.open(fpath, FILE_WRITE);

    if(!file || file.isDirectory())
    {
        Serial.println("- failed to open file for writing");
        return (-1);
    }

    ssize_t written = file.write(buff, len);

    file.close(); 
    return (written); 
}

ssize_t MBFS::appendFile(const char* path, const uint8_t* buff, size_t len)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = LittleFS.open(path, FILE_APPEND);

    if(!file || file.isDirectory())
    {
        return (-1);
    }

    ssize_t written = file.write(buff, len);

    file.close();
    return (written);
}

ssize_t MBFS::getFileSize(const char* path)
{
    char fpath[65] = {0};
    snprintf(fpath, 64, "/%s", path);
    File file = LittleFS.open(fpath);

    if(!file || file.isDirectory())
    {
        return (-1);
    }

    ssize_t ret = file.size();

    file.close();
    return (ret);
}

bool MBFS::renameFile(const char* src, const char* dst)
{
    char spath[65] = {0};
    char dpath[65] = {0};

    snprintf(spath, 64, "/%s", src);
    snprintf(dpath, 64, "/%s", dst);

    if (LittleFS.rename(spath, dpath))
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

    ssize_t copyLen = this->getFileSize(src);
    if (copyLen <= 0)
    {
        return (ret);
    }

    uint8_t* buf = (uint8_t*)ps_malloc(copyLen);
    if (buf)
    {
        ssize_t read = this->readFile(src, buf, copyLen);
        if (read != copyLen)
        {
            Serial.printf("copyFile() error - bad read\r\n");
        }
        else
        {
            ssize_t written = this->writeFile(dst, buf, copyLen);
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

    if(LittleFS.remove(fpath))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}