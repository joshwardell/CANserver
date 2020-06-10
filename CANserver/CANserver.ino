/*   CAN Server by Josh Wardell
     http://www.jwardell.com/canserver/
     To be used with microDisplay

     Jun 3 2020
     Jun 10 2020 edited by Chris Allemang to include CANSignalAnalysis library

     Board: Node32s
     (must press IO0 right button to start programming)
*/

#include "WiFi.h"   //esp32
#include "ESPAsyncWebServer.h" //https://github.com/me-no-dev/ESPAsyncWebServer
#include <esp32_can.h>  //RX GPIO16 TX GPIO 17 https://github.com/collin80/esp32_can
#include "CANSignalAnalysis.h" //Include library for analyzing CAN Signals

#define LED1 1    //shared with serial tx - try not to use
#define LED2 2    //onboard blue LED
#define CFG1 15   //jumper to toggle second CANserver
#define CFG2 4    //future

//Define CAN message IDs
#define HVBattAmpVoltID 0x132 //Message ID for battery voltage and current
#define RearTorqueID 0x1D8 //Message ID for rear motor torque
#define BMSThermalID 0x312 //Message ID for minimum battery temperature
#define VCFrontCoolantID 0x241 //Message ID for battery and power train coolant rates
#define BMSPowerAvailableID 0x252 //Message ID for max regen and max discharge rates
#define UISpeedID 0x257 //Message ID for vehicle speed
#define DASStatusID 0x399 //Message ID for blind spots

#define BITRATE 500000  //CAN bitrate, Tesla=500000

// access point network credentials - don't change these
const char* ssid = "CANserver";
const char* password = "JWcanServer2020";

int testcounter = 100;



static int BattVolts = 0;     //ID 132 byte 0+1 scale .01 V
static int BattAmps = 0;      //ID 132 byte 2+3 scale -.1 offset 0 A
static int BattPower = 0;         //V*A
static int RearTorque = 0;     //ID 1D8 startbit 24 signed13 scale 0.25 NM
static int FrontTorque = 0;     //ID 1D8 startbit 24 signed13 scale 0.25 NM
static int MinBattTemp = 0;   //ID 312 SB 44 u9 scale .25 offset -25 C
static int BattCoolantRate = 0;  //ID 241 SB 0 u9 scale .01 LPM
static int PTCoolantRate = 0;    //ID 241 SB 22 u9 scale .01 LPM
static int MaxRegen = 0;      //ID 252 Bytes 0+1 scale .01 kW
static int MaxDisChg = 0;     //ID 252 Bytes 2+3 scale .01 kW
static int VehSpeed = 0;     //ID 257 SB 12 u12 scale .08 offset -40 KPH
static int v12v261 = 0;          //ID 261 SB0 u12 scale 0.005444 V
static int BattCoolantTemp = 0;  //ID 321 SB0 u10 scale 0.125 offset -40 C
static int PTCoolantTemp = 0;    //ID 321 SB10 u10 scale 0.125 offset -40 C
static int BattRemainKWh = 0;    //ID 352 byte 1/2 scale .1 kWh
static int BattFullKWh = 0;      //ID 352 byte 0/1 scale .1 kWh
static int InvHStemp376 = 0;     //ID 376 Byte 1 scale .5 offset -20 C
static int BSR = 0;
static int BSL = 0;
static int brightness = 4;       //LED brightness


int disp0mode;
int disp1mode;
int disp2mode;
String disp0str;    //commands to send to first display
String disp1str;    //commands to send to second display
String disp2str;    //commands to send to third display

String inString = "";    // string to hold input
int serialinput;    // string to hold input
unsigned long previouscycle = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

CANSignalAnalysis analyzeCAN; //Create an object to analyze CAN signals

void setup() {

  //pinMode(LED1,OUTPUT); // LED1 shares TXpin with serial
  pinMode(LED2, OUTPUT);
  pinMode(CFG1, INPUT_PULLUP);
  pinMode(CFG2, INPUT_PULLUP);

  if (digitalRead(CFG1) == 0) { //If jumpered, server 2
    ssid = "CANserver2";
  }

  Serial.begin(57600);  //comment out serial to use LED1
  delay(200);
  Serial.println();

  CAN0.begin(BITRATE);

  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // set up servers for displays
  server.on("/disp0", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", disp0str);
  });
  server.on("/disp1", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", disp1str);
  });
  server.on("/disp2", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", disp2str);
  });

  //receive posts of display buttons, TODO do something with the buttons
  server.on("/post0", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.print("POST0: ");
    for (size_t i = 0; i < len; i++) {
      Serial.write(data[i]);
    }
    Serial.println();
    request->send(200);
  });
  server.on("/post1", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.print("POST1: ");
    for (size_t i = 0; i < len; i++) {
      Serial.write(data[i]);
    }
    Serial.println();
    request->send(200);
  });
  server.on("/post2", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.print("POST2: ");
    for (size_t i = 0; i < len; i++) {
      Serial.write(data[i]);
    }
    Serial.println();
    request->send(200);
  });
  // Start server
  server.begin();

  //  CAN0.watchFor(0x100, 0xF00); //setup a special filter
  CAN0.watchFor(); //then let everything else through anyway
  //  CAN0.setCallback(0, gotHundred); //callback on that first special filter
}

void loop() {
  if (Serial) {
    long currentMillis = millis();
    if (currentMillis - previouscycle >= 500) { //Every 500ms
      previouscycle = currentMillis;
      digitalWrite(LED2, !digitalRead(LED2)); //flash LED for awake
      //digitalWrite(LED1, !digitalRead(LED1)); //debug LED1
      Serial.print("BattAmps(a):"); //print data to serial for debug
      Serial.print(BattAmps);
      Serial.print(" BattVolts(b):");
      Serial.print(BattVolts);
      Serial.print(" BattPower(c):");
      Serial.print(BattPower);
      Serial.print(" RearTorque(d):");
      Serial.print(RearTorque);
      Serial.print(" MinBattTemp(e):");
      Serial.print(MinBattTemp);
      Serial.print(" MaxRegen(f):");
      Serial.print(MaxRegen);
      Serial.print(" MaxDisChg(g):");
      Serial.print(MaxDisChg);
      Serial.print(" VehSpeed(h):");
      Serial.print(VehSpeed);
      Serial.print(" BSR(i):");
      Serial.print(BSR);
      Serial.print(" BSL(j):");
      Serial.print(BSL);
      Serial.print(digitalRead(CFG1));
      Serial.print(digitalRead(CFG2));
      Serial.println();
    }
  }//if serial
  if (Serial.available() > 0) {
    int inChar = Serial.read();   //read sample data for debug
    switch (inChar) {
      case 'a':
        BattAmps = inString.toInt();
        inString = "";
        break;

      case 'b':
        BattVolts = inString.toInt();
        inString = "";
        break;

      case 'c':
        BattPower = inString.toInt();
        inString = "";
        break;

      case 'd':
        RearTorque = inString.toInt();
        inString = "";
        break;

      case 'e':
        MinBattTemp = inString.toInt();
        inString = "";
        break;

      case 'f':
        MaxRegen = inString.toInt();
        inString = "";
        break;

      case 'g':
        MaxDisChg = inString.toInt();
        inString = "";
        break;

      case 'h':
        VehSpeed = inString.toInt();
        inString = "";
        break;

      case 'i':
        BSR = inString.toInt();
        inString = "";
        break;

      case 'j':
        BSL = inString.toInt();
        inString = "";
        break;

      case '\n':
        serialinput = inString.toInt(); //save off input
        inString = "";
        break;

      default:
        inString += (char)inChar;
        break;
    }
  } //serial available

  //set up display commands from data, see microDisplay command reference
  disp0mode = 0;
  /////disp0str = "-901vFDu0m11l"; //for display test
  disp0str = String(BattPower) + "vWK  Bu" + String(int(0.008 * BattPower)) + "b" + disp0mode + "m" + "3s12345tI1l";
  disp1str = String(RearTorque) + "vMNu" + String(int(0.006 * RearTorque)) + "b" + "0m";
  disp2str = String(int(0.621371 * VehSpeed)) + "vHPMu" + String(int(VehSpeed / 20)) + "b0m  TEaST  d2x400r";

  CAN_FRAME message;
  if (CAN0.read(message)) {
    /*    Serial.print(message.id, HEX);  ///debug display RX message
        if (message.extended) Serial.print(" X ");
        else Serial.print(" S ");
        Serial.print(message.length, DEC);
        for (int i = 0; i < message.length; i++) {
          Serial.print(message.data.byte[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
    */
    digitalWrite(LED2, !digitalRead(LED2)); //flash LED2 to show data Rx
    switch (message.id)
    {
      case HVBattAmpVoltID: //Case for HVBattAmpVoltID
        BattVolts = analyzeCAN.batteryVoltage(message.length, BattVolts, message.data.byte[1], message.data.byte[0]); //Analyze the battery voltage signal
        BattAmps = analyzeCAN.batteryCurrent(message.length, BattAmps, message.data.byte[3], message.data.byte[3]); //Analyze the battery current signal
        BattPower = BattVolts * BattAmps / 100; //Calculate power from Volts and Amps
        break;

      case RearTorqueID: //Case for RearTorqueID
        RearTorque = analyzeCAN.rearTorque(message.length, RearTorque, message.data.byte[4], message.data.byte[3]); //analzye rear torque signal
        break;

      case BMSThermalID: //Case for BMSThermalID
        MinBattTemp = analyzeCAN.minBattTemp(message.length, MinBattTemp, message.data.byte[6], message.data.byte[5]); //analyze min batter temp signal
        break;
      /*
            case 0x224:
              DCDCoutput224 = ((message.data.byte[3] << 8) + message.data.byte[2]) & 0x07FF;
              break;
      */
      case VCFrontCoolantID: //Case for VCFrontCoolantID
        BattCoolantRate = analyzeCAN.battCoolantRate(message.length, BattCoolantRate, message.data.byte[1], message.data.byte[0]); //analyze battery coolant rate
        PTCoolantRate = analyzeCAN.PTCoolantRate(message.length, PTCoolantRate, message.data.byte[3], message.data.byte[2]); //analyze power train coolant rate
        break;

      /*      case 0x261:
              v12v261 = (((message.data.byte[1] & 0x0F) << 12) | (message.data.byte[0] << 4)) >> 4;  //ID 261 SB0 u12 scale 0.005444 V
              break;
      */
      case BMSPowerAvailableID: //Case for BMSPowerAvailableID
        MaxRegen = analyzeCAN.maxRegen(message.length, MaxRegen, message.data.byte[1], message.data.byte[0]); //analyze max regen signal
        MaxDisChg = analyzeCAN.maxDisChg(message.length, MaxDisChg, message.data.byte[3], message.data.byte[2]); //analyze max discharge signal
        break;

      case UISpeedID: //Case for UISpeedID
        VehSpeed = analyzeCAN.vehSpeed(message.length, VehSpeed, message.data.byte[2], message.data.byte[1]); //analyze vehicle speed signal
        break;

      case DASStatusID: //Case for DASStatusID
        BSR = analyzeCAN.rightBlindSpot(message.length, BSR, message.data.byte[0]); //analyze right blind spot signal
        BSL = analyzeCAN.leftBlindSpot(message.length, BSL, message.data.byte[0]); //analyze left blind spot signal 
        break;

      /*      ///////////
           case 0x292:
             SOCUI = (((message.data.byte[1] & 0x03) << 8) + message.data.byte[0]);
             SOCAVE = (((message.data.byte[4] << 8) + message.data.byte[3]) & 0xFFC0) >> 6;  //ID 292 SB30 u10 scale 0.1 %
             break;

           case 0x321:
             BattCoolantTemp = ((message.data.byte[1] & 0x03) << 8) + message.data.byte[0];  //ID 321 SB0 u10 scale 0.125 offset -40 C
             PTCoolantTemp = (((message.data.byte[2] << 8) + message.data.byte[1]) & 0x0FFC) >> 2;    //ID 321 SB10 u10 scale 0.125 offset -40 C
           break;

           case 0x352:
             BattFullKWh = ((message.data.byte[1] & 0x03) * 256) + message.data.byte[0];
             BattRemainKWh = ((message.data.byte[2] & 0x0F) * 64) + (message.data.byte[1] >> 2);
             //Serial.print(BattRemainKWh);
             //Serial.print(" ");
             break;

           case 0x315:
             InvHStemp376 = message.data.byte[1];
             break;

           /*     case 0x712:
                  if (message.data.byte[0] = 0); //index 0
                  {
                    CellTemp1 = message.data.byte[1];
                    CellTemp2 = message.data.byte[2];
                  }
                  break;
      */

      default:
        break;
    } //end can msg id case

  } //endif can rec msg

  // yield(); ///needed?
}
