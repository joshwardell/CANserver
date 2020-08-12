#include "SPIFFileSystem.h"

#include "vfs_api.h"

extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
}

namespace CANServer
{
    namespace SPIFFileSystem
    {
        void setup()
        {
            Serial.println("Setting up SPI FS...");

            //Spin up access to the file system
            SPIFFS_app.begin("spiffs1", false, "/a", 10);
            SPIFFS_data.begin("spiffs2", false, "/u", 10);
            Serial.println("Done");
        }


        

        using namespace CANServer::SPIFFileSystem;


        bool fs::begin(const char* partitionName, bool formatOnFail, const char * basePath, uint8_t maxOpenFiles)
        {
            Serial.printf("Mounting %s...\r\n", partitionName);

            if(esp_spiffs_mounted(partitionName)){
                log_w("SPIFFS Already Mounted!");
                return true;
            }

            esp_vfs_spiffs_conf_t conf = {
            .base_path = basePath,
            .partition_label = partitionName,
            .max_files = maxOpenFiles,
            .format_if_mount_failed = false
            };

            esp_err_t err = esp_vfs_spiffs_register(&conf);
            if(err == ESP_FAIL && formatOnFail){
                if(format()){
                    err = esp_vfs_spiffs_register(&conf);
                }
            }
            if(err != ESP_OK){
                log_e("Mounting SPIFFS failed! Error: %d", err);
                return false;
            }

            size_t total_bytes = 0;
            size_t used_bytes = 0;
            esp_spiffs_info(partitionName, &total_bytes, &used_bytes);
            Serial.printf("Total: %d B, Used: %d B\r\n", total_bytes, used_bytes);

            _impl->mountpoint(basePath);
            return true;
        }

        CANServer::SPIFFileSystem::fs SPIFFS_app;
        CANServer::SPIFFileSystem::fs SPIFFS_data;
    }
}