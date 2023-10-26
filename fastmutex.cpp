#include "fastmutex.h"

FastMutex::FastMutex()
{
    this->locked=0;
}

void FastMutex::Lock()
{
    while( __sync_lock_test_and_set( &locked, 1) == 1 )
    ;
}
void FastMutex::Unlock()
{
    __sync_lock_release( &locked );
}
