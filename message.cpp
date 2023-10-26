#include "message.h"
#include <memory.h>

Message::Message()
{
    vMessageBuffer = NULL;
}

Message::Message(int msg_size)
{
    if(msg_size > sizeof(struct MessageBuffer))
      vMessageBuffer = malloc(msg_size);
    else
      vMessageBuffer = malloc(sizeof(struct MessageBuffer));

    vmsg_size = msg_size;
//    printf("vMessageBuffer of %d bytes allocated\n", msg_size);
}


Message::~Message()
{
  if(vMessageBuffer)
    free(vMessageBuffer);
}

void Message::BecameAuxiliary()
{
    this->bufferWithData.isAuxiliary=true;
}
bool Message::isAuxiliary()
{
    if(this->bufferWithData.isAuxiliary)
    {
        return true;
    }
    else
    {
        return false;
    }
}
struct MessageBuffer* Message::GetBufferWithData()
{
    return &this->bufferWithData;
}

void Message::AssignReceivedData(MessageBuffer *dataToAssign)
{
    this->bufferWithData = *dataToAssign;
}

void Message::SetMessageID(unsigned long messageID)
{
    this->bufferWithData.MessageID = messageID;
    if(vMessageBuffer)
      memcpy(vMessageBuffer, (void*)(&bufferWithData), sizeof(struct MessageBuffer));
    //printf("messageID filled");  
}

unsigned long Message::GetMessageID()
{
    return this->bufferWithData.MessageID;
}

void Message::CheckMessageForNull(Message* messageForCheck)
{
    if (messageForCheck == NULL)
    {
        perror("Message is NULL\n");
    }
}

void* Message::GetvBuffer(){
    return(this->vMessageBuffer);
}

int Message::Getvmsg_size(){
    return this->vmsg_size;
}
