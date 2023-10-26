#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdio.h>
//#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include "message.h"
#include "consts.h"
#include "fastmutex.h"




class RingBuffer
{
public:
        RingBuffer( int incomingWaitingType );
        ~RingBuffer();

        /**
            Add data to ringbuffer
        */
        void AddData(Message* incomingMessage);
        void AddDataInterlocked(Message* incomingMessage);

        /**
          Return data from buffer
        */
        Message* GetData();

        Message* GetDataInterlocked();
private:
        void Update_State();
        int GetConsumerPointer();
        int GetPointer(int* index, FastMutex* wrappingSynchronizator);
//        Message* GetDataInterlocked();

        int waitingType;
        unsigned char *m_load_ptr, *m_consume_ptr;
        unsigned char *m_buff_end;
        unsigned char *m_buff;
        int m_max_load, m_max_consume, m_data_in_buffer;

        Message* data [BUFFER1_SIZE];
        int consumerIndex;
        int producerIndex;
        int lastIndex;

        int usedSlots;//number of buffered messages in array
        int freeSlots;//free slots for producer

        sem_t empty;// number of elements
        sem_t full;// 0
        pthread_mutex_t consumerMutex;
        pthread_mutex_t producerMutex;
        pthread_mutex_t updatePointersMutex;

        FastMutex* consumerWrappingSynchronizator;
        FastMutex* consumerPointerSynchronizator;

        FastMutex* producerWrappingSynchronizator;
        FastMutex* producerPointerSynchronizator;

        int CheckPointer(int);

        void UpdatePointers();
};


#endif // RINGBUFFER_H
