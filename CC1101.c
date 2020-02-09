#include "CC1101.h"
#include "UART.h"
#include "DigitalPorts.h"

extern char volatile txSPIBufferInUse;

DigitalInout CC1101_GD0 = {&Port2, BIT6};
DigitalInout CC1101_SS = {&Port3, BIT0};
DigitalInout SPI_SOMI = {&Port3, BIT2};

void activateCC1101(){
    setOutputLow(&CC1101_SS);
    P3SEL &= ~BIT2;
    while (testInput(&SPI_SOMI)) __nop();
    P3SEL |= BIT2;
}
void deactivateCC1101(){
    setOutputHigh(&CC1101_SS);
    P3SEL &= ~BIT2;
    while (!testInput(&SPI_SOMI)) __nop();
    P3SEL |= BIT2;    
}

void initCC1101Ports(){
    // A GD0 az, amelyik lábat lehúzza a CC1101, ha csomagot fogadott
    P2SEL &= ~(BIT7+BIT6); //clear select bits for XIN,XOUT, which are set by default

    useAsInput(&CC1101_GD0);
    useAsInput(&SPI_SOMI);
    enablePullup(&SPI_SOMI);

    useAsOutput(&CC1101_SS);
    setOutputHigh(&CC1101_SS);    
}

void writeRFReg(char address, char value) {
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, address);
    RingBuffer_blockingPutChar(&txSPIBuffer, value);
    while (RingBuffer_getCharNumber(&txSPIBuffer) || txSPIBufferInUse) __nop();
    deactivateCC1101();
}

char readRFReg(char address) {
    RingBuffer_clear(&rxSPIBuffer);
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, address+0x80);
    RingBuffer_blockingPutChar(&txSPIBuffer, 0);
    deactivateCC1101();
    while (RingBuffer_getCharNumber(&rxSPIBuffer) != 2){
        __nop();
    }
    RingBuffer_getChar(&rxSPIBuffer);
    return RingBuffer_getChar(&rxSPIBuffer);
}

void burstWriteRFReg(char address, unsigned char *buffer, unsigned char count){
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, address+0x40);
    for (unsigned char i=0; i<count; ++i){
        RingBuffer_blockingPutChar(&txSPIBuffer, buffer[i]);
    }
    while (RingBuffer_getCharNumber(&txSPIBuffer) || txSPIBufferInUse) {
        __nop();
    }
    
    deactivateCC1101();
}

void burstReadRFReg(char address, unsigned char *buffer, unsigned char count){
    RingBuffer_clear(&rxSPIBuffer);
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, address+0xC0);
    while (RingBuffer_getCharNumber(&rxSPIBuffer) < 1){
        __nop();
    }
    RingBuffer_clear(&rxSPIBuffer); // A Státusz most nem érdekel
    for (unsigned char i=0; i<count; ++i){
        RingBuffer_blockingPutChar(&txSPIBuffer, 0);
        while (RingBuffer_getCharNumber(&rxSPIBuffer) < 1){
            __nop();
        }
        buffer[i] = RingBuffer_getChar(&rxSPIBuffer);
    }
    deactivateCC1101();
}

void dumpBurstReadRFReg(char address, unsigned char count){
    RingBuffer_clear(&rxSPIBuffer);
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, address+0xC0);
    while (RingBuffer_getCharNumber(&rxSPIBuffer) < 1){
        __nop();
    }
    RingBuffer_clear(&rxSPIBuffer); // A Státusz most nem érdekel
    for (unsigned char i=0; i<count; ++i){
        RingBuffer_blockingPutChar(&txSPIBuffer, 0);
        while (RingBuffer_getCharNumber(&rxSPIBuffer) < 1){
            __nop();
        }
        RingBuffer_getChar(&rxSPIBuffer);
    }
    deactivateCC1101();
}

void sendStrobe(char strobe){
    activateCC1101();
    RingBuffer_blockingPutChar(&txSPIBuffer, strobe);
    while (RingBuffer_getCharNumber(&txSPIBuffer) || txSPIBufferInUse) __nop();
    deactivateCC1101();
}

void resetCC1101(){
    setOutputHigh(&CC1101_SS);
    __delay_cycles(30*16);
    setOutputLow(&CC1101_SS);
    __delay_cycles(30*16);
    setOutputHigh(&CC1101_SS);
    __delay_cycles(45*16);

    sendStrobe(TI_CC_REG_SRES);
}
void setRXMode(){
    sendStrobe(TI_CC_REG_SRX);
}
void setTXMode(){
    sendStrobe(TI_CC_REG_STX);
}

void initCC1101(){
    resetCC1101();
    __delay_cycles(1000000*16);
    writeRFSettings();
    unsigned char patable[] = {0xFB};
    burstWriteRFReg(TI_CC_REG_PATABLE, patable, 1);
    setRXMode();
}

void writeIntoTXBuffer(unsigned char *buffer, unsigned char count){
    burstWriteRFReg(TI_CC_REG_TXFIFO, buffer, count);
}

/*
void writeRFSettings(){
    // Write register settings
    writeRFReg(TI_CC_REG_IOCFG2,   0x0B);  // GDO2 output pin config.
    writeRFReg(TI_CC_REG_IOCFG0,   6);  // GDO0 output pin config.
    writeRFReg(TI_CC_REG_PKTLEN,   0xFF);  // Packet length.
    //writeRFReg(TI_CC_REG_PKTCTRL1, 0x05);  // Packet automation control.
    writeRFReg(TI_CC_REG_PKTCTRL0, 0x05);  // Packet automation control.
    writeRFReg(TI_CC_REG_ADDR,     0x01);  // Device address.
    //writeRFReg(TI_CC_REG_CHANNR,   0x00); // Channel number.
    //writeRFReg(TI_CC_REG_FSCTRL0,  0x00); // Freq synthesizer control.
    writeRFReg(TI_CC_REG_FREQ2,    0x21); // Freq control word, high byte
    writeRFReg(TI_CC_REG_FREQ1,    0x62); // Freq control word, mid byte.
    writeRFReg(TI_CC_REG_FREQ0,    0x76); // Freq control word, low byte.
    writeRFReg(TI_CC_REG_MDMCFG4,  0xF7); // Modem configuration. Felső 4 bit: F
                                          // Az alsó 4 bit a data rate exponense. Eredetileg 5
    writeRFReg(TI_CC_REG_MDMCFG3,  0x43); // Modem configuration. Eredetileg 0x83. Ez a data rate mantisszája
                                          // Képlet: R = @(M, E)(256+M)*2^(E-28)*26000000;
    writeRFReg(TI_CC_REG_MDMCFG2,  0x12); // Eredetileg 0x13 Modem configuration. GFSK + 30/32 sync word bits detected
                                          // 0x1B : GFSK + Manchester coding + 30/32 sync word bits detected
                                          // 0x12 : GFSK + 16/16 sync word
                                          // Megjegyzés: A Sync word mérete nagyon durván befolyásolja az adatsebességet
    //writeRFReg(TI_CC_REG_MDMCFG1,  0x62); // Modem configuration.
    //writeRFReg(TI_CC_REG_MDMCFG0,  0x76); // Modem configuration.
    writeRFReg(TI_CC_REG_DEVIATN,  0x15); // Modem dev (when FSK mod en)
    writeRFReg(TI_CC_REG_MCSM1 ,   0x0F); //MainRadio Cntrl State  Machine // Eredetileg 0x3F   
    writeRFReg(TI_CC_REG_MCSM0 ,   0x18); //MainRadio Cntrl State Machine. Eredetileg 0x18
    writeRFReg(TI_CC_REG_FOCCFG,   0x16); // Freq Offset Compens. Config
    writeRFReg(TI_CC_REG_FSCAL3,   0xE9); // Frequency synthesizer cal.
    writeRFReg(TI_CC_REG_FSCAL2,   0x2A); // Frequency synthesizer cal.
    writeRFReg(TI_CC_REG_FSCAL1,   0x00); // Frequency synthesizer cal.
    writeRFReg(TI_CC_REG_FSCAL0,   0x1F); // Frequency synthesizer cal.
    //writeRFReg(TI_CC_REG_FSTEST,   0x59); // Frequency synthesizer cal.
    writeRFReg(TI_CC_REG_TEST2,    0x81); // Various test settings.
    writeRFReg(TI_CC_REG_TEST1,    0x35); // Various test settings.
    writeRFReg(TI_CC_REG_TEST0,    0x09);  // Various test settings.
}
*/

void writeRFSettings(){
    // Write register settings
    writeRFReg(TI_CC_REG_IOCFG2, 0x0B);   // GDO2 output pin config.    
    writeRFReg(TI_CC_REG_IOCFG0, 0x06);   // GDO0 output pin config.
    writeRFReg(TI_CC_REG_PKTLEN, 0x30);   // Packet length.
    writeRFReg(TI_CC_REG_PKTCTRL0, 0x05); // Packet automation control.
    writeRFReg(TI_CC_REG_ADDR, 0x01);     // Device address.
    writeRFReg(TI_CC_REG_FSCTRL1, 0x0E);  // Freq synthesizer control.
    writeRFReg(TI_CC_REG_FREQ2, 0x21);    // Freq control word, high byte
    writeRFReg(TI_CC_REG_FREQ1, 0x62);    // Freq control word, mid byte.
    writeRFReg(TI_CC_REG_FREQ0, 0x76);    // Freq control word, low byte.
    writeRFReg(TI_CC_REG_MDMCFG4, 0x0E);  // Modem configuration. Felső 4 bit: F
                                          //                      Az alsó 4 bit a data rate exponense.
    writeRFReg(TI_CC_REG_MDMCFG3, 0x3B);  // Modem configuration. Eredetileg 0x83. Ez a data rate mantisszája
    writeRFReg(TI_CC_REG_MDMCFG2, 0x73);  // Modem configuration.
    writeRFReg(TI_CC_REG_MDMCFG1, 0x42);  // Modem configuration.
    writeRFReg(TI_CC_REG_DEVIATN, 0x00);  // Modem configuration.
    writeRFReg(TI_CC_REG_MCSM1, 0x0F); //MainRadio Cntrl State  Machine     
        /*
        Az MCSM1 5:4 mezője a CCA_MODE-ot adja meg.
        Ha ez nem 00, akkor a TXR strobe hatására nem biztos, hogy a rádiómodul küldő üzemmódba vált, csak akkor, 
        ha az adatlab 81. oldalán a feltételek teljesülnek.
        00 esetén mindig megpróbálja elküldeni a csomagot.
        Nekem most ez a módszer a legegyszerűbb, mert az nem baj, ha nem érkezik meg a csomag, az viszont igen,
        ha beragad a buffer-be.
        A csomagküldések minőségét viszont ezen keresztül lehetne növelni, hogy ha a CCA vizsga elbukik, ne is menjen ki a csomag.
        (Szerintem az ACK-ra várakozás jobb módszer)
        */     
    writeRFReg(TI_CC_REG_MCSM0, 0x18);    // MainRadio Cntrl State Machine.
    writeRFReg(TI_CC_REG_FOCCFG, 0x1D);   // Freq Offset Compens. Config
    writeRFReg(TI_CC_REG_BSCFG, 0x1C);    // Bit Synchronization Configuration
    writeRFReg(TI_CC_REG_AGCCTRL2, 0xC7); // AGC Control
    writeRFReg(TI_CC_REG_AGCCTRL1, 0x00); // AGC Control
    writeRFReg(TI_CC_REG_AGCCTRL0, 0xB0); // AGC Control 
    writeRFReg(TI_CC_REG_FREND1, 0xB6);   // Front End RX Configuration
    writeRFReg(TI_CC_REG_FSCAL3, 0xEA);   // Frequency Synthesizer Calibration
    writeRFReg(TI_CC_REG_FSCAL2, 0x2A);   // Frequency Synthesizer Calibration
    writeRFReg(TI_CC_REG_FSCAL1, 0x00);   // Frequency Synthesizer Calibration
    writeRFReg(TI_CC_REG_FSCAL0, 0x1F);   // Frequency Synthesizer Calibration
    writeRFReg(TI_CC_REG_TEST0, 0x09);    // Various test settings.
}