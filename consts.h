#ifndef CONSTS_H
#define CONSTS_H

#include <netinet/in.h>

//sender parameters
const int NUMBER_OF_MESSAGES_FOR_SEND = 500000;
const int MESSAGE_RATE = 100000;
const int MESSAGE_BLOCK = 1000;
const int MESSAGE_SIZE = 16;

const int TCP = 0;
const int UDP = 1;

const int SENDER_MODE = 0;
const int RECEIVER_MODE_1_PROCESS = 1;
const int RECEIVER_MODE_2_PROCESS = 2;

const int SEND = 0;
const int RECEIVE = 1;

//ports and IPs
const int PORT_NUMBER = 5011;
const int VT_STRING_PORT_NUMBER = 5012;
const char* const RECEIVER_IP_ADDRESS = "172.16.21.1";
//const char* const RECEIVER_IP_ADDRESS = "172.11.1.1";
//const char* const RECEIVER_IP_ADDRESS = "173.16.41.98";
const char* const OUTPUT_IP = "172.16.21.1";
//const char* const OUTPUT_IP = "192.111.111.1";

// sync mode - either this or that is non zero
const int SEMAPHORE= 1;
const int ACTIVE_WAITING = 0;

/**
RingBuffer size
*/
const int BUFFER1_SIZE = 500;
/**

*/
/**
*/
const int PROCESSES_QUANTITY = 2;

#endif // CONSTS_H
