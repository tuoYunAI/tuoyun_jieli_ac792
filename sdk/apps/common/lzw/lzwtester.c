////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                  Copyright (c) 2016-2020 David Bryant                  //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* This module provides a command-line test harness for the lzw library.
 * Given a list of files, it will read each one and byte-for-byte verify
 * the data after a round-trip through a compression / decompression cycle
 * at each of the 8 available maximum symbol size settings.
 *
 * It can also optionally perform fuzz testing by randomly corrupting the
 * compressed bitstream. Obviously this will introduce integrity failures,
 * but it should not cause a crash. It also has an "exhaustive" mode that
 * creates hundreds of simulated images from each input file by successive
 * truncation from both ends.
 */

typedef struct {
    unsigned int size, index, wrapped, byte_errors, first_error, fuzz_testing;
    unsigned char *buffer;
} streamer;

static int read_buff(void *ctx)
{
    streamer *stream = ctx;

    if (stream->index == stream->size) {
        return EOF;
    }

    return stream->buffer [stream->index++];
}

static void write_buff(int value, void *ctx)
{
    streamer *stream = ctx;

    // for fuzz testing, randomly corrupt 1 byte in every 65536 (on average)

    if (stream->fuzz_testing) {
        static unsigned long long kernel = 0x3141592653589793;
        kernel = ((kernel << 4) - kernel) ^ 1;
        kernel = ((kernel << 4) - kernel) ^ 1;
        kernel = ((kernel << 4) - kernel) ^ 1;

        if (!(kernel >> 48)) {
            value ^= (int)(kernel >> 40);
        }
    }

    if (stream->index == stream->size) {
        stream->index = 0;
        stream->wrapped++;
    }

    stream->buffer [stream->index++] = value;
}


int jl_lzw_decompress(void *dst, int dst_len, void *src, int src_len)
{
    streamer reader;
    streamer writer;

    reader.buffer = src;
    reader.size = src_len;
    reader.index = 0;

    writer.size = dst_len;
    writer.buffer = dst;

    return lzw_decompress(write_buff, &writer, read_buff, &reader);
}
