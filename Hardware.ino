#include "Hardware.h"

// Counts to value and value to count conversion functions.
float Counts2Value(int Counts, DACchan *DC)
{
  return (Counts - DC->b) / DC->m;
}

int Value2Counts(float Value, DACchan *DC)
{
  int counts;

  counts = (Value * DC->m) + DC->b;
  if (counts < 0) counts = 0;
  if (counts > 65535) counts = 65535;
  return (counts);
}

void Calibrate(void)
{
  char   *res;
  String sRes;
  float  V1,V2;

// Calibrate the positive output
   // Set votage to V1
   AD5625(TWIadd, Vpos, 1000,3);
   // Ask user to enter V1
   serial->print("\nEnter Vpos voltage: ");
   delay(1);
   ReadStreams();
   GetToken(false);
   while((res = GetToken(false)) == NULL) ReadStreams();
   sRes = res;
   V1 = sRes.toFloat();
   GetToken(false);  // flush the buffer
   // Set votage to V2
   AD5625(TWIadd, Vpos, 20000,3);
   // Ask user to enter V2
   serial->print("\nEnter Vpos voltage: ");
   delay(1);
   ReadStreams();
   GetToken(false);
   while((res = GetToken(false)) == NULL) ReadStreams();
   sRes = res;
   V2 = sRes.toFloat();
   GetToken(false);  // flush the buffer
   // Calculate the calibration parameters, m and b
   FETPD.VposDAC.m = 19000.0 / (V2-V1);
   FETPD.VposDAC.b = 20000.0 - V2 * FETPD.VposDAC.m;
   AD5625(TWIadd, Vpos, Value2Counts(FETPD.VposValue, &FETPD.VposDAC),3);
// Calibrate the negative output
   // Set votage to V1
   AD5625(TWIadd, Vneg, 1000,3);
   // Ask user to enter V1
   serial->print("\nEnter Vneg voltage: ");
   delay(1);
   ReadStreams();
   GetToken(false);
   while((res = GetToken(false)) == NULL) ReadStreams();
   sRes = res;
   V1 = sRes.toFloat();
   GetToken(false);  // flush the buffer
   // Set votage to V2
   AD5625(TWIadd, Vneg, 20000,3);
   // Ask user to enter V2
   serial->print("\nEnter Vneg voltage: ");
   delay(1);
   ReadStreams();
   GetToken(false);
   while((res = GetToken(false)) == NULL) ReadStreams();
   sRes = res;
   V2 = sRes.toFloat();
   GetToken(false);  // flush the buffer
   // Calculate the calibration parameters, m and b
   FETPD.VnegDAC.m = 19000.0 / (V2-V1);
   FETPD.VnegDAC.b = 20000.0 - V2 * FETPD.VnegDAC.m;
   AD5625(TWIadd, Vneg, Value2Counts(FETPD.VnegValue, &FETPD.VnegDAC),3);
}

// This function sets the DAC counts.
int AD5625(int8_t adr, uint8_t chan, uint16_t val,int8_t Cmd)
{
  int iStat;

  Wire.beginTransmission(adr);
  Wire.write((Cmd << 3) | chan);
  Wire.write((val >> 8) & 0xFF);
  Wire.write(val & 0xFF);
  {
    iStat = Wire.endTransmission();
  }
  return (iStat);
}

// This function enables the internal voltage reference in the
// AD5625
int AD5625_EnableRef(int8_t adr)
{
  int iStat;

  Wire.beginTransmission(adr);
  Wire.write(0x38);
  Wire.write(0);
  Wire.write(1);
  iStat = Wire.endTransmission();
  return (iStat);
}
