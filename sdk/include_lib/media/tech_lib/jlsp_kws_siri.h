#ifndef _KWS_SIRI_H_
#define _KWS_SIRI_H_

int JLSP_wake_word_get_heap_size(void *file_ptr, int *model_size);
void *JLSP_wake_word_init(char *heap_buffer, void *fp, int heap_size, int model_size, float confidence, int online);
int JLSP_wake_word_reset(void *m);
int JLSP_wake_word_process(void *m, char *pcm, int size);
int JLSP_wake_word_free(void *m);

#endif
