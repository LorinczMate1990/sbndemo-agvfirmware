#ifndef ANALOG_PORTS
#define ANALOG_PORTS

void initAnalogPorts(int ports);
void startAnalogSampling();
char isSamplingReady();
unsigned short getAnalogValue(int port);

#endif