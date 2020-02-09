#include "RingBuffer.h"
#include "hardware_board.h"

#define CIRCLE_STEP(actVal, upperLimit) ((actVal) = ((actVal)+1 >= (upperLimit))?0:(actVal)+1)

void RingBuffer_init(volatile RingBuffer *ringBuffer, char *buffer, unsigned int bufferSize){
    ringBuffer->overflow = 0;
    ringBuffer->underflow = 0;
    ringBuffer->writeIndex = 0;
    ringBuffer->readIndex = 0;
    ringBuffer->empty = 1;
    ringBuffer->buffer = buffer;
    ringBuffer->bufferSize = bufferSize;
    
    ringBuffer->isFinalDestinationIsUsed = 0;
    ringBuffer->finalDestination = 0;
}

void RingBuffer_blockingPutChar(RingBuffer volatile * volatile ringBuffer, char c){
    __delay_cycles(10); // Nem igazán értem, hogy itt miért van szükség néhány órajelnyi várakozásra
    do {
        RingBuffer_putChar(ringBuffer, c);
    } while (ringBuffer->overflow);
    __delay_cycles(10);
}

void RingBuffer_putChar(RingBuffer volatile * volatile ringBuffer, char c){
    ringBuffer->overflow = 0;
    if ((ringBuffer->isFinalDestinationIsUsed && (*ringBuffer->isFinalDestinationIsUsed) ) || !ringBuffer->isFinalDestinationIsUsed){

        if ( ringBuffer->writeIndex == ringBuffer->readIndex && !ringBuffer->empty ){
            ringBuffer->overflow = 1;
            return;
        }
        ringBuffer->empty = 0;
        ringBuffer->buffer[ringBuffer->writeIndex] = c;
        
        CIRCLE_STEP(ringBuffer->writeIndex, ringBuffer->bufferSize);
    } else {
        *(ringBuffer->isFinalDestinationIsUsed) = 1;
        *(ringBuffer->finalDestination) = c;
    }
}

char* RingBuffer_putString(RingBuffer volatile * volatile ringBuffer, char *str){
    while (*str){
        RingBuffer_putChar(ringBuffer, *str);
        if (ringBuffer->overflow){
            return str;
        }
        ++str;
    }
    return str;
}

void RingBuffer_clear(RingBuffer volatile * volatile ringBuffer){
    ringBuffer->writeIndex = 0;
    ringBuffer->readIndex = 0;
    ringBuffer->empty = 1;
}

char RingBuffer_getChar(RingBuffer volatile * volatile ringBuffer){
    if (ringBuffer->empty){
        ringBuffer->underflow = 1;
        return 0;
    }
    
    char ret = ringBuffer->buffer[ringBuffer->readIndex];
    CIRCLE_STEP(ringBuffer->readIndex, ringBuffer->bufferSize);
    
    if (ringBuffer->writeIndex == ringBuffer->readIndex){
        ringBuffer->empty = 1;
    }
    
    return ret;
}

void RingBuffer_flushUntil(RingBuffer volatile * volatile ringBuffer, char c){
    while (RingBuffer_checkNextChar(ringBuffer) != c && !ringBuffer->empty){
        RingBuffer_getChar(ringBuffer);
    }
    RingBuffer_getChar(ringBuffer);
}

unsigned int RingBuffer_getCharNumber(RingBuffer volatile * volatile ringBuffer){
    if (ringBuffer->readIndex < ringBuffer->writeIndex){
        return ringBuffer->writeIndex - ringBuffer->readIndex;
    } else if (ringBuffer->readIndex > ringBuffer->writeIndex){
        return ringBuffer->bufferSize - ringBuffer->readIndex + ringBuffer->writeIndex;
    } else if (ringBuffer->empty){
        return 0;
    } else {
        return ringBuffer->bufferSize;
    }
}

char RingBuffer_checkNextChar(RingBuffer volatile * volatile ringBuffer){
    return ringBuffer->buffer[ringBuffer->readIndex];
}

unsigned int RingBuffer_getContent(RingBuffer volatile * volatile ringBuffer, char *buffer, unsigned int maxSize){
    unsigned int ringBufferSize = RingBuffer_getCharNumber(ringBuffer);
    ringBufferSize = (ringBufferSize<=maxSize)?ringBufferSize:maxSize;
    for (unsigned int i=0; i<ringBufferSize; ++i){
        buffer[i] = RingBuffer_getChar(ringBuffer);
    }
    return ringBufferSize;
}

unsigned int RingBuffer_getContentUntil(RingBuffer volatile * volatile ringBuffer, char *buffer, char limiter, unsigned int maxSize){
    unsigned int ringBufferSize = RingBuffer_getCharNumber(ringBuffer);
    ringBufferSize = (ringBufferSize<=maxSize)?ringBufferSize:maxSize;
    for (unsigned int i=0; i<ringBufferSize; ++i){
        if (RingBuffer_checkNextChar(ringBuffer) == limiter){
            buffer[i] = RingBuffer_getChar(ringBuffer);
            return i+1;
        }
        buffer[i] = RingBuffer_getChar(ringBuffer);
    }
    return ringBufferSize;
}

unsigned int RingBuffer_getCharPos(RingBuffer volatile * volatile ringBuffer, char c){
    unsigned int pos = ringBuffer->readIndex;
    unsigned int ret = 0;
    if (ringBuffer->readIndex == ringBuffer->writeIndex && !ringBuffer->empty){
        if (ringBuffer->buffer[pos] == c) {
            return 0;
        }
        ++ret;
        CIRCLE_STEP(pos, ringBuffer->bufferSize);
    }
    while (pos != ringBuffer->writeIndex){
        if (ringBuffer->buffer[pos] == c){
            if (pos-ringBuffer->readIndex >= 0){
                return ret; //pos-ringBuffer->readIndex; Törölhető 2018 május 1 után
            } else {
                return ret; //pos + (ringBuffer->bufferSize - ringBuffer->readIndex);
            }
        }
        ++ret;
        CIRCLE_STEP(pos, ringBuffer->bufferSize);
    }
    return ringBuffer->bufferSize;
}

unsigned int RingBuffer_getSequencePos(RingBuffer volatile * volatile ringBuffer, char *sequence){
    if (ringBuffer->empty){
        return ringBuffer->bufferSize+1;
    }
    unsigned int counter = 0;
    unsigned int i = ringBuffer->readIndex;
    unsigned int found = 0;
    do {
        if (sequence[found] == ringBuffer->buffer[i]){
            ++found;
            if (!sequence[found]) return counter;
        }
        ++counter;
        CIRCLE_STEP(i, ringBuffer->bufferSize);
    } while (i != ringBuffer->writeIndex);
    return ringBuffer->bufferSize+1;
}
