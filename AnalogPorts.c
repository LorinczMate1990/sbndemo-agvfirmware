#include <msp430f2274.h>
#include "AnalogPorts.h"

volatile char waitForAnalogRes = 0;

void initAnalogPorts(int ports){
    P2SEL |= BIT0;    
	ADC10CTL1 = INCH_0 + ADC10DIV_3;                      // Channel 0, ADC10CLK/0
	ADC10CTL0 = SREF_0 + ADC10SHT_0 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
	ADC10AE0 |= BIT0;                                     // ADC input enable P1.0
}

void startAnalogSampling(){
    waitForAnalogRes = 1;
    ADC10CTL0 |= ENC + ADC10SC;	    
}
char isSamplingReady(){
    return !waitForAnalogRes;
}
unsigned short getAnalogValue(int port){
    return ADC10MEM;
}

// ADC10 interrupt service routine
__attribute__((interrupt(ADC10_VECTOR)))
void ADC10_ISR (void)
{
	waitForAnalogRes = 0;
}
