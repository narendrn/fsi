#ifndef ITAC_INSTRUMENTATOR_H
#define ITAC_INSTRUMENTATOR_H

#include "VT.h"
#include <stdio.h>
#include <stdlib.h>
#include "consts.h"

struct ClientInitString {
    const char* clientInitString;
    const char* clientName;
    int processRank;
};

class ITAC_Instrumentator
{
public:
    ITAC_Instrumentator();
    static const char* CreateClientInitString(const char* clientName, int processRank);
    void CollectClientInitString(const char* clientString);
    void RegisterClientsString();

private:
//    struct ClientInitString* clientStringsArray[PROCESSES_QUANTITY];
    const char* clientStringsArray[PROCESSES_QUANTITY];
    int sequenceNumberOfFreeSlotAtStringArray;
    const char* serverClientString;
};

#endif // ITAC_INSTRUMENTATOR_H
