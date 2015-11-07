#include "thread.h"

HANDLE start_thread(unsigned long (*func)(void *),void *param)
{
    HANDLE handle = CreateThread(NULL,0,func,param,0,NULL);
    return handle;
}

void wait_for_thread(HANDLE handle)
{
    WaitForSingleObject(handle,INFINITE);
}
