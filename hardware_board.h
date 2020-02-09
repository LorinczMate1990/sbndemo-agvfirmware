#ifndef HARDWARE_BOARD_H
#define HARDWARE_BOARD_H

#include <msp430f2274.h>
#include "DigitalPorts.h"
#include "AnalogPorts.h"

#include "UART.h"
#include "CC1101.h"

/**
Enable global interrupts : __enable_interrupt()
Disable global interrupts : __disable_interrupt()
*/

enum BoardConfigEnum {ENABLE_BUTTON_INTERRUPT=1, // __attribute__((interrupt(PORT1_VECTOR)))
                      ENABLE_SERIAL_PORT=2,
                      ENABLE_CC1101=4,
                      ENABLE_ANALOG_INPUT = 8
                     };
typedef enum BoardConfigEnum BoardConfig;

void initBoard(unsigned int config);

extern DigitalPort Port1;
extern DigitalPort Port2;
extern DigitalPort Port3;

extern DigitalInout BUTTON;
extern DigitalInout RED_LED;
extern DigitalInout GREEN_LED;

extern DigitalInout FORWARD;
//extern DigitalInout TURN_ENABLE;
//extern DigitalInout TURN_RIGHT;
//extern DigitalInout TURN_LEFT;
extern DigitalInout SERVO_CONTROL;

#endif
