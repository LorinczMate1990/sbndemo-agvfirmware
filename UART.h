#ifndef UART_H
#define UART_H

#include <msp430f2274.h>
#include "RingBuffer.h"
#include "hardware_board.h"

extern RingBuffer rxUartBuffer;
extern RingBuffer txUartBuffer;
extern RingBuffer rxSPIBuffer;
extern RingBuffer txSPIBuffer;

void initSerialPort();
void initSPIPort();

#endif
