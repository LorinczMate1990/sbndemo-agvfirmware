#ifndef CC1101_H
#define CC1101_H

#include "hardware_board.h"
#include "CC1101RegisterConsts.h"

extern DigitalInout CC1101_GD0;
extern DigitalInout CC1101_SS;
extern DigitalInout SPI_SOMI;

// Alacsonyszintű kezelőfüggvények
void activateCC1101();
void deactivateCC1101();
void initCC1101Ports();

void writeRFReg(char address, char value);
void burstWriteRFReg(char address, unsigned char *buffer, unsigned char count);
void burstReadRFReg(char address, unsigned char *buffer, unsigned char count);
void dumpBurstReadRFReg(char address, unsigned char count);
char readRFReg(char address);
void sendStrobe(char strobe);


// Rádió állapotait beállító függvények
void resetCC1101();
void setRXMode();
void setTXMode();
void writeRFSettings();
void initCC1101();

// Csomagkezelő függvények
void writeIntoTXBuffer(unsigned char *buffer, unsigned char count);

#endif