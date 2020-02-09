#include <msp430f2274.h>
#include "UART.h"
#include "timer.h"

#define TIMER_ON (TBCCTL0 = CCIE) // Aktiválja a TimerB-t
#define TIMER_OFF (TBCCTL0 = 0)    // Deaktiválja a TimerB-t

void initTimerB(unsigned int max, char useSMCLK){
  TBCCTL0 = CCIE;      // Engedélyezi a megszakítást a TimerB-re 
  TBCCR0 = max;        // A megszakítás sebessége. Minél kisebb, annál sûrûbb. 
                       //  => 0.84sec 12 =1ms ACLK (TBSSEL_1) esetén
                       // SMCLK (TBSSEL_2) esetén a timer órajele a központi órajel, 
                       // ami 16MHz esetén:
                       // 500 -> 1/32 = 0.03125 ms-es megszakítás

  if (useSMCLK){
    TBCTL = MC_1+TBSSEL_2; // Timer with master clock
  } else {
    TBCTL = MC_1+TBSSEL_1; // Timer with ~1.2kHz clock
  }
}