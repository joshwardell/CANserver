## CANserver development TODO list

### ESP32 firmware
* Dual bus support and detection (WIP)
* Light SD card file browser
* Backup of settings to SD if present
* Incrementing log filename (or better yet date stamp)
* Support pushbutton POST to trigger events (like log start/stop, or display toggle)
* Nicer setup and config of LEDs (Blink for BSD, blinkers, display warnings) and Relay (toggle off delay after XYZ)
* Use of bargraphs instead of LEDs (for same reasons as above)
* Show on-display alternatives if there are no LEDs (like we do for BSD)
* Auto connect to LAN NAS or cloud server to dump logs etc
* Support for many more vehicles with CAN (S, X, even Hondas...)
* Better alphanumeric display and text mode integration/config

### Phone apps
* Set up CAN signals and displays from a phone app
* Update firmware from an app

### Desktop/PI tools
* PC or phone tool to choose signals from any DBC file and send over their config
* Integration with something like Teslogger to cloud process that data
* Software to process and graph logs easily (or maybe slight update to CANbusAnalyzer) with separation of messages per bus

### Documentation/web
* Document popular display configurations
* Document popular Tesla CAN signal config
* Document popular signals and which bus they are on
* Cloud hosting and sharing of display configs

### Completed

* Customization of CAN messages and signals - grab string from serial or web and convert to message ID, bits, scale, units, etc.
* Customization of each display - assingning signals, format, etc
* Saving these customizations to onboard eeprom
* A simple web page served to enter these configurations - pull up 1.4 on your phone and fill in a few fields
* Logically standardize this to be handled by external phone or PC tools in the future over wifi
* Dump CAN logs to SD Card, if configured, triggered by pushbutton etc
* Reprogram over Wifi
* Auto-connect to home wifi
* Passing data over wifi to phone apps like TesLax and ScanMyTesla as a much higher bandwidth alternative to bluetooth OBD tools (on multiple busses from multiple canserves while I dream)
* Long term time and signal filtered contiuous can logs...think Teslafi with thousands of signals
* Auto-connect to home wifi and server to dump logs etc
* Integration with something like Teslogger to cloud process that data
