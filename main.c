#include "hardware_board.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "timer.h"

const char CAR_NO_FEEDBACK = 0;
const char VERBOSE = 0;
const char DRIVER_CODE = 1;
const char USE_NEURON_NOT_PID = 0;
unsigned char counter = 0;
volatile char sendingInProgress = 0;

unsigned short a2usFixpoint(char* str, char fixpoint){
    unsigned short ret = 0;
    char gotDecimalPoint = 0;
    while ((*str) && fixpoint>=0){
        if ((*str) == '.'){
            gotDecimalPoint = 1;
        } else {
            ret *= 10;
            ret += (*str)-'0';
        }
        if (gotDecimalPoint) {
            fixpoint--;
        }
        str++;
    }
    return ret;
}

void sendPacket(unsigned char *buffer){
    writeIntoTXBuffer(buffer, buffer[0]+1);
    sendingInProgress = 1;
    setTXMode();
}

unsigned char recPacket(unsigned char *buffer, unsigned char maxLength){
    unsigned char len = readRFReg(0x3f);
    if (len+2<maxLength){
        burstReadRFReg(0x3f, buffer, len+2); // RSSI és LQI miatt
    } else {
        // Nem tudom kiolvasni, de a buffert ki kell üríteni.
        if (VERBOSE>0) RingBuffer_putString(&txUartBuffer, "\n\rOF\n\r");
        dumpBurstReadRFReg(0x3f, len+2);
    }
    return len;
}

volatile char buttonPressed = 0;
volatile char packetReceivedOrSent = 0;
volatile char packetReceived = 0;
volatile char packetSent = 0;
volatile char timerBFlag = 0;

char* strip(char *str){
    unsigned int start = 0;
    while (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r') ++start;
    unsigned int end = strlen(str)-1;
    while (str[end] == ' ' || str[end] == '\t' || str[end] == '\n' || str[end] == '\r') --end;
    str[end+1] = 0;
    return str+start;
}

typedef struct CarStruct{
    unsigned short speedRef;
    unsigned short steerRef;
    unsigned short speed;
    float biasWeight;
    float leftSteerWeight;
    float rightSteerWeight;
    float speedWeight;
    float speedRefWeight;

    float intError;
} Car;

void Car_createControllerMessage(Car *car, unsigned char *buffer, char validSpeedRef){
    buffer[0] = validSpeedRef;
    buffer[1] = car->speedRef / 256;
    buffer[2] = car->speedRef % 256;
    buffer[3] = car->steerRef / 256;
    buffer[4] = car->steerRef % 256;
    buffer[5] = car->speed / 256;
    buffer[6] = car->speed % 256;
}

void Car_createDriverMessage(Car *car, unsigned char *buffer){
    memcpy(buffer, &(car->biasWeight), 4);
    memcpy(buffer+4, &(car->leftSteerWeight), 4);
    memcpy(buffer+8, &(car->rightSteerWeight), 4);
    memcpy(buffer+12, &(car->speedWeight), 4);
    memcpy(buffer+16, &(car->speedRefWeight), 4);
}

const float epsilon = 0.0001;

void Car_udpateByControllerMessageAsNeuron(Car *car){
    float error = ((float)car->speed - (float)car->speedRef)/1000;

    car->biasWeight = car->biasWeight - epsilon * error * (-1);
    if (car->steerRef > 8){
        car->leftSteerWeight = car->leftSteerWeight - epsilon * error * (((int)car->steerRef) - 9);
    } else {
        car->rightSteerWeight = car->rightSteerWeight - epsilon * error * (9 - ((int)car->steerRef));
    }

    car->speedWeight = car->speedWeight - epsilon * error * ((float)car->speed/1000.0);
    car->speedRefWeight = car->speedRefWeight - epsilon * error * ((float)car->speedRef/1000.0); 
}

void Car_udpateByControllerMessageAsPID(Car *car){
    float error = ((float)car->speed - (float)car->speedRef)/1000;
    car->intError += error;

    car->biasWeight = car->biasWeight - epsilon * error * error;
    car->speedWeight = car->speedWeight - epsilon * error * car->intError;
    car->speedRefWeight = 0;
    car->leftSteerWeight = 0;
    car->rightSteerWeight = 0;
}

float Car_getMotorSignalAsNeuron(Car *car){
    float ret = -car->biasWeight + car->speedWeight * ( ((float)car->speed)/1000.0) + car->speedRefWeight * ( ((float)car->speedRef)/1000.0);
    if (car->steerRef > 8){
        ret += car->leftSteerWeight * (car->steerRef - 9);
    } else {
        ret += car->rightSteerWeight * (9 - car->steerRef);
    }
    return ret;
}

float Car_getMotorSignalAsPID(Car *car){
    float error = ( ((float)car->speed)/1000.0) - (((float)car->speedRef) / 1000.0);
    float ret = car->biasWeight * error + car->speedWeight * car->intError; 
    return ret;
}

float Car_getMotorSignal(Car *car){
    if (CAR_NO_FEEDBACK) {
        return ((float)car->speedRef)/100.0;
    }

    if (USE_NEURON_NOT_PID){
        return Car_getMotorSignalAsNeuron(car);
    } else {
        return Car_getMotorSignalAsPID(car);
    }
}

void Car_udpateByControllerMessage(Car *car, unsigned char *message){
    car->speedRef = message[1]*256 + message[2];
    car->steerRef = message[3]*256 + message[4];

    if (message[0]){
        RingBuffer_putChar(&txUartBuffer, 'l');
        Car recovery = *car;
        float originalMotorSignal;
        float newMotorSignal;


        car->speed = message[5]*256 + message[6];
        if (USE_NEURON_NOT_PID){
            originalMotorSignal = Car_getMotorSignalAsNeuron(car);
            Car_udpateByControllerMessageAsNeuron(car);
            newMotorSignal = Car_getMotorSignalAsNeuron(car);
        } else {
            originalMotorSignal = Car_getMotorSignalAsPID(car);
            Car_udpateByControllerMessageAsPID(car);
            newMotorSignal = Car_getMotorSignalAsPID(car);
        }
        
        if ((originalMotorSignal > 1 && newMotorSignal > originalMotorSignal) || 
            (originalMotorSignal < 0 && newMotorSignal < originalMotorSignal)){
            *car = recovery;
            RingBuffer_putChar(&txUartBuffer, 'd');
        }

        char buf[50];
        RingBuffer_putString(&txUartBuffer, "State: ");
        sprintf(buf, "out: %d, outnew: %d, speed: %d, ref: %d", (int)(originalMotorSignal*1000), (int)(newMotorSignal*1000), car->speed, car->speedRef);
        RingBuffer_putString(&txUartBuffer, buf);
    }
}

void Car_udpateByDriverMessage(Car *car, unsigned char *message){
    memcpy(&(car->biasWeight), message, 4);
    memcpy(&(car->leftSteerWeight), message+4, 4);
    memcpy(&(car->rightSteerWeight), message+8, 4);
    memcpy(&(car->speedWeight), message+12, 4);
    memcpy(&(car->speedRefWeight), message+16, 4);
}

volatile Car car;

void separateKeyValuePair(char* source, char* key, char* value, char limiter){
    unsigned int i=0;
    while (limiter != source[i] && source[i]){
        key[i] = source[i];
        ++i;
    }

    key[i] = 0;
    if (source[i]){
        ++i;
        strcpy(value, source+i);
    } else {
        value[0] = 0;
    }
}

void controller(){
    initBoard(ENABLE_BUTTON_INTERRUPT | ENABLE_SERIAL_PORT | ENABLE_CC1101);
    initTimerB(1000, 0);
    __enable_interrupt();

    //RingBuffer_putString(&txUartBuffer, "Controller program started\n\r\n\r");

    initCC1101(); // Ehhez kellenek megszakítások
    //RingBuffer_putString(&txUartBuffer, "Init finished\n\r\n\r");

    setOutputLow(&GREEN_LED);
    setOutputLow(&RED_LED);
    
    enableRisingEdgeInterrupt(&CC1101_GD0); // Azelőtt ezt nem engedélyezhetem, hogy a CC1101 ne lenne inicializálva

    char waitingForResponse = 1;
    unsigned char responseReceivedCycleCounter = 0;
    unsigned char timerBCounter = 0;
    unsigned char speedIsFresh = 0;
    unsigned char speedIsSent = 0;
    TIMER_ON;
    while (1){
        if (RingBuffer_getCharPos(&rxUartBuffer, ',') < rxUartBuffer.bufferSize){
            char key[50];
            char value[50];
            char buffer[50];
            unsigned int len;
            
            len = RingBuffer_getContent(&rxUartBuffer, buffer, 50);
            buffer[len-1] = 0;
            separateKeyValuePair(buffer, key, value, ':');

            if (VERBOSE > 0){
                RingBuffer_putString(&txUartBuffer, "Key: |");
                RingBuffer_putString(&txUartBuffer, key);
                RingBuffer_putString(&txUartBuffer, "|      ");
                RingBuffer_putString(&txUartBuffer, "Value: |");
                RingBuffer_putString(&txUartBuffer, value);
                RingBuffer_putString(&txUartBuffer, "|\n\r");            
            }

            if (!strcmp(key, "setspeedref")){
                car.speedRef = atoi(value)*1000;
            } else if (!strcmp(key, "setspeed")) {
                car.speed = a2usFixpoint(value, 3);
                speedIsFresh = 1;
                speedIsSent = 0;
            } else if (!strcmp(key, "setsteer")) {
                car.steerRef = atoi(value);
            } else if (!strcmp(key, "getweights")) {
                if (USE_NEURON_NOT_PID){
                    // float ret = car->biasWeight * error + car->speedWeight * car->intError; 
                    sprintf(buffer, "%d,%d,%d,%d,%d\n\r", (int)(1000*car.biasWeight), 
                                                        (int)(1000*car.leftSteerWeight), 
                                                        (int)(1000*car.rightSteerWeight), 
                                                        (int)(1000*car.speedWeight),
                                                        (int)(1000*car.speedRefWeight));
                } else {
                    sprintf(buffer, "%d,%d\n\r", (int)(1000*car.biasWeight), 
                                                        (int)(1000*car.speedWeight));
                }
                RingBuffer_putString(&txUartBuffer, buffer);
            } else if (!strcmp(key, "getlabel")) {
                if (USE_NEURON_NOT_PID){
                    sprintf(buffer, "bias,leftSteer,rightSteer,speed,speedRef\n\r");
                } else {
                    sprintf(buffer, "P,I\n\r");
                }
                RingBuffer_putString(&txUartBuffer, buffer);
            }
        }

        if (buttonPressed){
            buttonPressed = 0;
        }

        if (packetReceivedOrSent){
            if (VERBOSE>0) RingBuffer_putChar(&txUartBuffer, '2');

            unsigned char res[2];
            burstReadRFReg(TI_CC_REG_RXBYTES, res, 1);
            if (res[0] & TI_CC_REG_NUM_RXBYTES){
                // Van adat az RX buffer-ben -> ez valószínűleg egy fogadás volt
                packetReceived = 1;
                if (VERBOSE>0) {
                    RingBuffer_putChar(&txUartBuffer, '3');
                    RingBuffer_putChar(&txUartBuffer, 'b');
                }
            } else {
                sendingInProgress = 0;
                if (VERBOSE>0) {
                    RingBuffer_putChar(&txUartBuffer, '3');
                    RingBuffer_putChar(&txUartBuffer, 'a');
                }
                packetSent = 1;
            }
            packetReceivedOrSent = 0;
        }

        if (!waitingForResponse) {
            ++responseReceivedCycleCounter;
        }

        if (timerBFlag) {
            ++timerBCounter;
            timerBFlag = 0;
        }

        if ((!waitingForResponse) || (timerBCounter > 2)){
            if (VERBOSE>0) RingBuffer_putChar(&txUartBuffer, '1');
            unsigned char buffer[20];
            Car_createControllerMessage(&car, buffer+1, speedIsFresh);
            speedIsSent = 1;

            buffer[0] = 7;
            sendPacket(buffer);
            setOutputHigh(&GREEN_LED);
            waitingForResponse = 1;
            timerBCounter = 0;
            responseReceivedCycleCounter = 0;
        }

        if (packetSent){
            packetSent = 0;
            if (VERBOSE>0) RingBuffer_putChar(&txUartBuffer, '4');            
            setOutputLow(&GREEN_LED);
        }

        if (packetReceived){
            unsigned char len;
            {
                char buffer[50];
                len = recPacket(buffer, 25);
                //if (len == 7){
                    Car_udpateByDriverMessage(&car, buffer);
                    if (speedIsSent){
                        speedIsFresh = 0;
                    }
                //} else if (VERBOSE>0) {
                //    sprintf(buffer, "VAL: %d\n", len);
                //    RingBuffer_putString(&txUartBuffer, buffer);
                //}
            }
            packetReceived = 0;
            waitingForResponse = 0; 
        }        
    }

}

void steerControl(unsigned int position){
    // Ezekkel a beállításokkal: position \in [0, 17]
    
    static unsigned int cycleCounter = 0;
    const unsigned int maxTime = 2000; // Ekkora szünet kell két impulzus között. (20ms)

    const unsigned int mostRight = 48;
    //const unsigned int mostLeftMs = 27;
    const unsigned int upTime = mostRight + position;

    if (cycleCounter < upTime){
        setOutputHigh(&SERVO_CONTROL);
    } else {
        setOutputLow(&SERVO_CONTROL);
    }
    
    ++cycleCounter;
    if (cycleCounter > maxTime){
        cycleCounter = 0;
    }
}

void speedControl(unsigned int speed){
    static unsigned int actTime = 0;
    const unsigned int maxTime = 100;
    
    if (VERBOSE >= 2){
        static unsigned int divider = 0;
        if (divider > 40000){
            char buffer[20];
            sprintf(buffer, "Car> SR: %d\n\r", speed);
            RingBuffer_putString(&txUartBuffer, buffer);
            divider = 0;
        }
        divider++;
    }

    actTime++;
    if (speed > actTime){
        setOutputHigh(&FORWARD);
    } else {
        setOutputLow(&FORWARD);
    }

    if (actTime > maxTime){
        actTime = 0;
    }
}

volatile unsigned int speed = 0;
void driverTimerEvent(){
    steerControl(car.steerRef);
    speedControl(speed);
    //speedControl(car.speedRef);
}

void driver(){
    initBoard(ENABLE_BUTTON_INTERRUPT | ENABLE_SERIAL_PORT | ENABLE_CC1101);
    initTimerB(500, 1);
    setOutputLow(&FORWARD);
    __enable_interrupt();

    RingBuffer_putString(&txUartBuffer, "Car program started\n\r\n\r");

    initCC1101(); // Ehhez kellenek megszakítások
    RingBuffer_putString(&txUartBuffer, "initCC1101 finished\n\r\n\r");

    setOutputLow(&SERVO_CONTROL);

    setOutputLow(&GREEN_LED);
    setOutputLow(&RED_LED);
    
    enableRisingEdgeInterrupt(&CC1101_GD0); // Azelőtt ezt nem engedélyezhetem, hogy a CC1101 ne lenne inicializálva

    setOutputLow(&FORWARD);
    
    __delay_cycles(16000000);

    RingBuffer_putString(&txUartBuffer, "Init finished\n\r\n\r");
    char sendAnswer = 0;
    unsigned int timerBCounter = 0;
    car.intError = 0;
    TIMER_ON;
    while (1){
        if (buttonPressed){
            buttonPressed = 0;
        }

        if (timerBFlag) {
            ++timerBCounter;
            timerBFlag = 0;
        }

        if (sendAnswer && (timerBCounter>=1000)){
            char message[30];
            message[0] = 20;
            Car_createDriverMessage(&car, message+1);
            sendPacket(message);
            setOutputHigh(&GREEN_LED);
            if (VERBOSE >= 2){
                RingBuffer_putString(&txUartBuffer, "Car> Resp\n\r");
            }
            setOutputHigh(&RED_LED);
            sendAnswer = 0;
            timerBCounter = 0;
        }

        if (packetReceivedOrSent){
            char res[2];
            burstReadRFReg(TI_CC_REG_RXBYTES, res, 1);
            if (res[0] & TI_CC_REG_NUM_RXBYTES){
                // Van adat az RX buffer-ben -> ez valószínűleg egy fogadás volt
                packetReceived = 1;
            } else {
                sendingInProgress = 0;
                packetSent = 1;
            }
            packetReceivedOrSent = 0;
        }

        if (packetSent){
            setOutputLow(&GREEN_LED);
            packetSent = 0;
        }

        if (packetReceived){
            setOutputLow(&RED_LED);
            unsigned char buffer[50];
            buffer[0] = 0;
            unsigned char len = recPacket(buffer, 10);
            Car_udpateByControllerMessage(&car, buffer);

            float ratio = Car_getMotorSignal(&car);

            if (ratio > 1) ratio = 1.0;
            if (ratio < 0) ratio = 0.0;
            speed = ratio * 100;

            static char printerCounter = 0;
            if (++printerCounter > 5 && VERBOSE >= 2){
                sprintf(buffer, "s:%d\n\r", speed);
                RingBuffer_putString(&txUartBuffer, buffer);
                printerCounter = 0;
            }

            sendAnswer = 1;
            packetReceived = 0; 
        }
    }
}

int main(){
    if (DRIVER_CODE){
        driver();
    } else {
        controller();
    }
    
}

__attribute__((interrupt(PORT1_VECTOR)))
void buttonInterrupt(){
    if (testInterruptFlag(&BUTTON)){
        buttonPressed = 1;
        clearInterruptFlag(&BUTTON);   
    }
}

__attribute__((interrupt(PORT2_VECTOR)))
void CC1101Interrupt(){
    if (testInterruptFlag(&CC1101_GD0)){
        packetReceivedOrSent = 1; // Ez bejön küldés után is!!!
        clearInterruptFlag(&CC1101_GD0);
    }
}

__attribute__((interrupt(TIMERB0_VECTOR)))
void TimerB(void){
    if (DRIVER_CODE){
        driverTimerEvent();
    }
    timerBFlag = 1;
}
