#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

struct MessageBuffer {
//        unsigned long ClientSystemTime;
//        unsigned long ServerSystemTime;
//        unsigned long TransferTime;
        unsigned long MessageID;
        bool isAuxiliary;
};

class Message
{
public:
    Message();
    Message(int msg_size);
    ~Message();
    struct MessageBuffer* GetBufferWithData();
    void AssignReceivedData( struct MessageBuffer* dataToAssign);
    void SetMessageID(unsigned long messageID);
    unsigned long GetMessageID();


    static void CheckMessageForNull(Message* messageForCheck);
    void BecameAuxiliary();
    bool isAuxiliary();
    void* GetvBuffer();
    int Getvmsg_size();

private:
      struct MessageBuffer bufferWithData;
      void* vMessageBuffer;
      int vmsg_size;
};

#endif // MESSAGE_H
