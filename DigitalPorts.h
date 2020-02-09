#ifndef DIGITAL_PORTS
#define DIGITAL_PORTS

typedef struct DigitalPortStruct{
    volatile unsigned const char *in;
    volatile unsigned char *out;
    volatile unsigned char *dir;
    volatile unsigned char *ie;
    volatile unsigned char *ren;
    volatile unsigned char *ifg;
    volatile unsigned char *ies;
} DigitalPort;

typedef struct DigitalInoutStruct{
    volatile DigitalPort *port;
    const int bit;
} DigitalInout;

void enableFallingEdgeInterrupt(DigitalInout *w);
void enableRisingEdgeInterrupt(DigitalInout *w);
void enablePullup(DigitalInout *w);
void enablePulldown(DigitalInout *w);
void useAsOutput(DigitalInout *w);
void useAsInput(DigitalInout *w);

void setOutputLow(DigitalInout *w);
void setOutputHigh(DigitalInout *w);
void toggleOutput(DigitalInout *w);
char testInput(DigitalInout *w);

char testInterruptFlag(DigitalInout *w);
void clearInterruptFlag(DigitalInout *w);
#endif