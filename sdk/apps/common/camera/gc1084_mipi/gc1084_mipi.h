#ifndef _GC1084_H
#define _GC1084_H

#include "typedef.h"

#define GC1084_INPUT_W    1280
#define GC1084_INPUT_H    720
#define GC1084_OUTPUT_W    1280
#define GC1084_OUTPUT_H    720

#define DEBUG_CMOS 1

#if (DEBUG_CMOS == 1)
#define x_puts puts
#define x_printf printf
#else
#define x_puts(...)
#define x_printf(...)
#endif

#endif


