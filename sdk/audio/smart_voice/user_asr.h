#ifndef _USER_ASR_H_
#define _USER_ASR_H_

int user_platform_asr_open(void);

void user_platform_asr_close(void);

int user_asr_core_handler(void *priv, int taskq_type, int *msg);

#endif

