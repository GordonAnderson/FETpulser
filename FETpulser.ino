/*
 *  FETpulser
 *
 *  The FET pulser generates two complementary outputs with programmable voltage levels. These outputs are switched between ground and 
 *  the programmed output voltage, this switching is fast, about 10 nS rise and fall times. The intend is to use this to drive BN gates
 *  in mass spec applications. The system can be battery operated to allow floating this system at a high voltage. The system can be triggered
 *  using a BNC TTL level input or through a fiber optic cable to allow high voltage isolation.
 *  
 *  This application uses the ESP8266 to communicate with host computer to control setup parameters in the FET pulser. Additionally the 
 *  RS232 port can also be used for communcation and setup of the WiFi interface. The serial port baud rate is set to 115200.
 *
 *  As currently desiged this system uses DHCP is acquire an IP address. Multicast DNS is supported allow local network IP address
 *  lookup based on name. Note, this lookup is alwas lower case, the defualt host name is MPISnet, the local DNS lookup is mipsnet.local.
 *  
 *  This interface will also support operating as an access point. To use in this way do not connect, issue the AP command then the MDNS con=mmand
 *  and enter repeat mode. Then a remote client can connect
 *
 *  Gordon Anderson
 *
 */
#include <arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <FS.h>
#include "hardware.h"
#include "Serial.h"
#include "FETpulser.h"
#include "Errors.h"

extern "C" {
#include "user_interface.h"
}

char Version[] = "\nFETpulser Version 1.0, May 21, 2016";

FETpulserData FETPD;

FETpulserData FETPDdefault = 
                      { 50,-50,false,
                        0,285.71,0,
                        1,-285.71,0,
                        "FETpulser",
                        "pulser",
                        "FETpulser1234",
                        "AP",
                        123456,
                      };

MDNSResponder mdns;

ESP8266WiFiClass wifi;

WiFiServer server(2015);
WiFiClient client;
bool APmode = false;

void listNetworks()
{
  // scan for nearby networks:
  serial->println("");
  serial->println("** Scan Networks **");
  int numSsid = wifi.scanNetworks();
  if (numSsid == -1) {
    serial->println("Couldn't get a wifi connection");
    while (true);
  }
  // print the list of networks seen:
  serial->print("number of available networks:");
  serial->println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    serial->print(thisNet);
    serial->print(") ");
    serial->print(wifi.SSID(thisNet));
    serial->print("\tSignal: ");
    serial->print(wifi.RSSI(thisNet));
    serial->print(" dBm");
    serial->print("\tEncryption: ");
    printEncryptionType(wifi.encryptionType(thisNet));
  }
  SendACK;
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      serial->println("WEP");
      break;
    case ENC_TYPE_TKIP:
      serial->println("WPA");
      break;
    case ENC_TYPE_CCMP:
      serial->println("WPA2");
      break;
    case ENC_TYPE_NONE:
      serial->println("None");
      break;
    case ENC_TYPE_AUTO:
      serial->println("Auto");
      break;
  }
}

void AP(void)
{
  if(strlen(FETPD.password) == 0)
  {
    APmode = true;
    wifi.hostname(FETPD.host);
    wifi.softAP((const char*)FETPD.ssid);
  }
  else
  {
    APmode = true;
    wifi.hostname(FETPD.host);
    wifi.softAP((const char*)FETPD.ssid, (const char*)FETPD.password);
  }  
  mdns.addService("fetpulser", "tcp", 2015);
  server.begin();
  server.setNoDelay(true);
  wifi.mode(WIFI_AP);
  if (!mdns.begin(FETPD.host, wifi.localIP(), 1))
  {
    serial->println("Error setting up MDNS responder!");
  }
  else serial->println("mDNS responder started");
  SendACK;
}

void Connect(void)
{
  wifi.begin(FETPD.ssid, FETPD.password);
  wifi.hostname(FETPD.host);
  SendACK;
}

void Disconnect(void)
{
  SendACK;
  if (wifi.status() != WL_CONNECTED)
  {
    serial->println("Not connected to a network!");
    return;
  }
  wifi.disconnect();  
}

// Report WiFi status
void Status(void)
{
  SendACKonly;
  serial->println(wifi.status());
}

void ReportIP(void)
{
  SendACKonly;
  if(APmode) serial->println(wifi.softAPIP());
  else serial->println(wifi.localIP());
}

// Set the WiFi start mode, IDLE, AP, or CONNECT
void SetStartMode(char *res)
{
  if((strcmp("IDLE", res) == 0) || (strcmp("AP", res) == 0) || (strcmp("CONNECT", res) == 0))
  {
    strcpy(FETPD.StartMode,res);
    SendACK;
    return;
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;
}

bool SaveSettings(void)
{
  // open the file in write mode
  File f = SPIFFS.open("/config.dat", "w");
  if (f)
  {
    // now write FETpulser data structure to file
    f.write((const uint8_t *)&FETPD,sizeof(FETPD));
    f.close();
    return(true);
  }
  return(false);
}

bool RestoreSettings(void)
{
  File f = SPIFFS.open("/config.dat", "r");
  if (f) 
  {
    // Read the FEToulser data structure from the file
    f.read((uint8_t *)&FETPD,sizeof(FETPD));
    f.close();
    return(true);
  }
  return(false);
}

void setup() 
{
  APmode=false;
  SerialInit();
  Wire.begin(SDA, SCL);
  // Set the port pins as needed
  pinMode(STRIG,OUTPUT);
  digitalWrite(STRIG,LOW);
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
  // Init the DAC
  AD5625_EnableRef(0x1A);
  wifi.disconnect();
  bool result = SPIFFS.begin();
  if(!RestoreSettings()) SaveSettings();
  if(FETPD.Signature != 123456) FETPD = FETPDdefault;
  // If input pin 4 is low then load the default settings
  if(digitalRead(4)==LOW) FETPD = FETPDdefault;
  // Set all the pulser parameters
  AD5625(TWIadd, Vpos, Value2Counts(FETPD.VposValue, &FETPD.VposDAC),3);
  AD5625(TWIadd, Vneg, Value2Counts(FETPD.VnegValue, &FETPD.VnegDAC),3);
  if(FETPD.Invert) digitalWrite(STRIG,HIGH);
  delay(10);
  if(strcmp("AP", FETPD.StartMode) == 0) AP();
  if(strcmp("CONNECT", FETPD.StartMode) == 0) Connect();
}

// Set the pulse output invert state, TRUE or FALSE
void SetInvert(char *res)
{
  if(strcmp("TRUE", res) == 0)
  {
    FETPD.Invert = true;
    digitalWrite(STRIG,HIGH);
    SendACK;
    return;
  }
  else if(strcmp("FALSE", res) == 0)
  {
    FETPD.Invert = false;
    digitalWrite(STRIG,LOW);
    SendACK;
    return;    
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;
}

// Report the pulser output state, HIGH or LOW
void GetOutputLevel(void)
{
  SendACKonly;
  if(digitalRead(TRIGGER) == LOW) serial->println("LOW");
  else serial->println("HIGH");
}

// Set the pulser output state, HIGH, LOW, or PULSE. Pulse = 1mS
void SetOutputLevel(char *res)
{
  int state;
  
  state = digitalRead(TRIGGER); // Read the current state
  if(strcmp("HIGH", res) == 0)
  {
    if(state != HIGH) digitalWrite(STRIG,!digitalRead(STRIG));
    SendACK;
    return;    
  }
  else if(strcmp("LOW", res) == 0)
  {
    if(state != HIGH) digitalWrite(STRIG,!digitalRead(STRIG));
    SendACK;
    return;        
  }
  else if(strcmp("PULSE", res) == 0)
  {
    digitalWrite(STRIG,!digitalRead(STRIG));
    delay(1);
    digitalWrite(STRIG,!digitalRead(STRIG));
    SendACK;
    return;        
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;  
}

void SetVpos(char *sval)
{
  String Sval;
  float  V;

  Sval = sval;
  V = Sval.toFloat();
  if((V >= 0) && (V <= 200))
  {
    FETPD.VposValue = V;
    AD5625(TWIadd, Vpos, Value2Counts(FETPD.VposValue, &FETPD.VposDAC),3);
    SendACK;
    return;
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;  
}

void SetVneg(char *sval)
{
  String Sval;
  float  V;

  Sval = sval;
  V = Sval.toFloat();
  if((V <= 0) && (V >= -200))
  {
    FETPD.VnegValue = V;
    AD5625(TWIadd, Vneg, Value2Counts(FETPD.VnegValue, &FETPD.VnegDAC),3);
    return;
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;  
}

void ReadStreams(void)
{  
  // Put serial received characters in the input ring buffer
  if (Serial.available() > 0) 
  {
    serial = &Serial;
    PutCh(Serial.read());
  }
  if(client)
  {
    if (client.available() > 0) 
    {
      serial = &client;
      PutCh(client.read());
    }
  }
  ESP.wdtFeed();
}

// This function process all the serial IO and commands
void ProcessSerial(void)
{
  ReadStreams();
  // If there is a command in the input ring buffer, process it!
  if(RB_Commands(&RB) > 0) while(ProcessCommand()==0);  // Process until flag that there is nothing to do
                                                                         
}
void loop() 
{
  static int WiFiStatus=0;

  if((WiFiStatus != 3) && (wifi.status() == 3))
  {
    // Here is we just detected a connection
    serial->println("Connected!");
    if (!mdns.begin(FETPD.host, wifi.localIP(), 1))
    {
      serial->println("Error setting up MDNS responder!");
    }
    else serial->println("mDNS responder started");
    mdns.addService("fetpulser", "tcp", 2015);
    server.begin();
    server.setNoDelay(true);
  }
  WiFiStatus = wifi.status();
  //delayMicroseconds(50);
  client = server.available();
  while(client) 
  {
    digitalWrite(0, LOW);
    if (!client.connected()) break;
    ProcessSerial();
  }
  digitalWrite(0, HIGH);
  ProcessSerial();
}
