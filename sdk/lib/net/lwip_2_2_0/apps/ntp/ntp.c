/**
 * @author  Artium Nihamkin <artium@nihamkin.com>
 * @date May 2016
 *
 * @section LICENSE
 *
 * The MIT License (MIT)
 * Copyright © 2016 Artium Nihamkin, http://nihamkin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Test program for the ntp library. It uses UNIX sockets.
 */


#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <string.h>
#include "lwip.h"
#include "time.h"
#include "ntp/ntp.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "device/device.h"
#include "system/sys_time.h"
#include "system/init.h"
#include "system/timer.h"
#include "event/net_event.h"
#include "wifi/wifi_connect.h"

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#error "ntp.h: Your compiler does not support packed attribute"
#endif

#if NTP_ERR_INFO_ON
#define NTP_ERR_PRINTF	printf
#else
#define NTP_ERR_PRINTF
#endif


#if NTP_DBUG_INFO_ON
#define NTP_DBG_PRINTF             printf
#else
#define NTP_DBG_PRINTF
#endif

#define NTP_ORIGIN_YEAR 1900
#define NTP_EPOCH_DELTA 2208988800ULL  // 1900-1970的秒数差
#define TIMEOUT_BASE_MS 1000
#define MAX_RETRY 10

typedef union {
    struct {
        char integer;
        unsigned char fraction;
    };
    int raw;
} fixed_16_16_t;

typedef union {
    struct {
        unsigned int integer;
        unsigned int fraction;
    };
    unsigned long long raw;
} ntp_timestamp_t;



typedef enum  {
    NO_WARNING                 = 0,
    LAST_MINUTE_HAS_61_DECONDS = 1,
    LAST_MINUTE_HAS_59_DECONDS = 2,
    ALARM_CONDITION            = 3,
} ntp_leap_indicator_t;

typedef enum  {
    RESERVED          = 0,
    SYMMETRIC_ACTIVE  = 1,
    SYMMETRIC_PASSIVE = 2,
    CLIENT            = 3,
    SERVER            = 4,
    BROADCAST         = 5,
    RESERVED_6        = 6,
    RESERVED_7        = 7,
} ntp_mode_t;

/*
 * This is the main NTP structure that represents an NTP message.
 * The structure is the same for every message that is being sent
 * both by the server and by the client.
 * The content of the message may differ, some fields are unused when
 * the client sends the message.
 *
 * Notice: values are in network byte order.
 *
 */
typedef struct PACKED {
    uint8_t          flags;
    uint8_t          stratum;
    uint8_t          poll;
    int8_t           precision;
    fixed_16_16_t    root_delay;
    fixed_16_16_t    root_dispersion;
    uint8_t          ref_id[4];
    ntp_timestamp_t  reference_timestamp;
    ntp_timestamp_t  originate_timestamp;
    ntp_timestamp_t  receive_timestamp;
    ntp_timestamp_t  transmit_timestamp;
    //增加这两个拓展参数会导致一些ntp服务器无回复
    /* unsigned int        key_identifier;           // not used by sntp */
    /* unsigned int        mesage_digest[4];         // not uses by sntp */
} ntp_packet_t;


//////////////////////////////////////////////////////////////
//解析并打印接收到ntp数据信息
static void print_fixed_point(const char *label, fixed_16_16_t fixed)
{
    NTP_DBG_PRINTF("%-18s: %d.%03d\n", label,
                   (int8_t)fixed.integer,
                   (fixed.fraction * 1000) / 256);
}

static void print_ntp_timestamp(const char *label, ntp_timestamp_t ts)
{
    uint32_t seconds = ntohl(ts.integer);
    uint32_t fraction = ntohl(ts.fraction);

    if (seconds == 0) {
        NTP_DBG_PRINTF("%-18s: <null>\n", label);
        return;
    }

    // 转换为UNIX时间
    time_t unix_sec = seconds - NTP_EPOCH_DELTA;
    uint32_t microsec = (uint64_t)fraction * 1000000ULL >> 32;

    // 格式化为可读时间
    struct tm *timeinfo = gmtime(&unix_sec);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    NTP_DBG_PRINTF("%-18s: %s.%06d UTC\n", label, buffer, microsec);
}

// 打印整个NTP数据包
static void print_ntp_packet(const ntp_packet_t *pkt)
{
    uint8_t li = (pkt->flags >> 6) & 0x03;
    uint8_t vn = (pkt->flags >> 3) & 0x07;
    uint8_t mode = pkt->flags & 0x07;

    NTP_DBG_PRINTF("================= NTP 协议分析 =================\n");
    NTP_DBG_PRINTF("%-18s: LI=%d VN=%d Mode=%d\n", "Header Flags", li, vn, mode);
    NTP_DBG_PRINTF("%-18s: %u\n", "Stratum", pkt->stratum);
    NTP_DBG_PRINTF("%-18s: %d\n", "Poll", pkt->poll);
    NTP_DBG_PRINTF("%-18s: %d\n", "Precision", pkt->precision);

    print_fixed_point("Root Delay", pkt->root_delay);
    print_fixed_point("Root Dispersion", pkt->root_dispersion);

    NTP_DBG_PRINTF("%-18s: %d.%d.%d.%d\n", "Ref Identifier",
                   pkt->ref_id[0], pkt->ref_id[1], pkt->ref_id[2], pkt->ref_id[3]);

    print_ntp_timestamp("Ref Timestamp", pkt->reference_timestamp);
    print_ntp_timestamp("Originate", pkt->originate_timestamp);
    print_ntp_timestamp("Receive", pkt->receive_timestamp);
    print_ntp_timestamp("Transmit", pkt->transmit_timestamp);
    NTP_DBG_PRINTF("=================================================\n");
}
////////////////////////////////////////////////////////////
//
#define NTP_PORT 123

/*
 * This is the message that is sent from client to server.
 *
 * It's definition is:
 *    mode - CLIENT (4)
 *    vn   - 4
 *
 * @notice Transmit timestamp is optional.
 *
 */
#define NTP_REQUEST_MSG { .flags = 0b000100011 } ;

#define NTP_ORIGIN_YEAR 1900 /// aka epoch
#define NTP_GET_LI(b1)   ((ntp_leap_indicator_t)(((b1) & 0b11000000) >> 6))
#define NTP_GET_VN(b1)                          (((b1) & 0b00111000) >> 3)
#define NTP_GET_MODE(b1) ((ntp_mode_t)          (((b1) & 0b00000111)))

#define NTP_GET_TS_SECONDS_AFTER_MINUTE(ts)  ((ntohl((ts).integer)      )  % 60)
#define NTP_GET_TS_MINUTES_AFTER_HOUR(ts)    ((ntohl((ts).integer) /  60)  % 60)
#define NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts)  ((ntohl((ts).integer) / 3600) % 24)

#define NTP_GET_TS_DAYS_SINCE_JAN_1_1900(ts) (ntohl((ts).integer) / 86400)

#define NTP_IS_LEAP_YEAR(y) ((((y)%4 == 0) && ((y)%100 != 0)) || (y)%400 == 0)

extern u32 timer_get_sec(void);

static void ntp_get_time(struct tm *tmr)
{
    printf("%4d:%02d:%02d   %02d:%02d:%02d \n\n"
           , tmr->tm_year + 1900
           , tmr->tm_mon + 1
           , tmr->tm_mday
           , tmr->tm_hour
           , tmr->tm_min
           , tmr->tm_sec);
}

#if 0
struct host_name {
    struct list_head entry;
    char *name;
};

struct ntp_server_data {
    struct list_head host_name_head;
    int fd;
    u32 timeout;
    u32 houroffset;
    u32 query_interval;
    u32 close_flag;
    void (*get_time_cb)(struct tm *t);
};

static struct ntp_server_data ntp_data = {
    .houroffset = 8,
    .query_interval = 1,
    .get_time_cb = ntp_get_time,
};

void ntp_set_query_interval_min(u32 min)
{
    ntp_data.query_interval = min;
}

void ntp_set_timezone(u32 zone)
{
    ntp_data.houroffset = zone;
}

void ntp_set_time_cb(void (*f)(struct tm *t))
{
    ntp_data.get_time_cb = f;
}

int ntp_add_host_name(char *name)
{
    struct host_name *n = malloc(sizeof(struct host_name));

    if (n == NULL) {
        return -1;
    }

    n->name = name;
    list_add_tail(&n->entry, &ntp_data.host_name_head);

    return 0;
}

int ntp_free_all_host_name(void)
{
    struct list_head *pos, *node;
    struct host_name *n;
    list_for_each_safe(pos, node, &ntp_data.host_name_head) {
        n = list_entry(pos, struct host_name, entry);
        list_del(&n->entry);
        free(n);
    }

    return 0;
}

int ntp_remove_host_name(char *name)
{
    struct list_head *pos, *node;
    struct host_name *n;
    list_for_each_safe(pos, node, &ntp_data.host_name_head) {
        n = list_entry(pos, struct host_name, entry);

        if (!strcmp(name, n->name)) {
            list_del(&n->entry);
            free(n);
            return 0;
        }
    }
    NTP_DBG_PRINTF("ntp not find host name \n");

    return -1;
}
#endif

/*
 * @brief Extracts date from the timestamp.
 *
 * The following note from the RFC is not implemented:
 * "If bit 0 is set, the UTC time is in the range 1968-
 *  2036, and UTC time is reckoned from 0h 0m 0s UTC on 1 January
 *  1900.  If bit 0 is not set, the time is in the range 2036-2104 and
 *  UTC time is reckoned from 6h 28m 16s UTC on 7 February 2036.  Note
 *  that when calculating the correspondence, 2000 is a leap year, and
 *  leap seconds are not included in the reckoning."
 *
 * @param ts[in]     time stamp in network byte order
 * @param year[out]  1900 to 2037, no correction if bit 0 is not set.
 * @param month[out] 0 to 11.
 * @param day[out]   0 to 30.
 */
static void ntp_get_date(ntp_timestamp_t ts, unsigned int *year,
                         unsigned int *month, unsigned int *day)
{
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t days = ntohl(ts.integer) / 86400;  // 从1900年1月1日开始的天数

    *year = NTP_ORIGIN_YEAR;
    while (days > 0) {
        uint16_t year_days = NTP_IS_LEAP_YEAR(*year) ? 366 : 365;
        if (days < year_days) {
            break;
        }
        days -= year_days;
        (*year)++;
    }

    *month = 0;
    while (*month < 12) {
        uint8_t m_days = days_in_month[*month];
        if (*month == 1 && NTP_IS_LEAP_YEAR(*year)) {
            m_days++;
        }

        if (days < m_days) {
            break;
        }
        days -= m_days;
        (*month)++;
    }

    *day = days;
}

#if 0
static inline void analysis_ntp_protocol(ntp_timestamp_t ts, struct tm *s_tm)
{
    unsigned int y, mo, d;
    unsigned int s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    unsigned int mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    unsigned int h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);
    ntp_get_date(ts, &y, &mo, &d);
    s_tm->tm_sec = s;
    s_tm->tm_min = mi;
    s_tm->tm_hour = h + ntp_data.houroffset;
    s_tm->tm_mday = d + 1;
    s_tm->tm_mon = mo + 1; //fake
    s_tm->tm_year = y;
}

static inline u32 get_time_diff(ntp_timestamp_t ts)
{
    static u32 tmp = 0;
    u32 sec;
    u32 min;
    u32 hour;
    sec = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    min = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    hour = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    printf("tmp %d time %d \n", tmp, sec + 60 * min + 3600 * hour);

    if (tmp == 0) {
        tmp = sec + 60 * min + 3600 * hour;
        return 0;
    } else {
        return ((sec + 60 * min + 3600 * hour) - tmp);
    }

}

void ntp_thread(void *arg)
{
    int msg[32];
    int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    struct list_head *pos;
    struct host_name *n;
    u8 is_already_set_time = 1;
    struct tm s_tm;
    u32 flag;
    int err;
    u32 count = ntp_data.query_interval * 60;
    if ((ntp_data.fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_DBG_PRINTF("ntp cannot create socket");
        return ;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(0);
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ntp_data.fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_DBG_PRINTF("ntp bind fail \n");
        return ;
    }

    flag = fcntl(ntp_data.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    err = fcntl(ntp_data.fd, F_SETFL, flag);
    if (err < 0) {
        NTP_DBG_PRINTF("std stdion to non-block fails\n");
        return ;
    }

    while (1) {

        if (ntp_data.close_flag) {
            goto err;
        }

        err = os_taskq_pend_timeout(msg, ARRAY_SIZE(msg), 100);
        if (err == OS_Q_EMPTY) {
            if (count == ntp_data.query_interval * 60) { //60s
                count = 0;
                is_already_set_time = 0;
                list_for_each(pos, &ntp_data.host_name_head) {
                    n = list_entry(pos, struct host_name, entry);
                    hp = gethostbyname(n->name);

                    NTP_DBG_PRINTF("will attemt to communicate with %s\n", n->name);

                    if (!hp) {
                        NTP_DBG_PRINTF("could not obtain address of %s\n", n->name);
                        continue;
                    }
                    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
                    NTP_DBG_PRINTF("sending data..\n");
                    if (sendto(ntp_data.fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                        NTP_DBG_PRINTF("sendto failed");
                        continue;
                    }
                }
            }

            count++;

            i = recv(ntp_data.fd, &server_msg, sizeof(server_msg), 0);
            /* NTP_DBG_PRINTF("recieved: %d out of %lu bytes \n", i, sizeof(server_msg)); */
            if (!is_already_set_time && i > 0) {

                analysis_ntp_protocol(server_msg.recieve_timestamp, &s_tm);

                if (ntp_data.get_time_cb) {
                    ntp_data.get_time_cb(&s_tm);
                }

                /* is_already_set_time = 1; */

            }
        }

    }

err:
    ntp_free_all_host_name();
    closesocket(ntp_data.fd);
}

void ntp_thread(void *arg)
{
    int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    struct list_head *pos;
    struct host_name *n;
    fd_set rdfs;
    int retval = 0;
    struct timeval tv;
    u8 is_already_set_time = 1;
    u8 count = 0;
    struct tm s_tm;
    u32 recv_timout_tocal = 0;
    tv.tv_sec = ntp_data.query_interval;

    if ((ntp_data.fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_DBG_PRINTF("ntp cannot create socket");
        return;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(0);
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ntp_data.fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_DBG_PRINTF("ntp bind fail \n");
        return;
    }

    while (1) {
        if (ntp_data.close_flag) {
            goto exit;
        }

        FD_ZERO(&rdfs);
        FD_SET(ntp_data.fd, &rdfs);

        NTP_DBG_PRINTF("set timeout %d sec \n", tv.tv_sec);
        retval = select(ntp_data.fd + 1, &rdfs, NULL, NULL, &tv);

        if (retval < 0) {
            goto exit;
        } else if (retval == 0) {
            count++;
            if (count == 10) {
                goto exit;
            }
            is_already_set_time = 0;
            NTP_DBG_PRINTF("%d sec send of one time \n\n", ntp_data.query_interval);
            tv.tv_sec = ntp_data.query_interval;
            recv_timout_tocal += tv.tv_sec;
            list_for_each(pos, &ntp_data.host_name_head) {
                n = list_entry(pos, struct host_name, entry);
                hp = gethostbyname(n->name);

                NTP_DBG_PRINTF("will attemt to communicate with %s\n", n->name);

                if (!hp) {
                    NTP_DBG_PRINTF("could not obtain address of %s\n", n->name);
                    continue;
                }

                memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
                NTP_DBG_PRINTF("sending data..\n");

                if (sendto(ntp_data.fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                    NTP_DBG_PRINTF("sendto failed");
                    goto exit;
                }

            }

        } else {
            count = 0;
            NTP_DBG_PRINTF("recieving data..\n");
            i = recv(ntp_data.fd, &server_msg, sizeof(server_msg), 0);
            NTP_DBG_PRINTF("received: %u out of %lu bytes \n", i, sizeof(server_msg));

            //如果需要更精确，修改这函数
            analysis_ntp_protocol(server_msg.recieve_timestamp, &s_tm);

            if (recv_timout_tocal <= ntp_data.query_interval) {
                tv.tv_sec = ntp_data.timeout * 60 ;
            } else {
                tv.tv_sec = ntp_data.timeout * 60 - recv_timout_tocal;
            }

            recv_timout_tocal = 0;

            if (!is_already_set_time) {
                if (ntp_data.get_time_cb) {
                    ntp_data.get_time_cb(&s_tm);
                }
                is_already_set_time = 1;
            }
        }
    }

exit:
    ntp_free_all_host_name();
    closesocket(ntp_data.fd);
}

static int ntp_thread_pid = 0;

void ntp_client_init(void)
{
    INIT_LIST_HEAD(&ntp_data.host_name_head);
    /* ntp_add_host_name("www.catcoder.cn"); */
    /* ntp_add_host_name("time.nist.gov"); */
    ntp_set_time_cb(ntp_get_time);
    ntp_set_query_interval_min(1);
}

int ntp_client_start(void)
{
    return thread_fork("ntp_thread", NTP_CLIENT_THREAD_PRIO, NTP_CLIENT_THREAD_STACK_SIZE, 0, &ntp_thread_pid, ntp_thread, NULL);
}

void ntp_client_uninit(void)
{
    ntp_data.close_flag = 1;
    thread_kill(&ntp_thread_pid, KILL_WAIT);
    ntp_data.close_flag = 0;
}
#endif

static void analysis_ntp_protocol_ext(ntp_timestamp_t ts, struct tm *s_tm)
{
    uint32_t seconds = ntohl(ts.integer);
    time_t unix_sec = seconds - NTP_EPOCH_DELTA;
    struct tm *timeinfo = gmtime(&unix_sec);

    if (timeinfo) {
        memcpy(s_tm, timeinfo, sizeof(struct tm));
    } else {
        memset(s_tm, 0, sizeof(struct tm));
    }
}

static u8 net_time_init_flag = 0;
static u8 ntp_time_exit = 0;
static volatile u8 ntp_time_running = 0;
static volatile u8 rtc_update_running = 0;
static u16 rtc_update_timer = 0;
extern const u32 rtc_update_interval;
static OS_SEM ntp_sem;

static void update_rtc_by_ntp(const char *host)
{
    struct sys_time st;
    int ret = -1;
    struct tm pt;

    while (ret) {
        if (!host) {
            ret = ntp_client_get_time_all(&pt, 4000);
        } else {
            ret = ntp_client_get_time_once(host, &pt, 4000);
        }

        if (ret) {
            os_time_dly(100);
        }
    }
    time_t mt = mktime(&pt);
    set_sys_source_timer(mt);
    set_time_compensate_sec(timer_get_sec());
    set_time_compensate_ms(timer_get_ms());
    set_rtc_time_base_on_unix_timestamp(mt);

    struct net_event evt = {0};
    evt.arg = "net";
    evt.event = NET_NTP_GET_TIME_SUCC;
    net_event_notify(NET_EVENT_FROM_USER, &evt);
}

static void update_rtc_task(void *priv)
{
    if (rtc_update_running) {
        NTP_ERR_PRINTF("update_rtc_task is still running...\n");
        return;
    }

    rtc_update_running = 1;
    update_rtc_by_ntp(NULL);
    rtc_update_running = 0;
}

static void update_rtc_call(void *priv)
{
    enum wifi_sta_connect_state state;
    state = wifi_get_sta_connect_state();

    if (!rtc_update_running && (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state)) {
        thread_fork("update_rtc_task", 1, 512, 0, NULL, update_rtc_task, NULL);
    }
}

static void update_rtc_timer(const u32 msec)
{
    if (rtc_update_timer) {
        return;
    }

    rtc_update_timer = sys_timer_add_to_task("sys_timer", NULL, update_rtc_call, msec);
    if (!rtc_update_timer) {
        printf("update_rtc_timer err!");
    }
}

static int ntp_client_init(void)
{
    return os_sem_create(&ntp_sem, 0);
}
early_initcall(ntp_client_init);

u8 ntp_client_get_time_status(void)
{
    return net_time_init_flag;
}

void ntp_client_get_time_clear(void)
{
    net_time_init_flag = 0;
}

void ntp_client_get_time_exit(void)
{
    ntp_time_exit = 1;
    os_sem_post(&ntp_sem);
}

void ntp_client_get_time(const char *host)
{
    if (ntp_time_running) {
        NTP_ERR_PRINTF("ntp_client_get_time is still running...\n");
        return ;
    }
    ntp_time_running = 1;

    struct tm pt;
    int timeout_cnt = 0, ret;

    ntp_time_exit = 0;
    os_sem_set(&ntp_sem, 0);

    memset(&pt, 0, sizeof(struct tm));

    while (!ntp_time_exit) {
        if (!host) {
            ret = ntp_client_get_time_all(&pt, 4000);
        } else {
            ret = ntp_client_get_time_once(host, &pt, 4000);
        }
        if (!ret) {
            time_t mt = mktime(&pt);
            set_sys_source_timer(mt);
            set_time_compensate_sec(timer_get_sec());
            set_time_compensate_ms(timer_get_ms());
            set_rtc_time_base_on_unix_timestamp(mt);

            //notify
            struct net_event evt = {0};
            evt.arg = "net";
            evt.event = NET_NTP_GET_TIME_SUCC;
            net_event_notify(NET_EVENT_FROM_USER, &evt);

            set_local_time_init_flag(1);
            net_time_init_flag = 1;
            if (rtc_update_interval) {
                update_rtc_timer(rtc_update_interval);
            }

            break;
        } else {
            if (timeout_cnt > 10) {
                ntp_time_exit = 1;
                timeout_cnt = 0;
            }
            os_sem_pend(&ntp_sem, ++timeout_cnt * 100);
        }
    }
    ntp_time_running = 0;
}

int ntp_client_get_time_all(struct tm *s_tm, int recv_to)
{
    const int num_hosts = NTP_HOST_NUM;
    int ret = -1;

    if (!s_tm) {
        return -1;
    }

    for (int i = 0; i < num_hosts; i++) {
        if (ntp_client_get_time_once(ntp_host[i], s_tm, recv_to) == 0) {
            ret = 0;
            break;
        }
    }
    return ret;
}

bool is_valid_ntp_response(ntp_packet_t *packet)
{
    bool all_zeros = true;
    for (size_t i = 1; i < sizeof(ntp_packet_t); i++) {
        if (((uint8_t *)packet)[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    if (all_zeros) {
        return false; // 检测到全0包
    }
    // 验证NTP头部的标志位（正确响应应为0x24）
    const uint8_t valid_flags = 0x24; // LI=0, VN=4, Mode=4 (server)
    if ((packet->flags & 0xFF) != valid_flags) {
        return false;
    }

    return true;

    /* uint32_t seconds = ntohl(packet->transmit_timestamp.integer); */
    /* time_t timestamp = seconds - NTP_EPOCH_DELTA; */

    /* const time_t MIN_VALID_TIME = 1609459200; // 2021-01-01 */
    /* const time_t MAX_VALID_TIME = 2524608000; // 2050-01-01 */

    /* return (timestamp >= MIN_VALID_TIME) & (timestamp <= MAX_VALID_TIME); */
}

int ntp_client_get_time_once(const char *host, struct tm *s_tm, int recv_to)
{
    if (!recv_to) {
        recv_to = 2000;
    }
    if (s_tm == NULL) {
        return -1;
    }
    if (host == NULL) {
        host = "s2c.time.edu.cn";
    }
    ntp_packet_t client_msg = {.flags = 0x23};  // Client mode, version 4
    ntp_packet_t server_msg = {0};
    struct sockaddr_in servaddr = {0};
    struct sockaddr_in localaddr = {0};
    struct hostent *hp;
    int fd, ret = -1;
    socklen_t addr_len;

    if (!host || !s_tm) {
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_ERR_PRINTF("create socket failed: %d\n", errno);
        return -1;
    }

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = 0;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_ERR_PRINTF("bind failed: %d\n", errno);
        goto cleanup;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_to, sizeof(recv_to));

    if (!(hp = gethostbyname(host))) {
        NTP_ERR_PRINTF("host lookup failed: %s\n", host);
        goto cleanup;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(NTP_PORT);
    memcpy(&servaddr.sin_addr, hp->h_addr, hp->h_length);
    addr_len = sizeof(servaddr);

    for (int attempt = 0; attempt < MAX_RETRY; attempt++) {
        if (sendto(fd, &client_msg, sizeof(client_msg), 0,
                   (struct sockaddr *)&servaddr, addr_len) < 0) {
            NTP_ERR_PRINTF("sendto failed(%d): %d\n", attempt, errno);
            continue;
        }
        while (1) {
            ssize_t received = recvfrom(fd, &server_msg, sizeof(server_msg), 0,
                                        (struct sockaddr *)&servaddr, &addr_len);
            if (received < 0) {
                NTP_DBG_PRINTF("recvfrom failed");
                break;
            }
            if (received > 0) {
                print_ntp_packet(&server_msg); //打印接收到的ntp数据包
                if (is_valid_ntp_response(&server_msg)) {
                    ntp_timestamp_t timestamp = (server_msg.transmit_timestamp.raw != 0) ?
                                                server_msg.transmit_timestamp : server_msg.receive_timestamp;
                    analysis_ntp_protocol_ext(timestamp, s_tm);
                    ret = 0;
                    goto cleanup;
                } else {
                    NTP_ERR_PRINTF("Received invalid NTP packet, ignoring\n");
                }
            }
        }
    }

cleanup:
    closesocket(fd);
    return ret;
}

#if 0
void printf_ts(char header[], ntp_timestamp_t ts)
{
    unsigned int y, mo, d;
    unsigned int s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    unsigned int mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    unsigned int h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);

    ntp_get_date(ts, &y, &mo, &d);
    printf("%s\t= ", header);
    printf("%02u:%02u:%02u %02u/%02u/%02u\n", h, mi, s, d + 1, mo + 1, y);
}

int ntp_test(void)
{
    const char const *host   = "www.catcoder.cn";
    const int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    int fd;
    printf("will attemt to communicate with %s\n", host);

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("cannot create socket");
        return 0;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(0);
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr));

    /* look up the address of the server given its name */
    hp = gethostbyname(host);

    if (!hp) {
        printf("could not obtain address of %s\n", host);
        return 0;
    }

    /* put the host's address into the server address structure */
    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    printf("sending data..\n");

    /* send a message to the server */
    if (sendto(fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("sendto failed");
        return 0;
    }

    printf("recieving data..\n");

    i = recv(fd, &server_msg, sizeof(server_msg), 0);

    printf("recieved: %u out of %lu bytes \n", i, sizeof(server_msg));

    printf("LI\t\t\t= %u\n",   NTP_GET_LI(server_msg.byte_1));
    printf("VN\t\t\t= %u\n",   NTP_GET_VN(server_msg.byte_1));
    printf("MODE\t\t\t= %u\n", NTP_GET_MODE(server_msg.byte_1));

    printf("STRATUM\t\t\t= %u\n", server_msg.stratum);

    printf("POLL INTERVAL\t\t= %u\n",         server_msg.poll);
    printf("PRECISION\t\t= %d\n",             server_msg.precision);
    printf("ROOT DELAY\t\t= %d\n",      ntohl(server_msg.root_delay.raw));
    printf("ROOT DISPRESION\t\t= %d\n", ntohl(server_msg.root_dispersion.raw));
    printf("REF ID\t\t\t= %s\n",            server_msg.reference_identifier);

    printf_ts("ORIGINATE_TIMESTAMP", server_msg.originate_timestamp);
    printf_ts("REFERENCE TIMESTAMP", server_msg.reference_timestamp);
    printf_ts("RECIEVE TIMESTAMP", server_msg.recieve_timestamp);
    printf_ts("TRANSMIT TIMESTAMP", server_msg.transmit_timestamp);

    return 0;
}
#endif

