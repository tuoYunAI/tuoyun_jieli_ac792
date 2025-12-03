#include "media_memory.h"
#include "malloc.h"
#include "app_config.h"

#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma code_seg(".audio.text.cache.L1")
#endif

/*将要使用psram的模块对应得枚举添加到以下列表，不在列表中的模块默认使用sram*/
static const enum audio_module media_psram_module[] =  {
    AUD_MODULE_ECHO,
    AUD_MODULE_REVERB,
    /* AUD_MODULE_VBASS,
    AUD_MODULE_DYN_EQ,
    AUD_MODULE_CHORUS,
    AUD_MODULE_VOICE_CHANGER,
    AUD_MODULE_FREQ_SHIFT,
    AUD_MODULE_NOTCH_HOWLING, */

    //codec module list
    AUD_MODULE_AAC,
    AUD_MODULE_AAC_ENERGY,
};

void *media_malloc(enum audio_module module, size_t size)
{
    return zalloc(size);
}

void media_free(void *pv)
{
    free(pv);
}
