#include "system/spinlock.h"
#include "system/timer.h"
#include "btstack/avctp_user.h"
#include "generic/circular_buf.h"
#include "le_hogp/standard_hid.h"
#include "bt_common.h"
#include "app_config.h"


#if TCFG_BT_SUPPORT_PROFILE_HID

#define LOG_TAG     		"[EDR_HID]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"


#define TEST_USER_HID_EN    0

#define HID_DATA            0xA0     /*Device&host*/
#define HID_DATC            0xB0     /*Device&host  DEPRECATED*/

/*DATA*/
#define DATA_OTHER          0x00
#define DATA_INPUT          0x01
#define DATA_OUTPUT         0x02
#define DATA_FEATURE        0x03

#define HID_SEND_MAX_SIZE   16  //描述符数据包的长度
#define HID_TMP_BUFSIZE     (64*2)
#define cbuf_get_space(a) (a)->total_len


static void (*user_hid_send_wakeup)(void) = NULL;
static u16 hid_channel;//inter_channel
static u16 hid_ctrl_channel;//ctrl_channel
static volatile u8 hid_s_step;
static u16 hid_timer_id;
static cbuffer_t user_send_cbuf;
static u8 hid_tmp_buffer[HID_TMP_BUFSIZE];
static volatile u8 bt_send_busy;
static spinlock_t lock;

void hid_diy_regiest_callback(void *cb, void *interrupt_cb);
int user_hid_send_data(u8 *buf, u32 len);


typedef struct {
    u8 report_type;
    u8 report_id;
    u8 data[HID_SEND_MAX_SIZE - 2];
} hid_data_info_t;

/* enum { */
/* REMOTE_DEV_UNKNOWN  = 0, */
/* REMOTE_DEV_ANDROID		, */
/* REMOTE_DEV_IOS			, */
/* }; */
//参考识别手机系统
void sdp_callback_remote_type(u8 remote_type)
{
    log_info("edr_hid:remote_type= %d", remote_type);
}

//-----------------------------------------------------
static u16 user_data_read_sub(u8 *buf, u16 buf_size)
{
    u16 ret_len;

    if (0 == cbuf_get_data_size(&user_send_cbuf)) {
        return 0;
    }

    spin_lock(&lock);
    cbuf_read(&user_send_cbuf, &ret_len, 2);
    if (ret_len && ret_len <= buf_size) {
        cbuf_read(&user_send_cbuf, buf, ret_len);
    }
    spin_unlock(&lock);

    return ret_len;
}

static void user_data_try_send(void)
{
    if (bt_send_busy) {
        return;
    }

    bt_send_busy = 1;//hold

    u8 tmp_send_buf[HID_SEND_MAX_SIZE];

    u16 send_len = user_data_read_sub(tmp_send_buf, HID_SEND_MAX_SIZE);

    if (send_len) {
        user_hid_send_data(tmp_send_buf, send_len);
    }

    bt_send_busy = 0;
}

static u32 user_data_write_sub(u8 *data, u16 len)
{
    u16 wlen;
    u16 buf_space = cbuf_get_space(&user_send_cbuf) - cbuf_get_data_size(&user_send_cbuf);
    if (len + 2 > buf_space) {
        return 0;
    }

    spin_lock(&lock);
    wlen = cbuf_write(&user_send_cbuf, &len, 2);
    wlen += cbuf_write(&user_send_cbuf, data, len);
    spin_unlock(&lock);

    user_data_try_send();

    return wlen;
}

#if TEST_USER_HID_EN
typedef struct {
    u8 report_type;
    u8 report_id;
    u8 button;
    s8 x;
    s8 y;
} hid_ctl_info_t;

static const hid_ctl_info_t test_key[5] = {
    {0xA1, 1, 0, 0, 0},
    {0xA1, 1, 0, 100, 0},
    {0xA1, 1, 0, -100, 0},
};

static void test_hid_send_step(void)
{
    static u8 xy_step = 0;
    u8 send_len = 5;

    if (!hid_s_step) {
        return;
    }

    xy_step = !xy_step;

    if (xy_step) {
        hid_s_step = 1;
    } else {
        hid_s_step = 2;
    }

    if (0 == user_hid_send_data((u8 *)&test_key[hid_s_step], send_len)) {
        hid_s_step = 0;
    }
}
#endif

static void user_hid_timer_handler(void *para)
{
    static u8 count = 0;

    if (!hid_channel) {
        return;
    }

    if (++count > 5) {
        count = 0;
        hid_s_step = 1;
        test_hid_send_step();
    }
}

static void user_hid_send_ok_callback(void)
{
    bt_send_busy = 0;

    if (user_hid_send_wakeup) {
        user_hid_send_wakeup();
    }

    user_data_try_send();

    /* test_hid_send_step(); */
}

void user_hid_regiser_wakeup_send(void *cbk)
{
    user_hid_send_wakeup = cbk;
}

void user_hid_disconnect(void)
{
    if (hid_channel) {
        bt_cmd_prepare(USER_CTRL_HID_DISCONNECT, 0, NULL);
    }
}

int user_hid_send_data(u8 *buf, u32 len)
{
    if (!hid_channel) {
        return -1;
    }

    hid_s_param_t s_par;
    s_par.chl_id = hid_channel;
    s_par.data_len = len;
    s_par.data_ptr = buf;

    int ret = bt_cmd_prepare(USER_CTRL_HID_SEND_DATA, sizeof(hid_s_param_t), (u8 *)&s_par);
    if (ret) {
        log_error("hid send fail!!! %d", ret);
    }

    return ret;
}

static void user_hid_msg_handler(u32 msg, u8 *packet, u32 packet_size)
{
    switch (msg) {
    case 1:
        hid_ctrl_channel = little_endian_read_16(packet, 0); //ctrl_channel
        hid_channel = little_endian_read_16(packet, 2); //inter_channel
        bt_send_busy = 0;
        log_info("hid connect 0x%d, 0x%x", hid_ctrl_channel, hid_channel);
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        break;

    case 2:
        log_info("hid disconnect");
        hid_channel = 0;
        hid_ctrl_channel = 0;
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        break;

    case 3:
        if (hid_channel == little_endian_read_16(packet, 0)) {
            user_hid_send_ok_callback();
        }
        break;

    default:
        break;
    }
}

void user_hid_init(void (*user_hid_output_handler)(u8 *packet, u16 size, u16 channel))
{
    log_info("%s", __FUNCTION__);

    hid_channel = 0;
    hid_ctrl_channel = 0;
    hid_diy_regiest_callback(user_hid_msg_handler, user_hid_output_handler);

#if TEST_USER_HID_EN
    if (!hid_timer_id) {
        hid_timer_id = sys_timer_add_to_task("sys_timer", NULL, user_hid_timer_handler, 5000);
    }
#endif

    cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
}

void user_hid_exit(void)
{
    log_info("%s", __FUNCTION__);

    user_hid_disconnect();

    if (hid_timer_id) {
        sys_timer_del(hid_timer_id);
        hid_timer_id = 0;
    }
}

int edr_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (!hid_channel) {
        return -1;
    }

    if (len > HID_SEND_MAX_SIZE - 2) {
        log_info("buffer limitp!!!\n");
        return -2;
    }

    hid_data_info_t data_info;
    data_info.report_type = HID_DATA | DATA_INPUT;
    data_info.report_id = report_id;
    memcpy(data_info.data, data, len);

    if (!user_data_write_sub((u8 *)&data_info, 2 + len)) {
        log_error("hid buffer full!!!");
        return -3;
    }

    return 0;
}

void edr_hid_key_deal_test(u16 key_msg)
{
    //default report_id 1
    edr_hid_data_send(1, (u8 *)&key_msg, 2);

    if (key_msg) {
        key_msg = 0;
        edr_hid_data_send(1, (u8 *)&key_msg, 2);
    }
}

int edr_hid_is_connected(void)
{
    return hid_channel;
}


#if TCFG_BT_PROFILE_HID_CHANGE_DESCRIPTOR
static const u8 use_hid_descriptor[] = {
    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x01,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,
    0x75, 0x05,
    0x95, 0x01,
    0x81, 0x03,
    0x05, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x09, 0x30,
    0x09, 0x31,
    0x16, 0x00, 0xD8,
    0x26, 0x00, 0x28,
    0x75, 0x10,
    0x95, 0x02,
    0x81, 0x06,
    0xC0,
    0xC0,

    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,
    0x85, 0x02,
    0x15, 0x00,
    0x25, 0x01,
    0x09, 0x34,
    0x09, 0x40,
    0x0A, 0x23, 0x02,
    0x0A, 0x24, 0x02,
    0x09, 0xE9,
    0x09, 0xEA,
    0x09, 0xB0,
    0x09, 0xB1,
    0x09, 0xB3,
    0x09, 0xB4,
    0x09, 0xB5,
    0x09, 0xB6,
    0x09, 0xB7,
    0x09, 0xCD,
    0x75, 0x01,
    0x95, 0x0E,
    0x81, 0x22,
    0x75, 0x01,
    0x95, 0x02,
    0x81, 0x02,
    0xC0
};

struct user_hid_consumer_cmd {
    //Bluetooth HID Protocol Message Header Octet
    u8 HIDP_Hdr;
    //Bluetooth HID Boot Reports
    u8 report_id;
    u8 button;
} _GNU_PACKED_;

static struct user_hid_consumer_cmd u_consumer = {
    .HIDP_Hdr = 0xA1,
    .report_id = 0x02,
    .button = 0,
};

u8 sdp_user_hid_service_data[0x200];

// consumer key
#define CONSUMER_MENU               0x01
#define CONSUMER_MENU_ESCAPE        0x02
#define CONSUMER_AC_HOME            0x04

void hid_consumer_send_test(u8 menu)
{
    if (menu == 1) {
        u_consumer.button = CONSUMER_MENU;
    }
    if (menu == 2) {
        u_consumer.button = CONSUMER_MENU_ESCAPE;
    }
    if (menu == 3) {
        u_consumer.button = CONSUMER_AC_HOME;
    }
    put_buf((u8 *)&u_consumer, sizeof(u_consumer));
    user_data_write_sub((u8 *)&u_consumer, sizeof(u_consumer));
    os_time_dly(2); //需要加延迟，连续发送会返回繁忙
    u_consumer.button = 0x00;
    user_data_write_sub((u8 *)&u_consumer, sizeof(u_consumer));
}

static void user_hid_sdp_init(const u8 *hid_descriptor, u16 size)
{
    int real_size = bt_sdp_create_diy_hid_service(sdp_user_hid_service_data, sizeof(sdp_user_hid_service_data), hid_descriptor, size);
    log_info("dy_hid_service(%d):", real_size);
    //客户自行更改自定义的sdp_user_hid_service_data
}

void user_hid_descriptor_init(void)
{
    //bt_change_hci_class_type(0x240418);  /*需要更换手机显示图标的可以自己改成以前的值*/
    user_hid_init(NULL);
    user_hid_sdp_init(use_hid_descriptor, sizeof(use_hid_descriptor));
}

#endif

#endif
