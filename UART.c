#include "UART.h"
#include "RingBuffer.h"
#include "hardware_board.h"

RingBuffer rxUartBuffer;
RingBuffer txUartBuffer;
RingBuffer rxSPIBuffer;
RingBuffer txSPIBuffer;

volatile char txBufferInUse = 0;
char uartTxBuffer[100];
char uartRxBuffer[100];

volatile char txSPIBufferInUse = 0;
char spiTxBuffer[100];
char spiRxBuffer[100];

/**
A chipen két soros modul (A és B) található, melyeket különböző protokollokra lehet felkonfiguráéni.

Az A modult UART-ként használjuk, ezzel kommunikálunk a PC-vel.

Az UART a legegyszerűbb aszinkron, full duplex protokoll.
    Aszinkron   : Nincs közös órajel
    Full duplex : A kommunikációban résztvevő felek egyszerre adhatnak és vehetnek. Pl.: telefon
    Half duplex : A kommunikáció egy adott időben csak az egyik irányba történik. Pl.: Walkie-talkie
    Simplex     : A kommunikációs csatorna egyirányú. Pl.: Rádió

A kommunikáció bájt-alapú, a résztvevő felek megállapodnak egy baud rate-ben (bitsebesség) és 
a bájtokat határoló start és stop bitek számában. (Ez a kommunikációban overhead-ként jelentkezik 
és a szinkronizáció miatt szükséges)
*/
void initSerialPort(){
    // Ennél a chipnél a P3-as port 4. és 5. lába felelős a soros kommunikációért
    // A P3SEL 4. és 5. bitének egybe állításával lehet a sima digitális IO funkciók helyett
    // soros portként használni őket. (A PxSEL alapvetően arra való, hogy aktiváljuk a lábak speciális funkcióit)
    P3SEL |= BIT4 | BIT5;
    // Az UCA0CTL0 és az UCA0CTL1 kontrol regiszterekkel lehet beállítani az A modult
    // Ennek részletes leírása megtalálható az MSP430fx2xx család leírásában az UCAxCTL1, USCI_Ax Control Register 1
    // fejezetben
    // Az UCA0CTL1 bitjei:
    //      UCSSEL_2
    //      UCSSEL_1 : A baud rate generátor által használt órajel kiválasztása
    //                 Mi az SMCLK-t használjuk, ami 1MHz-es és az 10 bitkombináció állítja be
    //      UCRXEIE  : Hibás / értelmezhetetlen karakterek fogadásakor is állítódjon-e be az RXIFG flag?
    //      UCBRKIE  : A BREAK karakter fogadása is állítsa be az RXIFG flag-et?
    //      UCDORM   : Ha 0, minden karakter fogadása beállítja az RXIFG flag-et. Ha egy, csak néhány speciális
    //      UCTXBRK  : Transmit break
    //      USCWRST  : Ha 1, akkor a soros modul reset állapotban van, nem csinál semmit
    // Nekünk ezek közül az UCSSEL_2-t kell beállítanunk, hogy az 1MHz-es SMLCK órajelet használjuk ű
    // baud rate generálásához és a USCWRST-t 1-be kell tennünk, hogy a modul ne működjön, amíg nem állítjuk be
    UCA0CTL1 = UCSSEL_2;
    
    // Az UCA0BR0 és az UCA0BR1 a baud rate előosztók
    // Az elv az, hogy az órajelből valahogy elő kell állítani a kiválasztott baud rate-et, ami
    // esetünkben a szabványos legalacsonyabb, 1200 lesz (ez az égetőben lévú soros konverter hibája miatt ilyen alacsony)
    // Ehhez az 1MHz = 1000kHz-es órajelet el kell osztanunk 833-mal, hogy 1.2kHz-et kapjunk, ami már a bitsebességünk.
    // (Egész pontosan 1.2005-öt fogunk kapni, de ezt a hibát már a modulátorral kell javítani (később)
    // Az UCA0BR0 és az UCA0BR1 1-1 bájtos regiszterek, ezért a 833-at fel kell osztanunk alsó és felső bájtra
    // Mivel 833 = 3*256 + 65, ezért
    //UCA0BR0 = 65;                   
    //UCA0BR1 = 3;                    
    // Mivel az órajelet 1MHz-ről 16MHz-re növeltem, ezért 833*16 ~ 13333 = 52 * 256 + 21
    UCA0BR0 = 21;
    UCA0BR1 = 52;
    // A generált órajel így: 1200,03000075  

    // A pontatlan időzítés miatt a stabil kommunikációhoz modulálni kell (a végére érthető lesz)
    // A moduláció kiválasztásához először meg kell határozni az órajel és a baud rate hányadosát:
    //                   N = 16MHz / 1200Hz = 13333.33
    // Alacsony frekvenciás baud rate mód esetén (ennek az ellentéte az oversampling) az órajelosztó 
    // (N) értékét az UCA0BR0 regiszterbe kell tenni (ahogy az megtörtént), a tört részét pedig a
    // modulátornak kell megadni az alábbi módon:
    //              UCBRSx = round( ( N – INT(N) ) * 8 )
    // Ez esetünkben UCBRSx = round( (833.33 - 833) * 8 ) = round(8/3) = 3
    // Az UCBRSx bitek az UCA0MCTL regiszterben vannak és a 011 bitmintát kell nekik beállítani
    
    // Az UCA0MCTL felső 4 bitje az UCBRFx bitcsoport, ami oversampling esetén fontos
    // Utána jönnek az URBRSx bitek, majd legalul van az UCOS16 bit, ami 1, ha oversampling módban akarunk
    // működni. (Ekkor egy bitet 16-szor mintavételezünk és többségi szavazás alapján döntünk. Az 
    // osztónak 16-odnak kell lennie és ezzel nagyobb megbízhatóságot érhetünk el, de ennek esetünkben nincs
    // jelentősége)
    // Tehát az UCA0MCTL regiszterben minden bitet ki kell nullázni, kivéve az UCBRSx alsó kettőjét.
    UCA0MCTL = UCBRS1 | UCBRS0;

    // Ugyanezek 9600 baud rate-re:
    // N = 1000Hz / 9.6kHz = 104.17
    //UCA0BR0 = 104;                   
    //UCA0BR1 = 0;    
    // UCBRSx = round( ( N – INT(N) ) * 8 ) = round(0.17*8) = 1
    //UCA0MCTL = UCBRS0;
    // Sajnos a programozó, amit használunk, nem képes 9600 baud rate-tel dolgozni, elvesznek karakterek
    
    // Végül a küldést és a fogadást vezérlő adatszerkezeteket kell beállítani
    RingBuffer_init(&rxUartBuffer, uartRxBuffer, 100);
    RingBuffer_init(&txUartBuffer, uartTxBuffer, 100);
    txUartBuffer.isFinalDestinationIsUsed = &txBufferInUse;
    txUartBuffer.finalDestination = &UCA0TXBUF;

    // Végül az UCAxCTL1 regiszterben az UCSWRST bitet (lásd fentebb) ki kell nulláznunk, hogy a modul elinduljon
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

    // Engedélyezzük még a fogadási és küldési interruptokat
    IE2 |= UCA0RXIE; // Enable USCI_A0 RX interrupt. Máshogy UC0IE
    IE2 |= UCA0TXIE; // Enable USCI_A0 TX interrupt. Máshogy UC0IE
    
    // És nullázzuk az interrupt flag-eket
    // !!! Ha itt már engedélyezve lenne a globális megszakítás, akkor ezzel a nullázással már elkéstünk volna
    IFG2 &= ~UCA0RXIFG;
    IFG2 &= ~UCA0TXIFG; 
}

/*
A chip a CC1101-es modullal SPI interface-en kommunikál
Ez egy szinkron, master-slave protokoll több résztvevő között.
    szinkron : A kommunikációt egy közös órajel szinkronizálja
    master-slave : A kommunikációban 
Az SPI hálózatban egy master és bármennyi slave van.
Minden résztvevőnek 3 lába vesz részt a kommunikációban:
    SCLK : Syncron clock - Ez a szinkronizáló órajel, a master generálja
    MOSI : Master out slave in - A master ezen küldi ki az adatot
    MISO : Master in slave out - A master ezen keresztül fogadja az adatot

Van még a slave-eken egy SS láb, ami ha magas, a slave inaktív, ha alacsony, a slave figyeli a bejövő adatot és
joga van válaszolni. Ez azért fontos, mert az összes node MOSI és MISO lábai össze vannak kötve és az összes slave 
ugyanazt a lábát rángatná a szervernek.

A masterhez befut minden slave SS lába és így tud választani a slave-ek között.
A protokoll nagy hátránya, hogy a masternek annyi kimeneti láb kell az SCLK-n, MOSI-n és MISO-n felül, amennyi slave van.
*/
void initSPIPort(){
    // A B soros modul SPI-ra használható lábai is a hármas porton találhatóak
    //  2. láb (BIT1) : SIMO
    //  3. láb (BIT2) : SOMI
    //  4. láb (BIT3) : UCLK
    P3SEL |= BIT1 | BIT2 | BIT3;
    // UCB0CTL0 beállítása
    //  UCCKPH : Ez azt állítja be, hogy az órajel melyik fázisában kell az adatot kitenni / beolvasni
    //           Nekünk ez most 0
    //  UCCKPL : Clock polarity. Nekünk most a magas órajel az inaktív állapot, tehát 1
    //  UCMSB  : A bitalapú soros kommunikációknál közös probléma, hogy az első bit a legalacsonyabb vagy a legmagasabb helyiérték-e
    //           Nekünk most az MSB (most significant bit) az első, tehát 1. (Ez a slave-ektől függ)
    //  UC7BIT : 7 vagy 8 bites-e egy karakter? Esetünkben 8 bites, tehát ez 0
    //  UCMST  : Master mode select. Most a chip lesz a master, tehát ez 1
    //  UCMODE[01] : Ezzel a két bittel tudunk választani a 4 üzemmód között:
    //   00 : 3 pin SPI. Ilyenkor a slave selectet mi vezéreljük, vagy csak magasra/alacsonyra kötjük
    //   01 : 4 pin SPI, UCxSTE active high. A SS-t is a hardver kezeli és a slave magas SS-nél van engedélyezve
    //   10 : 4 pin SPI, UCxSTE active low. A SS-t is a hardver kezeli és a slave alacsony SS-nél van engedélyezve    
    //      Ez az utóbbi kettő üzemmód nem túl rugalmas, a slave-eket nem tudjuk akárhogy kezelni
    //   11 : I2C üzemmód. Ez egy teljesen másik soros protokoll
    //  UCSYNC : Sync mode enable. Nekünk ez az SPI-hoz kell, tehát 1
    // A kész beállítás:  
    UCB0CTL0 = UCCKPL | UCMST | UCMSB | UCSYNC;

    // Az UCB0CTL1 regiszterben minket megint csak az órajel kiválasztása és a modul ideiglenes tiltása érdekel
    // Jó hír: Mivel az SPI szinkron, nem kell foglalkozni az órajelfrekvencia kiszámításával, mert a slave-ek
    // a mastertől azt megkapják 
    UCB0CTL1 = UCSSEL_2 | UCSWRST;

    // Azért az 1MHz-es órajelre egy előosztót érdemes betenni
    // Akár a baud rate esetén, itt is az UCB0BR0 és UCB0BR1 regiszterek közös beállításával
    // lehet egy órajelosztót betenni. Ez most legyen 2.
    // Mivel az órajelet 16MHz-re növeltem, ezért 2 helyett az előosztó legyen 32
    // De 32-vel nem működött TODO: Utána nézni, miért nem
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    // UCB0STAT beállítása
    //  UCLISTEN : 1 esetén a kimenet egyből bemásolódik a bemenetbe is, gyakorlatilag egy echo
    //  UCFE : Framing error flag. Ezt olvassuk és hiba esetén töröljük, ha akarjuk
    //  UCOE : Overrun error flag. Akkor állítódik be, ha nem olvastuk ki az UCxRXBUF-ot, mire egy új jön
    //         Magától törlődik
    //  UCBUSY : Az értéke 1, ha küldés vagy fogadás van folyamatban
    // Ezzel igazából nincs dolgunk

    // IE2 bit beállítása
    //  UCB0TXIE : Transmit interrupt enable a B porton (most SPI)
    //  UCB0RXIE : Receive interrupt enable a B porton (most SPI)
    //  UCA0TXIE : Transmit interrupt enable az A porton (most UART)
    //  UCA0RXIE : Receive interrupt enable au A portin (most UART)

    // Végül a küldést és a fogadást vezérlő adatszerkezeteket kell beállítani
    RingBuffer_init(&rxSPIBuffer, spiRxBuffer, 10);
    RingBuffer_init(&txSPIBuffer, spiTxBuffer, 10);
    txSPIBuffer.isFinalDestinationIsUsed = &txSPIBufferInUse;
    txSPIBuffer.finalDestination = &UCB0TXBUF;

    // Kiveszem a modult reszetből a beállítások elkészülte után
  
    UCB0CTL1 &= ~UCSWRST; 

    IE2 |= UCB0RXIE; // Enable USCI_B0 RX interrupt. Máshogy UC0IE
    IE2 |= UCB0TXIE; // Enable USCI_B0 TX interrupt. Máshogy UC0IE        

    // Töröljük az interrupt flaget
    IFG2 &= ~UCB0TXIFG;
    IFG2 &= ~UCB0RXIFG;

    // TODO : Miért kell ezt a UCB0CTL1 &= ~UCSWRST;  sor után tenni???
}

/**
Innentől jönnek a soros port interrupt kezelői, amik kezelik a soros és az SPI megszakításokat
Mivel a két soros modulnak a megszakításvektora ugyanazon a címen van, ezért ugyanaz a megszakításkezelő kezeli őket,
a megszakításon belül csak az interrupt flag-ekkel lehet különbséget tenni.
*/

__attribute__((interrupt(USCIAB0RX_VECTOR)))
void USCI0RX_ISR(void){
    if (IFG2 & UCA0RXIFG){
        RingBuffer_putChar(&rxUartBuffer, UCA0RXBUF);
        IFG2 &= ~UCA0RXIFG;
    }
    if (IFG2 & UCB0RXIFG){
        RingBuffer_putChar(&rxSPIBuffer, UCB0RXBUF);
        IFG2 &= ~UCB0RXIFG;
    }
}

__attribute__((interrupt(USCIAB0TX_VECTOR)))
void USCI0TX_ISR(void){
    if (IFG2 & UCA0TXIFG){
        if ( !txUartBuffer.empty ){
            UCA0TXBUF = RingBuffer_getChar(&txUartBuffer);
        } else {
            txBufferInUse = 0;
        }
        IFG2 &= ~UCA0TXIFG;
    }
    if (IFG2 & UCB0TXIFG){
        if ( !txSPIBuffer.empty ){
            UCB0TXBUF = RingBuffer_getChar(&txSPIBuffer);
        } else {
            txSPIBufferInUse = 0;
        }
        IFG2 &= ~UCB0TXIFG;
    }
}