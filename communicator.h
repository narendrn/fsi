#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "consts.h"
#include "message.h"


extern int mode;

class Communicator
{
public:
    Communicator(int protocol, int socketPurpose, int portNum, const char * ipAddress);
    ~Communicator();

    int GetSocketDescriptor();
    void CreateSocket();

    void SendMessage(Message *messageForSend);
    void SendVTclientinitString(const char* clientString);

    Message *  ReceiveMessageFromSender();
    const char* ReceiveStringFromSender();

private:
    struct sockaddr_in localAddress, remoteAddress;
    int socketFileDescriptor;

    void Bind();
    void Send(void* objectForSend, int lenghtOfObject);
    void SendMessage(Message *messageForSend, const char* ipAddress );

};

#endif // COMMUNICATOR_H
