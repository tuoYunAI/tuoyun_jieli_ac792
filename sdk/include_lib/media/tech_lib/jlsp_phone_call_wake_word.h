#ifndef _PHONE_CALL_WAKE_WORD_H_
#define _PHONE_CALL_WAKE_WORD_H_

//int JLSP_command_wake_word_get_heap_size(void* file_ptr, int* model_size);
int JLSP_phone_call_wake_word_get_heap_size(void *file_ptr, int *model_size, int *private_heap_size, int *share_heap_size);
//void* JLSP_command_wake_word_init(char *heap_buffer, void* fp, int heap_size, int model_size, float confidence, int online);
void *JLSP_phone_call_wake_word_init(void *fp, char *private_heap_buffer, int private_heap_size, char *share_heap_buffer, int share_heap_size, int model_size, float confidence, int online);
int JLSP_phone_call_wake_word_reset(void *m);
int JLSP_phone_call_wake_word_process(void *m, char *pcm, int size);
int JLSP_phone_call_wake_word_free(void *m);

#endif
