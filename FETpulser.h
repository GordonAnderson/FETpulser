#ifndef FETpulser_h
#define FETpulser_h

typedef struct
{
  // FET pulser data
  float   VposValue;
  float   VnegValue;
  bool    Invert;
  DACchan VposDAC;
  DACchan VnegDAC;
  // WiFi data
  char host[20];
  char ssid[20];
  char password[20];
  char StartMode[12];
  // End marker
  int   Signature;      // Flag to insure the array was read from flash ok
} FETpulserData;

#endif


