//
// Hardware definitions for the FET pulser
//
#ifndef Hardware_h
#define Hardware_h

typedef struct
{
  int8_t  Chan;              // DAC channel number 0 through max channels for chip
  float   m;                 // Calibration parameters to convert engineering to DAC counts
  float   b;                 // DACcounts = m * value + b, value = (DACcounts - b) / m
} DACchan;

// FET pulser IO pins
#define   SCL     12
#define   SDA     13
#define   TRIGGER 14
#define   LDAC    4
#define   STRIG   5

// DAC constants
#define   TWIadd   0x1A
#define   Vpos     0
#define   Vneg     1

// Function prototypes
void Calibrate(void);
float Counts2Value(int Counts, DACchan *DC);
int Value2Counts(float Value, DACchan *DC);
int AD5625(int8_t adr, uint8_t chan, uint16_t val,int8_t Cmd);
int AD5625_EnableRef(int8_t adr);

#endif

