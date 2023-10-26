#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <signal.h>

#include <pthread.h>
#include "consts.h"
#include "communicator.h"
#include "ringbuffer.h"
#include "itac_instrumentator.h"

#include "VT.h"

int             g_mode                      =RECEIVER_MODE_1_PROCESS;
int             g_portNumber                =PORT_NUMBER;
int             g_auxiliaryPortNumber       =VT_STRING_PORT_NUMBER;
int             g_numberOfMessagesForSend   =NUMBER_OF_MESSAGES_FOR_SEND;
int             g_processRank               =-100;
Communicator*   g_communicator              =NULL;
//Communicator*   g_auxiliaryCommunicator     =NULL;
pthread_mutex_t g_consumerIncrementMutex    = PTHREAD_MUTEX_INITIALIZER;
int             g_msg_size                  = MESSAGE_SIZE;
int             g_msg_rate                  = MESSAGE_RATE;
int             g_msg_block                 = MESSAGE_BLOCK;
volatile bool   g_isMessagesFinish          =false;
bool            g_isDebug                   =false;
int             g_sync_mode                 = SEMAPHORE;
using namespace std;

typedef struct
{
    RingBuffer* buffer1;
    RingBuffer* buffer2;
} buffersContainer;

struct timeval g_tv1;
struct timeval g_tv2;
struct timeval g_tv0;
uint64_t g_clock1,  g_clock_t;

unsigned long g_mid[1000000];
uint64_t g_clock[1000000];
uint64_t g_con[1000000];
uint64_t g_pub[1000000];
int g_count = 0;
int freq = 3066;

inline void rdtscll(uint64_t* n) {
#ifdef __x86_64__
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    *n = (uint64_t) (lo) | ((uint64_t) (hi) << 32);
#else
    __asm__ __volatile__ ("rdtsc" : "=A" (*n));
#endif
}

void nsleep(uint64_t clock)
{
    uint64_t clock1, clock2;
    rdtscll(&clock1);
    while(1){
        rdtscll(&clock2);
         if(clock2 - clock1 > clock){
//         printf("cl2- cl1  clock_delay = %ld  %ld\n", clock2-clock1,  clock);
           return;
         }
    }
}

/**
Output thread
**********************/
void *OutputFunction(void* param)
{
    int i;
    FILE* pFile;
    
    for(i=0; i<g_count; i++)
    {
        printf("R:m %8lu rec %llx  con %llx  pub %llx  lat=%d\n" , g_mid[i], g_clock[i], g_con[i], g_pub[i], (g_pub[i]-g_clock[i])/freq);
    }
    printf("Total number of messages received is  %d\n" , g_count);
   
    pFile = fopen("/root/time_intervals.bin", "wb+");
    if(!pFile)
        return 0;
    printf("Writing to file /root/time_intervals.bin\n");
    for(i=0; i<g_count; i++)
    {
        fwrite(&(g_clock[i]),sizeof(uint64_t),1,pFile);
        fwrite(&( g_pub[i]),sizeof(uint64_t),1,pFile);
    }
    fclose(pFile);
    
    fflush(stdout);
    sync();
    return 0;
}

/**
Receiver thread
**********************/
void *ReceiverFunction(void* param)
{
    RingBuffer* buffer1 = (RingBuffer*) param;
    Message* receivedMessage = NULL;
    uint64_t clock0, clock;
//  if we want receiver to publish immediately  
//	Communicator* communicator = new Communicator(UDP, SEND, g_portNumber, OUTPUT_IP);
    for(;;)
    {
        receivedMessage = g_communicator->ReceiveMessageFromSender();
        rdtscll(&clock0);
        g_clock[receivedMessage->GetBufferWithData()->MessageID] = clock0;
        //printf("R:m %8u r %ld\n" , 
        //      receivedMessage->GetBufferWithData()->MessageID, clock0);
        g_mid[g_count] = receivedMessage->GetBufferWithData()->MessageID;
        buffer1->AddData(receivedMessage);
        rdtscll(&clock);
//        printf("Publisher: MessageID sent from buffer2: %lu     rdtsc = %lx\n",
//                receivedMessage->GetBufferWithData()->MessageID, clock);

//  if we want receiver to publish immediately  
//        communicator->SendMessage(receivedMessage);
        receivedMessage=NULL;
        g_count++;
    }
    printf("Receice process finished\n");
    fflush(stdout);
    sync();
    return 0;
}


/**
Consumer
**********************/
int compareShared(volatile int* local_i)
{
    int i;
    pthread_mutex_lock(&g_consumerIncrementMutex);
    i=++(*local_i);// local_i is incremented
    pthread_mutex_unlock(&g_consumerIncrementMutex);
    return i;
}

void *ConsumerFunction(void* param)
{
    buffersContainer* buffers = (buffersContainer*)param;
    RingBuffer* buffer1 = buffers->buffer1;
    RingBuffer* buffer2 = buffers->buffer2;
    uint64_t clock;  
    int i, a,b,c;  
    for(;;)
    {
        Message* messageFromBuffer = buffer1->GetData();
        rdtscll(&clock);
        g_con[messageFromBuffer->GetBufferWithData()->MessageID] = clock;
        //printf("Consumer: MessageID from buffer1: %lu          rdtsc = %lx\n",
        //        messageFromBuffer->GetBufferWithData()->MessageID, clock);
//introduce a delay
        //nsleep(2930*20);//2930*10 is a saturation point for 100K msg/sec 
//        asm("movapd %xmm1 %xmm1");
        for(i=0;i<20000;i++)
          a=b+c;
        buffer2->AddData(messageFromBuffer);
//        rdtscll(&clock);
//        printf("Consumer:msg added                             rdtsc = %lx\n",
//                messageFromBuffer->GetBufferWithData()->MessageID, clock);
    }
    return 0;
}

/**
Publisher
**********************/
void *PublisherFunction(void* param)
{
    RingBuffer* buffer2 = (RingBuffer*) param;
    Message* messageForSend = NULL;
    uint64_t clock;
    Communicator* communicator = new Communicator(UDP, SEND, g_portNumber, OUTPUT_IP);
    for(;;)
    {
        messageForSend = buffer2->GetData();
        communicator->SendMessage(messageForSend);
        rdtscll(&clock); 
        g_pub[messageForSend->GetBufferWithData()->MessageID] = clock;
//        g_count++;    
//    printf("Publisher: MessageID sent from buffer2: %lu     rdtsc = %lx\n", 
//            messageForSend->GetBufferWithData()->MessageID, clock);

        delete messageForSend;
        messageForSend = NULL;
    }
    delete communicator;
    return 0;
}

/**
Auxiliariator
**********************/
void *AuxiliariatorFunction(void* param)
{
    pthread_t* victimThread = (pthread_t*)param;
    Communicator* auxiliaryCommunicator = new Communicator(UDP, RECEIVE, g_auxiliaryPortNumber, NULL);
    Message* receivedMessage = NULL;
    while(!g_isMessagesFinish)
    {
        receivedMessage = auxiliaryCommunicator->ReceiveMessageFromSender();
        if(receivedMessage->isAuxiliary())
        {
            g_isMessagesFinish=true;
            pthread_cancel(*victimThread);
            //DROP Receiver thread;
        }
        receivedMessage=NULL;
    }
    printf("Auxiliary process finished\n");
    return 0;

}

/**
ITAC
  Registration of clientinit strings
**********************/  
void RegisterClientInit()
{
    printf("We are going tp create clientinit strings\n");
    if (g_mode==SENDER_MODE)
    {

        g_processRank = 1;
        const char* clientInitString = ITAC_Instrumentator::CreateClientInitString("Sender", g_processRank);
        g_communicator->SendVTclientinitString(clientInitString);
    }else if(g_mode==RECEIVER_MODE_1_PROCESS)
    {
        ITAC_Instrumentator* itacInstrumentator = new ITAC_Instrumentator();
        g_processRank = 0;
        const char* localClientInitString = ITAC_Instrumentator::CreateClientInitString("Framework", g_processRank);
        const char* receivedClientInitString = g_communicator->ReceiveStringFromSender();
        itacInstrumentator->CollectClientInitString(localClientInitString);
        itacInstrumentator->CollectClientInitString(receivedClientInitString);
        itacInstrumentator->RegisterClientsString();
    }

}

void NotifyAboutEnd()
{
    Message* messageFinite = new Message();
    messageFinite->BecameAuxiliary();
    Communicator* auxiliaryCommunicator = new Communicator(UDP, SEND,g_auxiliaryPortNumber ,RECEIVER_IP_ADDRESS);
    auxiliaryCommunicator->SendMessage(messageFinite);
}

/**
Sender thread
**********************/
void SendMessages()
{

    if(g_isDebug)
    {
        Communicator* communicator = new Communicator(UDP, SEND, g_portNumber, RECEIVER_IP_ADDRESS);
        //    communicator->CreateSocket();
        Message* messageForSend = NULL;
        for(int i=0; i<g_numberOfMessagesForSend; i++)
        {
            messageForSend = new Message();
            messageForSend->SetMessageID(i);
            //        messageForSend->SetMessageID(1505407);
            communicator->SendMessage(messageForSend);
            printf("Message sent\n");
            delete messageForSend;
            sleep(0.01);
        }
    }
    else
    {

        unsigned long long usec;
        Message* messageForSend = NULL;
        int msg_count = 0;
        int msg_count_print = 0;
        struct timeval tv_prev;
        tv_prev.tv_sec = 1111111;

		uint64_t clock0, clock2;
		uint64_t clock_delay, clock_d;
		clock_delay = 3000000000 /  g_msg_rate;
		usec = g_msg_block *1000000 / g_msg_rate;
		gettimeofday(&g_tv0, NULL);
	        
		rdtscll(&g_clock_t);
		rdtscll(&g_clock1);
		for(int i=0; i<g_numberOfMessagesForSend; i++)
		{
			messageForSend = new Message(g_msg_size);
			messageForSend->SetMessageID(i);
			g_communicator->SendMessage(messageForSend);
			rdtscll(&clock0);
			g_clock[i] = clock0;
	        //printf("S:m %8d r %ld\n", i, clock0);
			delete messageForSend;
			msg_count++;
			msg_count_print++;
			rdtscll(&clock2);
	        // printf("cl2- cl1 = %ld \n", clock2-g_clock1);
			if(clock2-g_clock1<clock_delay-150){
			   clock_d = clock_delay - 150 -(clock2-g_clock1);
			   nsleep(clock_d);
			}
			rdtscll(&g_clock1);
		}
		gettimeofday(&g_tv1, NULL);

        NotifyAboutEnd();//send information to receiver about end.

		for(int i=0; i<g_numberOfMessagesForSend; i++)
		{
			printf("S:m %8d r %ld\n", i, g_clock[i]);
		}
		printf("rdtsc-time sqew = %ld percent\n",  ((g_clock1-g_clock_t)/3000)/
				( (1000000*(g_tv1.tv_sec  - g_tv0.tv_sec) + g_tv1.tv_usec - g_tv0.tv_usec)) );
		printf("Send process finished: total %d messages \n", g_numberOfMessagesForSend);
		printf("Average rate is %ld msg per sec\n",
				1000* g_numberOfMessagesForSend/
				( (1000000*(g_tv1.tv_sec  - g_tv0.tv_sec) + g_tv1.tv_usec - g_tv0.tv_usec)/1000) );
		}
}


int main(int argc, char *argv[])
{
    int opt;
    if(argc == 1)
    {
        //portNumber = PORTNUM;
        g_mode = RECEIVER_MODE_1_PROCESS;
        //                packetsType = TCP;
        g_portNumber = PORT_NUMBER;
    }
    else
    {
        /**
            The getopt function gets the next option argument from the argument list specified by the argv and argc arguments.
            Normally these values come directly from the arguments received by main.
            The options argument is a string that specifies the option characters that are valid for this program.
            An option character in this string can be followed by a colon (‘:’) to indicate that it takes a required argument.
            If an option character is followed by two colons (‘::’), its argument is optional; this is a GNU extension.
            */
        while((opt = getopt(argc, argv, "p:rsn:d")) != -1)
        {
            switch(opt)
            {
                            case 'p':
                g_portNumber = atoi(optarg);
                break;
                            case 'r':
                //mode = PING_SERVER;
                g_mode = RECEIVER_MODE_1_PROCESS;
                break;
                            case 's':
                g_mode = SENDER_MODE;
                break;
                            case 'n':
                g_numberOfMessagesForSend = atoi(optarg);
                break;
                            case 'd':
                g_isDebug = true;
                break;


            }
        }
    }

    std::cout << "Welcome to LatencyFramework" << std::endl;

    if (g_mode==SENDER_MODE)
    {

        g_communicator = new Communicator(UDP, SEND, g_portNumber, RECEIVER_IP_ADDRESS);
        printf("We are in SENDER_MODE\n");
        printf("Number of messages to send = %d\n", g_numberOfMessagesForSend);
        printf("Message rate = %d\n", g_msg_rate);
        printf("Message size = %d\n", g_msg_size);
        SendMessages();

    }else if(g_mode==RECEIVER_MODE_1_PROCESS)
    {
        g_communicator = new Communicator(UDP, RECEIVE, g_portNumber, NULL);
        RingBuffer* buffer1 = NULL;
        RingBuffer* buffer2 = NULL;
        if(g_sync_mode)
        {
            buffer1 = new RingBuffer(SEMAPHORE);
            buffer2 = new RingBuffer(SEMAPHORE);
        }else
        {
            buffer1 = new RingBuffer(ACTIVE_WAITING);
            buffer2 = new RingBuffer(ACTIVE_WAITING);
        }

        buffersContainer buffers12;
        buffers12.buffer1 = buffer1;
        buffers12.buffer2 = buffer2;

        printf("We are in RECEIVER_MODE_1_PROCESS\n");
        pthread_t receiverThread, consumerThread, publisherThread, auxiliaryThread;
        pthread_t consumerThread1;
        pthread_t consumerThread2;
        pthread_t outputThread;

        pthread_create(&receiverThread, NULL, ReceiverFunction, (void*)buffer1 /*(void*)buffer1*/);
        pthread_create(&consumerThread, NULL, ConsumerFunction, (void*)&buffers12/*(void*)buffer1*/);
        pthread_create(&consumerThread1, NULL, ConsumerFunction, (void*)&buffers12/*(void*)buffer1*/);
        pthread_create(&consumerThread2, NULL, ConsumerFunction, (void*)&buffers12/*(void*)buffer1*/);
        //pthread_create(&auxiliaryThread, NULL, AuxiliariatorFunction, (void*)&receiverThread);
        pthread_create(&publisherThread, NULL, PublisherFunction, (void*)buffer2/*(void*)buffer1*/);

        // run until the user hits control-d 
        while (getchar() != EOF) {
        // sleep for a bit to keep from consuming too much cpu
            usleep(100);
         }
        pthread_create(&outputThread, NULL, OutputFunction, (void*)buffer2);
        pthread_join(outputThread, NULL);
        pthread_join(receiverThread, NULL);
        pthread_join(consumerThread, NULL);
        pthread_join(consumerThread1, NULL);
        pthread_join(consumerThread2, NULL);
        pthread_join(publisherThread, NULL);
        //pthread_join(auxiliaryThread, NULL);
//        pthread_create(&outputThread, NULL, OutputFunction, (void*)buffer2);
//        pthread_join(outputThread, NULL);
    }
    delete g_communicator;
    g_communicator = NULL;
    return (0);
}
