#include "thread.h"
#include <stdlib.h>

typedef struct
{
    unsigned long (*func)(void *);
    void *param;
} ThreadWrapperData;

static void * thread_wrapper(void *data){
    ThreadWrapperData *thread_data = (ThreadWrapperData*)data;
    thread_data->func(thread_data->param);
    free(thread_data);
    return NULL;
}

ThreadHandle start_thread(unsigned long (*func)(void *),void *param)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,8388608);
    ThreadWrapperData *thread_data =
        (ThreadWrapperData *)malloc(sizeof(ThreadWrapperData));
    thread_data->func = func;
    thread_data->param = param;
    pthread_t handle;
    pthread_create(&handle,&attr,thread_wrapper,(void*)thread_data);
    return handle;
}

void wait_for_thread(ThreadHandle handle)
{
    pthread_join(handle,NULL);
}

