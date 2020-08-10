#ifndef __SPIFFILESYSTEM_H__
#define __SPIFFILESYSTEM_H__

#include <SPIFFS.h>

namespace CANServer
{
    namespace SPIFFileSystem
    {
        void setup();

        //We extend the basic spiffs to allow us to open multiple partitions
        class fs : public ::fs::SPIFFSFS
        {
        public:
            bool begin(const char* partitionName, bool formatOnFail=false, const char * basePath="/spiffs", uint8_t maxOpenFiles=10);
        };


        extern CANServer::SPIFFileSystem::fs SPIFFS_app;
        extern CANServer::SPIFFileSystem::fs SPIFFS_data;
    }
}

#endif