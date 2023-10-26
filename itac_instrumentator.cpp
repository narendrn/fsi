#include "itac_instrumentator.h"

ITAC_Instrumentator::ITAC_Instrumentator()
{
    for(int i=0; i<(PROCESSES_QUANTITY-1); i++ )
    {
        this->clientStringsArray[i]=NULL;
    }
    sequenceNumberOfFreeSlotAtStringArray = 0;//порядковый номер свободного элемента в массиве
}


/**
Creates string with VT_clientinit
@param clientName name of the system or process initializing with for ITAC tracing
@param processRank In the non-MPI version you have to give each process
                   your own rank between 0 and nproc-1 (nproc is the total number of processes).
                   "0" used for server process.
**********************/
const char* ITAC_Instrumentator::CreateClientInitString(const char* clientName, int processRank)
{
    const char* clientInitString = NULL;
    VT_clientinit(processRank, clientName, &clientInitString);

    return clientInitString;
}
/**
Put client string to the array
**********************/
void ITAC_Instrumentator::CollectClientInitString(const char* clientString)
{
    if(sequenceNumberOfFreeSlotAtStringArray!=-1)
    {
        this->clientStringsArray[sequenceNumberOfFreeSlotAtStringArray++] = clientString;
        if(sequenceNumberOfFreeSlotAtStringArray>(PROCESSES_QUANTITY-1))
        {
            sequenceNumberOfFreeSlotAtStringArray=-1;
        }
    }else
    {
        perror("There is no free slots for ClientInitStrings.\n" \
               "Increase PROCESSES_QUANTITY const.\n");
    }
}
/**
* Register all client strings with VT_serverinit()
**********************/
/**
 * Initializes one process as the server that contacts the other
 * processes and coordinates trace file writing. The calling
 * process always gets rank #0.
 *
 * There are two possibilities:
 * -# collect all infos from the clients and pass them here
 *    (numcontacts >= 0, contacts != NULL)
 * -# start the server first, pass its contact string to the clients
 *    (numcontacts >= 0, contacts == NULL)
 *
 * This call replaces starting the VTserver executable in a separate process.
 * Parameters that used to be passed to the VTserver to control tracing
 * and trace writing can be passed to VT_initialize() instead.
 *
 * @param servername     similar to clientname in VT_clientinit():
 *                       the name of the server. Currently only
 *                       used for error messages. Copied by ITC.
 * @param numcontacts    number of client processes
 * @param contacts       contact string for each client process (order is irrelevant);
 *                       copied by ITC
 * @retval contact       Will be set to a string which tells spawned children how to
 *                       contact this server. Guaranteed not to contain spaces.
 *                       The server may copy this string, but doesn't have to, because
 *                       ITC will not free this string until VT_finalize() is called.
 *                       *contact must have been set to NULL before calling this function.
 * @return error code

 extern int VT_serverinit( const char *servername, int numcontacts,
                          const char *contacts[], const char **contact );

 */
void ITAC_Instrumentator::RegisterClientsString()
{
    VT_serverinit("server", PROCESSES_QUANTITY, this->clientStringsArray, &serverClientString);
}

