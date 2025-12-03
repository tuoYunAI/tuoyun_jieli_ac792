////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                  Copyright (c) 2016-2020 David Bryant                  //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#ifndef LZWLIB_H_
#define LZWLIB_H_

int jl_lzw_decompress(void *dst, int dst_len, void *src, int src_len);

#endif /* LZWLIB_H_ */

