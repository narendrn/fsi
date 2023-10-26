#include "communicator.h"


Communicator::Communicator(int protocol, int socketPurpose, int portNum, const char * ipAddress)
{
    if (protocol==UDP)
    {
        if ((this->socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("Cant open datagram socket.\n");
            exit(0);
        }
        bzero((char *) &this->localAddress, sizeof(localAddress));
        bzero((char *) &this->remoteAddress, sizeof(remoteAddress));
        if(socketPurpose==SEND)
        {
            this->remoteAddress.sin_family = PF_INET;
            this->remoteAddress.sin_port = htons(portNum);
             if(inet_aton(ipAddress, &this->remoteAddress.sin_addr)==0)
                perror("inet_aton() failed");
        }
        if(socketPurpose==RECEIVE)
        {
            this->localAddress.sin_family = PF_INET;
            this->localAddress.sin_addr.s_addr = INADDR_ANY;
            this->localAddress.sin_port = htons(portNum);
            Bind();
        }
    }
    if(protocol == TCP)
    {

    }
}
Communicator::~Communicator()
{
    close(this->socketFileDescriptor);

}
void Communicator::Bind()
{
    int err = 0;
    if ( (err = bind(this->socketFileDescriptor, (struct sockaddr *) &this->localAddress, sizeof(localAddress)))
        < 0)
    {
        printf("Cant bind local address. %d -- %d\n",err,INADDR_ANY);
        exit(1);
    }
    
    int opt_val = 1;
    setsockopt(this->socketFileDescriptor, SOL_SOCKET, SO_TIMESTAMP, &opt_val, sizeof(int));
    printf(" Connected to port : %d\n", ntohs(this->localAddress.sin_port));
}

void Communicator::Send(void* objectForSend, int lenghtOfObject)
{
   int clilen = sizeof(this->remoteAddress);

    if ((sendto(this->socketFileDescriptor, objectForSend, lenghtOfObject, 0, (struct sockaddr *) &this->remoteAddress, clilen)) < 0)
    {
        printf("ERROR Sending DGRAM\n");
    }
}


void Communicator::SendMessage(Message *messageForSend)
{
    int lenghtOfMessage;
    int clilen = sizeof(this->remoteAddress);
    if(messageForSend->GetvBuffer()){
        lenghtOfMessage = messageForSend->Getvmsg_size();

        if ((sendto(this->socketFileDescriptor, messageForSend->GetvBuffer(), lenghtOfMessage, 0,
                    (struct sockaddr *) &this->remoteAddress, clilen)) < 0)
        {
            printf("ERROR Sending vDGRAM\n");
        }

    }
    else
    {
        lenghtOfMessage = sizeof(Message);
        /*
      sendto()
       Функция sendto() посылает сообщение указанному адресату, не подключаясь
    к нему. Обычно эта функция используется для отправки дейтафамм и неструкту
    рированных пакетов, а также в протоколе Т/ТСР (Transaction TCP), который,
    впрочем, еще не реализован в Linux.
    Прототип
    #include <sys/socket.h>
    #include <resolv.h>
    int sendto(int sockfd, void* msg, int len, int options,
               struct sockaddr *addr, int addr_len);
    */
        MessageBuffer* bufferForSend = messageForSend->GetBufferWithData();

        if ((sendto(this->socketFileDescriptor, (void *) bufferForSend, lenghtOfMessage, 0,
                    (struct sockaddr *) &this->remoteAddress, clilen)) < 0)
        {
            printf("ERROR Sending DGRAM\n");
        }
    }
}

/**
  This function can cause bug, after first call it replaces ipAddress field at the sin_addr
  */
void Communicator::SendMessage(Message *messageForSend, const char* ipAddress )
{
    int lenghtOfMessage = sizeof(Message);
    int clilen = sizeof(this->remoteAddress);
    MessageBuffer* bufferForSend = messageForSend->GetBufferWithData();

    if(inet_aton(ipAddress, &this->remoteAddress.sin_addr)==0)
       perror("inet_aton() failed");

    if ((sendto(this->socketFileDescriptor, (void *) bufferForSend, lenghtOfMessage, 0, (struct sockaddr *) &this->remoteAddress, clilen)) < 0)
    {
        printf("ERROR Sending DGRAM\n");
    }



}
void Communicator::SendVTclientinitString(const char* clientString)
{
    Send((void*)clientString, strlen(clientString));
}

/**
    Returning Message* with received data by recv()
*/
Message * Communicator::ReceiveMessageFromSender()
{
    int len;
    Message* messageForReceive = new Message();
    MessageBuffer receivedData;
    if (( len = recv(this->socketFileDescriptor, (void* )&receivedData, sizeof(MessageBuffer),0))<0)
    {
        perror("Didn't received");
    }
    messageForReceive->AssignReceivedData(&receivedData);
    return messageForReceive;
}

const char* Communicator::ReceiveStringFromSender()
{
    int len;
    const char* receivedData = new char[26];

    if (( len = recv(this->socketFileDescriptor, (void* )receivedData, 1024,0))<0)//почему передается (void*)????
    {
        perror("Didn't received");
    }
    return receivedData;
}
