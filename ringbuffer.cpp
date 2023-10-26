#include "ringbuffer.h"

RingBuffer::RingBuffer( /*void* buffer, unsigned int buffer_size*/ int incomingWaitingType)
{

    this->producerIndex=0;
    this->consumerIndex=0;
    this->lastIndex = BUFFER1_SIZE - 1;
    pthread_mutex_init(&this->consumerMutex, NULL);
    pthread_mutex_init(&this->producerMutex, NULL);
    pthread_mutex_init(&this->updatePointersMutex, NULL);

    if(sem_init(&this->empty, 0, BUFFER1_SIZE)<0)
    {
        printf("Cannot initialize semaphor\n");
    }
    if(sem_init(&this->full, 0, 0)<0)
    {
        printf("Cannot initialize semaphor\n");
    }

    this->consumerPointerSynchronizator = new FastMutex();
    this->consumerWrappingSynchronizator = new FastMutex();
    this->producerPointerSynchronizator = new FastMutex();
    this->producerWrappingSynchronizator = new FastMutex();
    this->freeSlots=BUFFER1_SIZE;
    this->usedSlots=0;
    this->waitingType=incomingWaitingType;


    memset(&this->data, 0x11111111, sizeof(this->data));


}
RingBuffer::~RingBuffer()
{
    sem_destroy(&this->empty);
    sem_destroy(&this->full);
}

void RingBuffer::AddData(Message* incomingMessage)
{
    if(this->waitingType==ACTIVE_WAITING)
    {
        Message::CheckMessageForNull(incomingMessage);
        AddDataInterlocked(incomingMessage);
    }
    if (this->waitingType==SEMAPHORE)
    {
        volatile int err;
        do
        {
            err = sem_wait(&this->empty);//decrement counter of empty buffer segments 
        }while (0 != err && EINTR == errno);
        Message::CheckMessageForNull(incomingMessage);
        pthread_mutex_lock(&this->producerMutex);

        this->data[this->producerIndex]=incomingMessage;
        //printf("Data is added to buffer[%d], MessageID = %lu\n", this->producerIndex, 
        //        this->data[this->producerIndex]->GetBufferWithData()->MessageID);
        this->producerIndex++;
        if(this->producerIndex==this->lastIndex+1)//wrapping
            this->producerIndex=0;

        pthread_mutex_unlock(&this->producerMutex); 
        sem_post(&this->full); //increment counter of full buffer segment  
    }
}
void RingBuffer::AddDataInterlocked(Message* incomingMessage)
{   
    producerPointerSynchronizator->Lock();
    while(this->freeSlots<=0)
    ;
    if(this->freeSlots>0)
    {
        __sync_fetch_and_sub(&this->freeSlots,1);//        freeSlots--;

//        if(freeSlots<0)
//        {
//            freeSlots=0;
//        }
        producerPointerSynchronizator->Unlock();
        int currentPointer = GetPointer(&this->producerIndex, this->producerWrappingSynchronizator);
        this->data[currentPointer]=incomingMessage;
        __sync_fetch_and_add(&this->usedSlots,1);//        this->usedSlots++;


    }
}
Message* RingBuffer::GetData()
{
    Message* returnableMessage = NULL;
    if(this->waitingType==ACTIVE_WAITING)
    {
        returnableMessage = GetDataInterlocked();
        return returnableMessage;
    }
    if (this->waitingType==SEMAPHORE)
    {
        volatile int err;
        do
        {
            err = sem_wait(&this->full);//decrement counter of empty buffer segments
        } while (0 != err && EINTR == errno);
        ///////////////// CRITICAL SECTION /////////////////////////////
        pthread_mutex_lock(&this->consumerMutex);

        returnableMessage = this->data[this->consumerIndex];
        Message::CheckMessageForNull(returnableMessage);
        //    printf("Data is taken from buffer[%d], MessageID = %lu\n", this->consumerIndex, 
        //            this->data[this->consumerIndex]->GetBufferWithData()->MessageID);
        this->consumerIndex++;

        if(this->consumerIndex==this->lastIndex+1)//wrapping
            this->consumerIndex=0;

        pthread_mutex_unlock(&this->consumerMutex);
        ///////////////// END OF CRITICAL SECTION /////////////////////////////
        //    this->UpdatePointers();
        sem_post(&this->empty);/**/
        return returnableMessage;
    }
    return returnableMessage;
}
Message* RingBuffer::GetDataInterlocked()
{
    consumerPointerSynchronizator->Lock();
    while(this->usedSlots<=0)
    ;
    if(this->usedSlots>0)
    {
        __sync_fetch_and_sub(&this->usedSlots,1);//        usedSlots--;

        consumerPointerSynchronizator->Unlock();
        int currentPointer = GetPointer(&this->consumerIndex, this->consumerWrappingSynchronizator);

        Message* returnableMessage = this->data[currentPointer];
//        this->data[currentPointer]=NULL;//VS + VH
        Message::CheckMessageForNull(returnableMessage);
        __sync_fetch_and_add(&this->freeSlots,1);//        this->freeSlots++;

        return returnableMessage;
    }
    return NULL;
}
inline static bool compare_and_exchange(volatile int& dest, int& old_val_out, int eax, int new_val)
{
        bool result;
        __asm__ __volatile__(
                "lock cmpxchgl %4, %1 \n setzb %0"
                : "=qm"(result), "+m" (dest), "=a" (old_val_out)
                : "a" (eax), "r" (new_val));
        return result;
        /*
         g - general effective address
         m - memory effective address
         r - register
         i - immediate value, 0..0xffffffff
         n - immediate value known at compile time.

         q - регистры с байтовой адресацией (eax, ebx, ecx, edx)
         A - eax или edx
         a, b, c, d, S, D - eax, ebx, ecx, edx, esi, edi соответственно

         I - immediate 0..31
         J - immediate 0..63
         K - immediate 255
         L - immediate 65535
         M - immediate 0..3 (shifts that can be done with lea)
         N - immediate 0..255 (one-byte immediate value)
         O - immediate 0..32
        */

}
int RingBuffer::GetConsumerPointer()
{
//    int returnablePointer;
    int currentPointer = __sync_fetch_and_add(&this->consumerIndex, 1);//next thread will receive incremented value
    if(currentPointer>this->lastIndex)
    {
        consumerWrappingSynchronizator->Lock();
        if(this->consumerIndex>this->lastIndex)
        {
            this->consumerIndex=0;
            consumerWrappingSynchronizator->Unlock();
            currentPointer = __sync_fetch_and_add(&this->consumerIndex, 1);
            return currentPointer;
        }
        else
        {
            while (currentPointer>this->lastIndex)
            {
                currentPointer = __sync_fetch_and_add(&this->consumerIndex, 1);
            }
            consumerWrappingSynchronizator->Unlock();
            return currentPointer;
        }
        consumerWrappingSynchronizator->Unlock();
    }
    return currentPointer;
}
int RingBuffer::CheckPointer(int currentPointer)
{
    if(currentPointer>this->lastIndex)
    {
        std::cout << "Here is a problem with currentPointer" << std::endl;
        return 0;
//        return false;
    }
    return currentPointer;
}
int RingBuffer::GetPointer(int* index, FastMutex* wrappingSynchronizator)
{
    int currentPointer = __sync_fetch_and_add(index, 1);//next thread will receive incremented value
    if(currentPointer>this->lastIndex)
    {
        wrappingSynchronizator->Lock();//HERE IS THE FULL BUFFER
        if(*index>this->lastIndex)
        {
            *index=0;
            wrappingSynchronizator->Unlock();
            currentPointer = __sync_fetch_and_add(index, 1);
            return CheckPointer(currentPointer);
        }
        else
        {
            while (currentPointer>this->lastIndex)
            {
                currentPointer = __sync_fetch_and_add(index, 1);
            }
            wrappingSynchronizator->Unlock();
            return CheckPointer(currentPointer);
        }
        wrappingSynchronizator->Unlock();
    }

    return CheckPointer(currentPointer);
}






