#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef volatile struct RingBufferStruct{
    unsigned int counter;
    char overflow;
    char underflow;
    unsigned int writeIndex;
    unsigned int readIndex;
    char *buffer;
    unsigned int bufferSize;
    char empty;
    
    volatile char *isFinalDestinationIsUsed;
    volatile unsigned char *finalDestination;
} RingBuffer;

void RingBuffer_init(RingBuffer volatile * volatile ringBuffer, char *buffer, unsigned int bufferSize);
void RingBuffer_clear(RingBuffer volatile * volatile ringBuffer);
void RingBuffer_blockingPutChar(RingBuffer volatile * volatile ringBuffer, char c);
void RingBuffer_putChar(RingBuffer volatile * volatile ringBuffer, char c);
char* RingBuffer_putString(RingBuffer volatile * volatile ringBuffer, char *str);
char RingBuffer_getChar(RingBuffer volatile * volatile ringBuffer);
unsigned int RingBuffer_getCharNumber(RingBuffer volatile * volatile ringBuffer);
void RingBuffer_flushUntil(RingBuffer volatile * volatile ringBuffer, char c);
char RingBuffer_checkNextChar(RingBuffer volatile * volatile ringBuffer);
unsigned int RingBuffer_getContent(RingBuffer volatile * volatile ringBuffer, char *buffer, unsigned int maxSize);
unsigned int RingBuffer_getContentUntil(RingBuffer volatile * volatile ringBuffer, char *buffer, char limiter, unsigned int maxSize);   
unsigned int RingBuffer_getCharPos(RingBuffer volatile * volatile ringBufferr, char c);
unsigned int RingBuffer_getSequencePos(RingBuffer volatile * volatile ringBuffer, char *sequence);

#endif