#ifndef _JL_AUDIO_KWS_H_
#define _JL_AUDIO_KWS_H_

#include "generic/typedef.h"

#define JL_KWS_WAKE_WORD            0
#define JL_KWS_COMMAND_KEYWORD      1
#define JL_KWS_CALL_KEYWORD         2

struct kws_multi_keyword_model {
    int (*mem_dump)(int model, int *model_size, int *private_size, int *share_size);
    void *(*init)(int model, char *private_buffer, int private_size, char *share_buffer, int share_size, int model_size, float *confidence, int online);
    int (*reset)(void *m);
    int (*process)(void *m, int model, char *pcm, int size);
    int (*free)(void *m);
};

void *audio_kws_open(u8 mode, const char *file_name);

int audio_kws_detect_handler(void *kws, void *data, int len);

void audio_kws_close(void *kws);

#endif

