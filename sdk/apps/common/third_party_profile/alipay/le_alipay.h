#ifndef _LE_ALIPAY_H_
#define _LE_ALIPAY_H_

#include <stdint.h>

void alipay_init(void);
void alipay_exit(void);
int upay_ble_send_data(u8 *data, u16 len);

#endif
