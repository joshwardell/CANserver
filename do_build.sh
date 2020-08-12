#!/bin/bash

mkdir -p build/
rm build/* &> /dev/null

RUN_ENVIRONMENT=""
BUILD_DIRECTORY="node32s"
if [ $# -gt 0 ]
  then
    RUN_ENVIRONMENT="-e ${1}"
    BUILD_DIRECTORY="${1}"
fi

~/.platformio/penv/bin/platformio run $RUN_ENVIRONMENT && \
~/.platformio/packages/tool-mkspiffs/mkspiffs_espressif32_arduino -c ui-data -p 256 -b 4096 -s 921600 .pio/build/$BUILD_DIRECTORY/spiffs.bin && \
~/.platformio/packages/tool-mkspiffs/mkspiffs_espressif32_arduino -c user-data -p 256 -b 4096 -s 49152 .pio/build/$BUILD_DIRECTORY/user_spiffs.bin


cp .pio/build/$BUILD_DIRECTORY/firmware.bin ./build/web-firmware.bin
cp .pio/build/$BUILD_DIRECTORY/spiffs.bin ./build/web-ui.bin
cp .pio/build/$BUILD_DIRECTORY/user_spiffs.bin ./build/user-data.bin

#merge all the bits togeather so we can create a single firmware image
touch fillbyte.bin
#First 4096 bytes are just FF - len 0x1000
python3 tools/padfiletolength.py fillbyte.bin 255 4096 > build/updateflash.bin

#Bootloader goes in at 0x1000 - len 0x7000
python3 tools/padfiletolength.py ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/bin/bootloader_dio_40m.bin 255 28672 >> build/updateflash.bin

#Partitions go in next at 0x8000 - len 0x1000
python3 tools/padfiletolength.py .pio/build/$BUILD_DIRECTORY/partitions.bin 255 4096 >> build/updateflash.bin

#some empty space that we aren't using because of the rejig of the layout for easier flashing
#We moved the NVS stuff to the end of flash (it used to take up this space)
python3 tools/padfiletolength.py fillbyte.bin 105 20480 >> build/updateflash.bin

#otadata is at 0xE000
python3 tools/padfiletolength.py ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 255 8192 >> build/updateflash.bin

#app in app_0 partition
python3 tools/padfiletolength.py ./build/web-firmware.bin 255 1572864 >> build/updateflash.bin

#nothing in app_1 parition
python3 tools/padfiletolength.py fillbyte.bin 255 1572864 >> build/updateflash.bin

#spiffs partition that contains the web ui
python3 tools/padfiletolength.py ./build/web-ui.bin 255 921600 >> build/updateflash.bin


#now create a flash image thats EVERYTHING (including overwriting of user modifiable scripts and user prefs)
cp ./build/updateflash.bin ./build/initialflash.bin
python3 tools/padfiletolength.py ./build/user-data.bin 255 49152 >> build/initialflash.bin
head -c 12288 /dev/zero >> build/initialflash.bin

#cleanup
rm fillbyte.bin
rm ./build/user-data.bin

