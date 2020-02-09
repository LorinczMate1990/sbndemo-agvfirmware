#ifndef TIMER_H
#define TIMER_H

#include <msp430f2274.h>
#include "UART.h"

#define TIMER_ON (TBCCTL0 = CCIE) // Aktiválja a TimerB-t
#define TIMER_OFF (TBCCTL0 = 0)    // Deaktiválja a TimerB-t

void initTimerB(unsigned int max, char useSMCLK);

#endif
