#include "hardware_board.h"

DigitalPort Port1 = {&P1IN, &P1OUT, &P1DIR, &P1IE, &P1REN, &P1IFG, &P1IES};
DigitalPort Port2 = {&P2IN, &P2OUT, &P2DIR, &P2IE, &P2REN, &P2IFG, &P2IES};
DigitalPort Port3 = {&P3IN, &P3OUT, &P3DIR, 0, &P3REN, 0, 0};

DigitalInout FORWARD = {&Port2, BIT1};
//DigitalInout TURN_ENABLE = {&Port2, BIT3};
//DigitalInout TURN_RIGHT = {&Port2, BIT2};
//DigitalInout TURN_LEFT = {&Port2, BIT4};
DigitalInout SERVO_CONTROL = {&Port2, BIT0};

DigitalInout BUTTON = {&Port1, BIT2};
DigitalInout RED_LED = {&Port1, BIT0};
DigitalInout GREEN_LED = {&Port1, BIT1};

void initCarSubsystem(){
    useAsOutput(&FORWARD);
    useAsOutput(&SERVO_CONTROL);
    
}

void initBoard(unsigned int config){
    WDTCTL = WDTPW + WDTHOLD;                  // Stop WDT. In a factory code, this is forbidden !!!
    BCSCTL1 = CALBC1_16MHZ;                    // Set DCO (Digitally controlled oscillator)
    BCSCTL2 = 0;
    BCSCTL3 |= LFXT1S_2;                       // LFXT1Sx = 10
    DCOCTL = CALDCO_16MHZ;                     // Set DCO


    //  enable pullups on SW1
    enablePullup(&BUTTON);

    // Configure LED outputs on port 1
    useAsOutput(&RED_LED);
    useAsOutput(&GREEN_LED);
    useAsInput(&BUTTON);
    
    initCarSubsystem();

    if (config & ENABLE_BUTTON_INTERRUPT){
        enableFallingEdgeInterrupt(&BUTTON);
    }
    
    if (config & ENABLE_CC1101){
        initCC1101Ports();
    }

    if (config & ENABLE_SERIAL_PORT){
        initSerialPort();
        initSPIPort();
    }

    if (config & ENABLE_ANALOG_INPUT){
        initAnalogPorts(0);
    }
}

void enableGlobalInterrupt(){
    __bis_SR_register(GIE);
}

void disableGlobalInterrupt(){
    __bic_SR_register(GIE);
}
