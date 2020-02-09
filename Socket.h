#ifndef SOCKET_H
#define SOCKET_H

#include "RingBuffer.h"

typedef struct SocketStruct{
    RingBuffer input;
    RingBuffer output;
} Socket;

unsigned int getBytesInBuffer(Socket *socket);

char getChar(Socket *socket);
void getString(Socket *socket, char *buffer, unsigned int bufferSize, char delimiter);
char peekNextChar(Socket *socket);

void putChar(Socket *socket, char data);
void putString(Socket *socket, char *data);


    
#endif
