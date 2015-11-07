#pragma once

#ifdef _WIN32
#include "thread_win32.h"
#else
#include "thread_pthread.h"
#endif

ThreadHandle start_thread(unsigned long (*func)(void *),void *param);
void wait_for_thread(ThreadHandle handle);
