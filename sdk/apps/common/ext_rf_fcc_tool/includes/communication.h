#ifndef _FCC_COMM_H_
#define _FCC_COMM_H_

#include "typedef.h"

typedef struct {
    void (*init)(void);
    s32(*read)(u8 *, u32);
    s32(*write)(u8 *, u32);
} fcc_comm_type;


fcc_comm_type *fcc_comm_ops(void);

#endif


