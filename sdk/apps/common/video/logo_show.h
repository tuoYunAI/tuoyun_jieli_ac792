#ifndef LOGO_SHOW_H_INCLUDED
#define LOGO_SHOW_H_INCLUDED

#include "fs/fs.h"
#include "server/video_server.h"
#include "server/video_dec_server.h"
#include "lcd_config.h"
#include "audio_server.h"
struct logo_dec {
    u8 file_type;
    u8 fname[MAX_FILE_NAME_LEN];
    struct server *video_dec;
    struct server *audio_dec;
    union video_dec_req video_req;
    union audio_req audio_req;
    u8 *audio_buf;
    u8 *video_buf;
    FILE *audio_file;
    u8 logo_state;
};

#define LOGO_FILE_TYPE_MOVE      0
#define LOGO_FILE_TYPE_JPG       1

#define LOGO_AUDIO_DEC_BUF_SIZE  64 * 1024
#define LOGO_VIDEO_DEC_BUF_SIZE  600 * 1024

#define LOGO_SHOW_END    12
#define LOGO_SHOW_START  11

int logo_get_state();
void logo_stop(void (*func)());
int logo_show(char *logo_path, char *audio_path, int time_out, void (*func)());
#endif // LOGO_SHOW_H_INCLUDED
