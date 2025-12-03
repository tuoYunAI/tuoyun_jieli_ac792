/*===================================================================*/
/*                                                                   */
/*  InfoNES_System_Win.cpp : The function which depends on a system  */
/*                           (for Windows)                           */
/*                                                                   */
/*  2000/05/12  InfoNES Project                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

#include "InfoNES.h"
#include "InfoNES_System.h"
/* #include "InfoNES_Resource.h" */
/* #include "InfoNES_Sound_Win.h" */
#include "InfoNES_pAPU.h"
#include "audio_music/pcm_play_api.h"
#include "event/event.h"
#include "event/key_event.h"
#include "app_config.h"

/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  Variables for Windows                                            */
/*-------------------------------------------------------------------*/
#define APP_NAME     "InfoNES v0.93J"

#define DUAL_THREAD_ACCELERATION  //利用双核双线程加速,目前分割工作量实测可加速33%, 但是也会占据另外一个核的33%性能

#ifdef DUAL_THREAD_ACCELERATION
#include "os/os_api.h"
static OS_SEM mcu_lcd_physics_flush_sem, mcu_lcd_physics_flush_end_sem;
#endif

static void _mcu_lcd_physics_flush(void)
{
    static WORD fb2[ NES_DISP_WIDTH * NES_DISP_HEIGHT ];//由于脏矩形，下面转换会改到干净矩形的区域,所以需要多申请一块
    unsigned char *data = (unsigned char *)fb2;
    for (int i = 0; i < NES_DISP_WIDTH * NES_DISP_HEIGHT; i++) {
        unsigned short color = WorkFrame[i] ;
        data[2 * i] = (color >> 8) & 0xff;
        data[2 * i + 1] = color & 0xff;
    }
    lcd_lvgl_full(0, NES_DISP_WIDTH - 1, 0, NES_DISP_HEIGHT - 1, data);

#if 1
    static int fps_time_hdl;
    static unsigned int fps;
    ++fps;
    if (time_lapse(&fps_time_hdl, 1000)) {
        printf("NES FPS=%d\r\n", fps);
        fps = 0;
    }
#endif
}

static void mcu_lcd_physics_flush(void)
{
#ifdef DUAL_THREAD_ACCELERATION
    os_sem_pend(&mcu_lcd_physics_flush_end_sem, 0);
    os_sem_post(&mcu_lcd_physics_flush_sem);
#else
    _mcu_lcd_physics_flush();
#endif
}

#ifdef DUAL_THREAD_ACCELERATION
static void nes_mcu_lcd_physics_flush_task(void *p)
{
    while (1) {
        os_sem_pend(&mcu_lcd_physics_flush_sem, 0);
        _mcu_lcd_physics_flush();
        os_sem_post(&mcu_lcd_physics_flush_end_sem);
    }
}
#endif

// Palette data
const WORD NesPalette[ 64 ] = {
#if 0//调色板有问题？
    0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
    0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
    0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
    0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
    0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
    0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
    0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
    0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
#else

    0x738E, 0x20D1, 0x0015, 0x4013, 0x880E, 0xA802, 0xA000, 0x7840,
    0x4140, 0x0200, 0x0280, 0x01C2, 0x19CB, 0x0000, 0x0000, 0x0000,
    0xBDD7, 0x039D, 0x21DD, 0x801E, 0xB817, 0xE00B, 0xD940, 0xCA41,
    0x8B80, 0x0480, 0x0540, 0x0487, 0x0411, 0x0000, 0x0000, 0x0000,
    0xFFDF, 0x3DDF, 0x5C9F, 0x445F, 0xF3DF, 0xFB96, 0xFB8C, 0xFCC7,
    0xF5C7, 0x8682, 0x4EC9, 0x5FD3, 0x075B, 0x0000, 0x0000, 0x0000,
    0xFFDF, 0xAF1F, 0xC69F, 0xD65F, 0xFE1F, 0xFE1B, 0xFDD6, 0xFED5,
    0xFF14, 0xE7D4, 0xAF97, 0xB7D9, 0x9FDE, 0x0000, 0x0000, 0x0000,


    /*0x738E,0x88C4,0xA800,0x9808,0x7011,0x1015,0x0014,0x004F,*/
    /*0x0148,0x0200,0x0280,0x11C0,0x59C3,0x0000,0x0000,0x0000,*/
    /*0xBDD7,0xEB80,0xE9C4,0xF010,0xB817,0x581C,0x015B,0x0A59,*/
    /*0x0391,0x0480,0x0540,0x3C80,0x8C00,0x0000,0x0000,0x0000,*/
    /*0xFFDF,0xFDC7,0xFC8B,0xFC48,0xFBDE,0xB39F,0x639F,0x3CDF,*/
    /*0x3DDE,0x1690,0x4EC9,0x9FCB,0xDF40,0x0000,0x0000,0x0000,*/
    /*0xFFDF,0xFF15,0xFE98,0xFE5A,0xFE1F,0xDE1F,0xB5DF,0xAEDF,*/
    /*0xA71F,0xA7DC,0xBF95,0xCFD6,0xF7D3,0x0000,0x0000,0x0000,*/
#endif
};


/* Menu screen */
int InfoNES_Menu()
{
#ifdef DUAL_THREAD_ACCELERATION
    lcd_interface_set_non_block(0);// mcu lcd硬件接口要求阻塞释放cpu,否则提前释放信号有概率花屏
    os_sem_create(&mcu_lcd_physics_flush_sem, 0);
    os_sem_create(&mcu_lcd_physics_flush_end_sem, 1);
    thread_fork("nes_mcu_lcd_flush_task", 26, 200, 0, NULL, nes_mcu_lcd_physics_flush_task, NULL);
#endif
    return 0;
}


extern const BYTE nes_rom[];
/* Read ROM image file */
int InfoNES_ReadRom(const char *pszFileName)
{
    /*
     *  Read ROM image file
     *
     *  Parameters
     *    const char *pszFileName          (Read)
     *
     *  Return values
     *     0 : Normally
     *    -1 : Error
     */


    /* Read ROM Header */
    BYTE *rom = (BYTE *)nes_rom;
    memcpy(&NesHeader, rom, sizeof(NesHeader));
    if (memcmp(NesHeader.byID, "NES\x1a", 4) != 0) {
        /* not .nes file */
        return -1;
    }
    rom += sizeof(NesHeader);

    /* Clear SRAM */
    memset(SRAM, 0, SRAM_SIZE);

    /* If trainer presents Read Triner at 0x7000-0x71ff */
    if (NesHeader.byInfo1 & 4) {
        //memcpy( &SRAM[ 0x1000 ], rom, 512);
        rom += 512;
    }

    /* Allocate Memory for ROM Image */
    ROM = rom;
    rom += NesHeader.byRomSize * 0x4000;

    if (NesHeader.byVRomSize > 0) {
        /* Allocate Memory for VROM Image */
        VROM = (BYTE *)rom;
        rom += NesHeader.byVRomSize * 0x2000;
    }

    /* Successful */
    return 0;
}


/* Release a memory for ROM */
void InfoNES_ReleaseRom()
{
}


/* Transfer the contents of work frame on the screen */
void InfoNES_LoadFrame()
{
    // Set screen data
    mcu_lcd_physics_flush();
}

static unsigned char key_data = 0xff;
static unsigned char last_key_data;
static unsigned char hold_flag;

static int __prob_handler(struct sys_event *event)
{
    struct key_event *key = (struct key_event *)event->payload;

    if (key->action == KEY_EVENT_HOLD) {
        hold_flag = 1;
    } else {
        hold_flag = 0;
    }

    key_data = key->value;
    last_key_data = key_data;

    return -EINVAL;
}

#ifdef CONFIG_NES_DEMO
SYS_EVENT_STATIC_HANDLER_REGISTER(nes_key_event, 0) = {
    .event_type     = SYS_KEY_EVENT,
    .prob_handler   = __prob_handler,
};
#endif

#define PAD_JOY_A      0x01
#define PAD_JOY_B      0x02
#define PAD_JOY_SELECT 0x04
#define PAD_JOY_START  0x08
#define PAD_JOY_UP     0x10
#define PAD_JOY_DOWN   0x20
#define PAD_JOY_LEFT   0x40
#define PAD_JOY_RIGHT  0x80

/* Get a joypad state */
void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem)
{
    *pdwPad1 = 0;
    *pdwPad2 = 0;

    if (hold_flag) {
        key_data = last_key_data;
    }

    if (key_data != 0xff) {
        if (key_data == KEY_UP || key_data == KEY_VOLUME_INC) {
            *pdwPad1 |= PAD_JOY_UP;
        }
        if (key_data == KEY_DOWN || key_data == KEY_VOLUME_DEC) {
            *pdwPad1 |= PAD_JOY_DOWN;
        }
        if (key_data == KEY_LEFT || key_data == KEY_ENC) {
            *pdwPad1 |= PAD_JOY_LEFT;
        }
        if (key_data == KEY_RIGHT || key_data == KEY_MODE) {
            *pdwPad1 |= PAD_JOY_RIGHT;
        }
        if (key_data == KEY_PHOTO) {
            *pdwPad1 |= PAD_JOY_SELECT;
        }
        if (key_data == KEY_OK) {
            *pdwPad1 |= PAD_JOY_START;
        }
        if (key_data == KEY_POWER) {
            *pdwPad1 |= PAD_JOY_A;
        }
        if (key_data == KEY_CANCLE) {
            *pdwPad1 |= PAD_JOY_B;
        }

        key_data = 0xff;
    }
}


/* memcpy */
void *InfoNES_MemoryCopy(void *dest, const void *src, int count)
{
    return memcpy(dest, src, count);
}


/* memset */
void *InfoNES_MemorySet(void *dest, int c, int count)
{
    return memset(dest, c, count);
}


/* Print debug message */
void InfoNES_DebugPrint(char *pszMsg)
{
}


/* Wait */
void InfoNES_Wait()
{
}

static void *NES_Sound_Handle;
static short *pcm_data;

/* Sound Initialize */
void InfoNES_SoundInit(void)
{
}


/* Sound Open */
int InfoNES_SoundOpen(int samples_per_sync, int sample_rate)
{
#if defined CONFIG_AUDIO_ENABLE
    pcm_data = (short *)malloc(samples_per_sync * 2);
    if (NULL == pcm_data) {
        return -1;
    }

    NES_Sound_Handle = audio_pcm_play_open(sample_rate, samples_per_sync * 2, sample_rate, 1, 100, 1);
    if (NULL == NES_Sound_Handle) {
        free(pcm_data);
        pcm_data = NULL;
        return -1;
    }

    return audio_pcm_play_start(NES_Sound_Handle);
#else
    return 0;
#endif
}


/* Sound Close */
void InfoNES_SoundClose(void)
{
#if defined CONFIG_AUDIO_ENABLE
    audio_pcm_play_stop(NES_Sound_Handle);

    NES_Sound_Handle = NULL;

    free(pcm_data);

    pcm_data = NULL;
#endif
}


/* Sound Output 4 Waves - 2 Pulse, 1 Triangle, 1 Noise */
void InfoNES_SoundOutput4(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4)
{
#if defined CONFIG_AUDIO_ENABLE
    BYTE wave;
    char s_wave;

    for (int i = 0; i < samples; ++i) {
        wave = ((unsigned short)wave1[i] + wave2[i] + wave3[i] + wave4[i]) / 4;
        wave += 0x80;
        s_wave = (char)wave;
        pcm_data[i] = (short)(s_wave) << 8;
        /* pcm_data[i] = (short)(s_wave) << 7; */
    }
    /* put_buf(wave1, samples); */

    audio_pcm_play_data_write(NES_Sound_Handle, pcm_data, samples * 2);
#endif
}

/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise, 1 DPCM */
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5)
{
#if defined CONFIG_AUDIO_ENABLE
    BYTE wave;
    char s_wave;

    for (int i = 0; i < samples; ++i) {
        wave = ((unsigned short)wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i]) / 5;
        wave += 0x80;
        s_wave = (char)wave;
        pcm_data[i] = (short)(s_wave) << 8;
        /* pcm_data[i] = (short)(s_wave) << 7; */
    }

    audio_pcm_play_data_write(NES_Sound_Handle, pcm_data, samples * 2);
#endif
}


/* Print system message */
void InfoNES_MessageBox(char *pszMsg, ...)
{
}

