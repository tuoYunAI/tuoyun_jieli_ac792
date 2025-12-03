#ifndef _PLC_H_
#define _PLC_H_

#include "generic/typedef.h"

struct esco_plc_parm {
    int sr;
    int nch;
};

int PLC_query(struct esco_plc_parm *parm, void *dataTypeobj);

int PLC_init(void *pbuf, struct esco_plc_parm *parm, void *dataTypeobj);

void PLC_run(void *pbuf, s16 *inbuf, s16 *outbuf, u16 point, u8 repair_flag);

#endif
