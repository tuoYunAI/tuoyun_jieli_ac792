#ifndef _RF_FCC_DATATYPE_H_
#define _RF_FCC_DATATYPE_H_


enum FCC_STATUS_GROUP {
    ST_SUCC = 0,
    ST_FAIL,
};


enum FCC_OPCODE_GROUP {
    OP_START,

    OP_CONTINUOUS_PACKET_TX = 0x01,
    OP_COUNT_PACKET_TX,
    OP_CARRIER_TX,
    OP_CONTINUOUS_TX,
    OP_GET_MAC,
    OP_RX_TEST,
    OP_SINGLE_TONE_TX,

    OP_INQURY_READY_ST = 0x20,
    OP_WRITE_TEST_RES,
    OP_EXIT_CUR_TEST,

    OP_END,

    OP_RESPOND = 0xFF,
};


#pragma pack (1)
struct fcc_data {
    u8 mark[2];
    u16 opcode;
    u16 params_len;
    u8 *params;
    u16 crc;
};
#pragma pack ()


#pragma pack (1)
struct host_data {
    struct list_head entry;
    struct fcc_data fcc_data;
    struct host_data *self;
};
#pragma pack ()


#define STRUCT_ID_FCC_CPTX 1
#pragma pack (1)
typedef struct __FCC_CPTX {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	mp_rate;
    u32	tx_durtime;
} FCC_CPTX;
#pragma pack ()


#define STRUCT_ID_FCC_CPTX_COUNT 2
#pragma pack (1)
typedef struct __FCC_CPTX_COUNT {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	mp_rate;
    u32	npackets;
} FCC_CPTX_COUNT;
#pragma pack ()


#define STRUCT_ID_FCC_CS 3
#pragma pack (1)
typedef struct __FCC_CS {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	mp_rate;
    u32	tx_durtime;
} FCC_CS;
#pragma pack ()


#define STRUCT_ID_FCC_CTX 4
#pragma pack (1)
typedef struct __FCC_CTX {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	mp_rate;
    u32	tx_durtime;
} FCC_CTX;
#pragma pack ()


#define STRUCT_ID_FCC_MAC_GET 5
#pragma pack (1)
typedef struct __FCC_MAC_GET {
    u8	mac_get;
} FCC_MAC_GET;
#pragma pack ()


#define STRUCT_ID_FCC_RX 6
#pragma pack (1)
typedef struct __FCC_RX {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u32	rx_durtime;
} FCC_RX;
#pragma pack ()


#define STRUCT_ID_FCC_STT 7
#pragma pack (1)
typedef struct __FCC_STT {
    u8	mp_channel;
    u8	bandwidth;
    u8	short_gi;
    u8	antenna_x;
    u8	pathx_txpower;
    u32	mp_rate;
    u32	tx_durtime;
} FCC_STT;
#pragma pack ()


#endif


