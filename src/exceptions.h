#pragma once
#include <fenv.h>
#define ACTIVE_FPE FE_DIVBYZERO| FE_INVALID| FE_OVERFLOW //| FE_UNDERFLOW
#define ENABLE_FPE  feenableexcept (ACTIVE_FPE)
#define DISABLE_FPE fedisableexcept(ACTIVE_FPE)
