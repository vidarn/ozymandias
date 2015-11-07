#pragma once

#include <signal.h>
#define ASSERT(TEST) if(!(TEST)){raise(SIGTRAP);}
#define array_count(a) sizeof(a)/sizeof(a[0])

