#include "duer_record.h"
#include "system/includes.h"
#include "device/device.h"
#include "generic/circular_buf.h"
#include "jlstream.h"


#if INTELLIGENT_DUER
#define MY_AUDIO_SAVE_TEST    0

#define AUDIO_RECORD_VOICE_VOLUME 100
#if MY_AUDIO_SAVE_TEST
static FILE *rec_file = NULL;
#define __file rec_file
#endif

duer_rec_t rec = {0,};

static int mic_is_busy()
{
    return rec.is_audio_record_open;
}

static void my_duer_rec_cbuf_init()
{
    if (!rec.buf) {
        rec.buf = malloc(DUER_CBUF_SIZE);
    }
    cbuf_init(&rec.cbuf, rec.buf, DUER_CBUF_SIZE);
}

static void my_duer_rec_cbuf_exit(void)
{
    cbuf_clear(&(rec.cbuf));
    free(rec.buf);
    rec.buf = NULL;
}

static int my_fwrite(FILE *file, void *buf, u32 size)
{
    int ret = fwrite(buf, size, 1, file);
    return ret;
}

static int my_duer_rec_write_data(void *file, void *voice_buf, unsigned int voice_len)
{
#if MY_AUDIO_SAVE_TEST
    if (__file) {
        int wlen = my_fwrite(__file, voice_buf, voice_len);
        if (wlen != voice_len) {
            printf("save file err: %d, %d\n", wlen, voice_len);
        }
    }
#endif
    putchar('*');
    int wlen = cbuf_write(&rec.cbuf, voice_buf, voice_len);
    if (wlen != voice_len) {
        MY_LOG_E("audio data out err: %d, %d\n", wlen, voice_len);
        cbuf_clear(&rec.cbuf);
    }
    return wlen;
}

static int mic_rec_pram_init_(int format, int CHANNEL, int SAMPLE_RATE)
{
    int op_ret = 0;
    MY_LOG_D("into volc audio soft init");
    if (!rec.enc_server) {
        rec.enc_server = video_mic_recorder_open(SAMPLE_RATE, format, NULL, my_duer_rec_write_data);
        if (rec.enc_server) {
            MY_LOG_D("enc server_open succ!");
            rec.is_audio_record_open = true;
        } else {
            MY_LOG_D("enc server_open error!");
            op_ret = 1;
        }
    }
    return op_ret;
}

static void mic_rec_close()
{
    int err;
    err = video_mic_recorder_close(rec.enc_server);
    rec.enc_server = NULL;
    rec.is_audio_record_open = 0;
    MY_LOG_D("_audio_recoder_stop err=%d", err);
}


int duer_rec_stop(void)
{
    if (!mic_is_busy()) {
        printf("duer_mic_is_null \n\n");
        return true;
    }
    mic_rec_close();
#if MY_AUDIO_SAVE_TEST
    if (__file) {
        fclose(__file);
        __file = NULL;
    }
#endif
    my_duer_rec_cbuf_exit();
    return true;
}


int duer_rec_start(void)
{
    my_duer_rec_cbuf_init();
    if (mic_is_busy()) {
        printf("duer_mic_is_busy \n\n");
        return false;
    }
#if MY_AUDIO_SAVE_TEST
    if (__file) {
        fclose(__file);
        __file = NULL;
    }
    __file = fopen("storage/sd0/C/ze.bin", "w+");
    if (!__file) {
        MY_LOG_E("fopen err \n\n");
    }
#endif

    mic_rec_pram_init_(DUER_REC_TYPE, 1, 16000);
    return true;
}
#endif
