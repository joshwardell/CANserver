#include "SPIFFileSystem.h"

#include <SPIFFS.h>


namespace CANServer
{
    namespace SPIFFileSystem
    {
        void setup()
        {
            //Spin up access to the file system
            SPIFFS.begin();

#ifdef jjjkljk
            //Lets sort out our settings files
            if(!SPIFFS.exists(SETTINGSFILE)) 
            {
                //File didn't exist (or there was an error opening it)
                //Lets create a new one and put some defaults into it.
                File file = SPIFFS.open(SETTINGSFILE, FILE_WRITE);
                if (!file) 
                {
                    //Another check to see if we were able to write the file...  If we arn't thats bad stuff
                }
                else
                {
                    file.println("\
        {\n\
            \"disp0\": {\n\
                \"mode\":0,\n\
                \"string\":\"\" \n\
            },\n\
            \"disp2\": {\n\
                \"mode\":0,\n\
                \"string\":\"\" \n\
            },\n\
            \"disp3\": {\n\
                \"mode\":0,\n\
                \"string\":\"\" \n\
            }\n\
        }");
                    file.close();
                }
            }
            
            //By now the settings file should either have been just created or already exist.  Lets load it
            File file = SPIFFS.open(SETTINGSFILE, FILE_READ);
            if (file)
            {
                //TODO: load the settings

                file.close();
            }
            else
            {
                //There was a problem opening the settings file.  Thats not good.
                Serial.println("Error opening settings file");

                //Default our settings to somthing sane
            }
#endif
        }
    }
}