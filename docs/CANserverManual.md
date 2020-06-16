## CANserver Manual

CANserver is intended to connect to a CAN network and transmit live data to a microDisplay, but has many more potential uses! It has a powerful ESP32 board with WiFi and Bluetooth and is Arduino programmable.

### Installation

CANserver should be connected to your CAN network with a supplied cable harness.

**Tesla powertrain bus**

Use the OBD harness to connect to your car's built-in (2020+) OBD connector, or you will need to buy a 3rd party OBD harness that plugs behind the center console. Note there are different harnesses for 2018 Model 3, and 2019+ Model Y and 3. 

    https://www.gpstrackingamerica.com/?s=tesla
    https://www.e-mobility-driving-solutions.com/produkt-kategorie/cable/?lang=en

(need to add common PT signals)

**Tesla chassis bus**

The CANserver is designed to plug and play easily under the passenger seat! Raise the seat to full height and look for a small box with a yellow connector. While the car is in Park, squeeze and pull to release the cable from the seat box. Plug it into the CANserver and plug the chassis pass-through cable into the seat box. Use the included double sized foam to mount the CANserver under the seat.

(add images, video, common signals)

**Setup of data**

In the futre hopefully you will able to easily choose what data to display on your microDisplays. Until then, it must be hardcoded into the Arduino program. (stay tuned!)

### Hardware overview
![CANserver Hardware](img/serverfeatures.jpg)

CANserver is based around a common ESP32 board that is powerful and easily programmed with Arduino.

There are three 12v+CAN connections--the left and bottom are hardwired for pass-through, and the top's 12v line is after the power switch. These are designed to support various CAN harnesses, and options should be available for OBDII, Tesla Chassis, DB9, or make your own using the standard SIP pins or XPH connectors.

After the power switch, there is a 7-30v to 5v switching regulator powering the rest of the board.

Next there is a CAN tranceiver, which is pulled high in hardware as shipped to disable transmission for customer confidence that this system can not negatively affect their vehicle. Enabling transmission can be dangerous and the user accepts all responsibility.

There are two configuration resistor pads (these will be jumpers in the future). RIO15 is used to configure as "CANServer2" SSID at startup so that two CANservers can be installed on two different CANbusses simultaneously. The second pad is for future use.

There is space to add your own pushbutton inputs, LED outputs, and I2C connector.


## Arduino programming and configuration

**Arduino Setup**

Download and install the Arduino desktop app from https://www.arduino.cc

The board uses the standard Node32S ESP32 Arduino board. 

In Arduino Preferences, add the following to your Board Manager URLs:

    https://dl.espressif.com/dl/package_esp32_index.json

Then under Board Manager, install *esp32 by Espressif Systems*

Under Boards, select Node32S

You may need to install the [Silabs USB drivers](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)

**Libraries**

The sketch requires several libraries to compile. Download the zip and install these libraries:

- ESPASyncWebServer from https://github.com/me-no-dev/ESPAsyncWebServer 
- ASyncTCP from https://github.com/me-no-dev/AsyncTCP
- esp32_can from https://github.com/collin80/esp32_can
- can_common from https://github.com/collin80/can_common

**Programming**

When uploading your program, once you see *Connecting...* **you must hold down the IO0 boot button** (to the right of the right of the USB cable) for ~2sec in order to start downloading.

![Uploading](img/uploadingbutton.png)

## PlatformIO over-the-air programming

The code can now also be compiled and uploaded using PlaformIO https://platformio.org

In your terminal, insall using

    brew install platformio
 
The first time you compile you must be internet connected so that it can install the needed libraries.
Afterwards, first power and connect your computer to the CANserver Wifi, then you can compile and upload new code

    platformio run --target upload --upload-port 192.168.4.1

**FAQ/etc**

I laid out the board to accept two common ESP32 modules and to use commonly available parts. A few notes:

- Make sure the ESP32 board is in the rightmost 19 pin sockets (the 20-pin sockets are common stock)
- A leg is purposely bent on the ESP32 board so it does not go into the socket. I found out the hard way that adding a 5v input there for the alternate board power is bad with 3.3V GPIO. (may add a jumper in the future rev)
- 5mm LEDs on v1 are pointless as one conflicts with serial TX, and the other already exists on the module. (may move to other IO in future rev)

Jun 8, 2020
