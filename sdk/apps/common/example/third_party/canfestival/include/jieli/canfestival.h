#ifndef CANFESTIVAL_H_
#define CANFESTIVAL_H_

#include "data.h"
#include "applicfg.h"
#include "config.h"
#include "timerscfg.h"

unsigned char canSend(CAN_PORT notused, Message *m);

#endif
