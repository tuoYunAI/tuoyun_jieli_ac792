#ifndef _JLDEMUXER_H_
#define _JLDEMUXER_H_

#include "jlstream.h"


int jldemuxer_get_tone_file_fmt(struct stream_file_info *info, struct stream_fmt *fmt);

int jldemuxer_file_coding_type_filter(u8 *data, int len, int types[]);

#endif
