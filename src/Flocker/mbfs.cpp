#include "mbfs.h"

bool initFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.printf("Could not init file system\r\n");
    return (false);
  }
  return (true);
}

void listDir(const char* dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);
    File root = SPIFFS.open(dirname);
    if(!root)
    {
        Serial.println("- failed to open directory");
        return;
    }

    if(!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    
    while(file)
    {
        if(file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels)
            {
                listDir(file.name(), levels -1);
            }
        } 
        else 
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(const char * path)
{
    Serial.printf("Reading file: %s\r\n", path);
    File file = SPIFFS.open(path);
    if(!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available())
    {
        Serial.write(file.read());
    }
    file.close();  
}

void writeFile(const char * path, const char * message)
{
    Serial.printf("Writing file: %s\r\n", path);
    File file = SPIFFS.open(path, FILE_WRITE);
    if(!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    if(file.print(message))
    {
        Serial.println("- file written");
    } 
    else 
    {
        Serial.println("- write failed");
    }

    file.close();  
}

void appendFile(const char * path, const char * message)
{
    Serial.printf("Appending to file: %s\r\n", path);
    File file = SPIFFS.open(path, FILE_APPEND);
    if(!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }

    if(file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }

    file.close();
}

void renameFile(const char * path1, const char * path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);

    if (SPIFFS.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void deleteFile(const char * path)
{
    Serial.printf("Deleting file: %s\r\n", path);

    if(SPIFFS.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}