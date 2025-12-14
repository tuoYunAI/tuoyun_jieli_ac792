#include "system/includes.h"
#include "traffic.h"
#include "app_protocol.h"
#include "app_event.h"
#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "lwip/sockets.h"


#define LOG_TAG             "[TRAFFIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"



typedef struct 
{
    media_parameter_t media_param;
    uint32_t rx_seq;
    uint32_t tx_seq;
    int downlink_audio_packet_len;
    mbedtls_aes_context aes_ctx; // AES 上下文
    struct sockaddr_in remote_addr; // 远程地址信息
    void *udp_fd
}traffic_state_machine_t, *traffic_state_machine_ptr;



static traffic_state_machine_t m_traffic_state;;


/**
 * 在没有语音包发送之前, 先发送空包来创建UDP通道, 否则服务端不知道设备真实的的IP和端口
 */

static u8 audio_transmission_started = 0;
static void traffic_receive_task();
static void traffic_receive_monitor_task();
static void traffic_uplink_empty_task();


void test_print_traffic_state(){
    log_info(
        "+++++++++++++++++++++traffic status+++++++++++++++\r\n"
        "        media_param.ip: %s\r\n"
        "        media_param.port: %d\r\n"
        "        media_param.codec: %s\r\n"
        "        media_param.transport: %s\r\n"
        "        media_param.sample_rate: %d\r\n"
        "        media_param.channels: %d\r\n"
        "        media_param.frame_duration: %d\r\n"
        "        media_param.encryption: %s\r\n"
        "        media_param.nonce: %s\r\n"
        "        media_param.aes_key: %s\r\n"
        "++++++++++++++++++++++++++++++++++++++++++++++++\r\n",         
         m_traffic_state.media_param.ip[0] ? m_traffic_state.media_param.ip : "null",
         m_traffic_state.media_param.port,
         m_traffic_state.media_param.codec[0] ? m_traffic_state.media_param.codec : "null",
         m_traffic_state.media_param.transport[0] ? m_traffic_state.media_param.transport : "null",
         m_traffic_state.media_param.sample_rate,
         m_traffic_state.media_param.channels,
         m_traffic_state.media_param.frame_duration,
         m_traffic_state.media_param.encryption[0] ? m_traffic_state.media_param.encryption : "null",
         m_traffic_state.media_param.nonce[0] ? m_traffic_state.media_param.nonce : "null", 
         m_traffic_state.media_param.aes_key[0] ? m_traffic_state.media_param.aes_key : "null"
    );
}


/**
 * 
 *  比特率 (kbps)	每帧字节数
    6 kbps	(6000 × 0.06) / 8 = 45 bytes
    8 kbps	60 bytes
    12 kbps	90 bytes
    16 kbps	120 bytes
    20 kbps	150 bytes
    24 kbps	180 bytes
    32 kbps	240 bytes
    48 kbps	360 bytes
    64 kbps	480 bytes
 */

int sample_rate_to_audio_packet_len(int sample_rate){
    switch (sample_rate){
        case 6000:
            return 45; // 6kbps
        case 8000:
            return 60; // 6kbps
        case 16000:
            return 120; // 12kbps
        case 20000:
            return 150; // 15kbps
        case 24000:
            return 180; // 18kbps
        case 32000:
            return 240; // 24kbps
        case 48000:
            return 360; // 36kbps
        case 64000:
            return 480; // 48kbps
        default:
            log_info("unsupported sample rate: %d", sample_rate);
            return -1;
    }
}

int clear_traffic_tunnel(){
    if(m_traffic_state.udp_fd){
        sock_unreg(m_traffic_state.udp_fd);
        m_traffic_state.udp_fd = NULL;
    }
    mbedtls_aes_free(&m_traffic_state.aes_ctx);
    memset(&m_traffic_state.media_param, 0, sizeof(m_traffic_state.media_param));
    memset(&m_traffic_state.remote_addr, 0, sizeof(m_traffic_state.remote_addr));
}

int start_traffic_tunnel(media_parameter_ptr param){
    if (!param){
        log_info("invalid media param\n");
        return -1;
    }
    if (param->ip[0]== '\0' || param->port == 0){
        log_info("invalid remote host: %s, %d", param->ip, param->port);
        return -1;
    }
    memcpy(&m_traffic_state.media_param, param, sizeof(media_parameter_t));
    log_info("start_traffic_tunnel, media param: ip=%s, port=%d, codec=%s, transport=%s, sample_rate=%d, channels=%d, frame_duration=%d\n",
            m_traffic_state.media_param.ip,
            m_traffic_state.media_param.port,
            m_traffic_state.media_param.codec,
            m_traffic_state.media_param.transport,
            m_traffic_state.media_param.sample_rate,
            m_traffic_state.media_param.channels,
            m_traffic_state.media_param.frame_duration
        );

    m_traffic_state.downlink_audio_packet_len = sample_rate_to_audio_packet_len(m_traffic_state.media_param.sample_rate);
    if (m_traffic_state.downlink_audio_packet_len <= 0){
        log_info("unsupported sample rate: %d", m_traffic_state.media_param.sample_rate);
        return -1;
    }

    struct sockaddr_in* remote_addr = &m_traffic_state.remote_addr;
    remote_addr->sin_family = AF_INET;
    remote_addr->sin_addr.s_addr = inet_addr(m_traffic_state.media_param.ip);
    remote_addr->sin_port = htons(m_traffic_state.media_param.port);

    do{
        
        // 初始化 AES 上下文
        mbedtls_aes_init(&m_traffic_state.aes_ctx);
        
        // 设置加密密钥
        int ret = mbedtls_aes_setkey_enc(&m_traffic_state.aes_ctx, m_traffic_state.media_param.aes_key, 128);
        if (ret != 0) {
            log_info("mbedtls_aes_setkey_enc failed: %d\n", ret);
            mbedtls_aes_free(&m_traffic_state.aes_ctx);
            break;
        }

        m_traffic_state.udp_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
        if(NULL == m_traffic_state.udp_fd) {
            log_info("sock_reg fail\n");
            break;
        }

        audio_transmission_started = 0;
        if (thread_fork("protocol_audio_demo", 16, 1024, 0, NULL, traffic_uplink_empty_task, NULL) != OS_NO_ERR) {
            log_info("thread audio demo fork fail\n");
        }
        if (thread_fork("protocol_audio", 28, 1024, 0, NULL, traffic_receive_task, NULL) != OS_NO_ERR) {
            log_info("thread audio receive fork fail\n");
            break;
        }
        if (thread_fork("traffic_receive_monitor_task", 20, 1024, 0, NULL, traffic_receive_monitor_task, NULL) != OS_NO_ERR) {
            log_info("thread audio receive fork fail\n");
            break;
        }
        
        return 0;
    }while(0);

    clear_traffic_tunnel();

    return -1;

}

void set_start_audio_transmission_flag(){
    audio_transmission_started = 1;
}

u8 send_buf[2048];
int send_audio(audio_stream_packet_ptr packet){
    if (!packet || !packet->payload || packet->payload_len <= 0){
        return -1;
    }

    if (!check_if_session_in_call() || !m_traffic_state.udp_fd){
        log_info("Audio channel not established\n");
        return -1;
    }   
  
    if (packet->payload_len + 16 > sizeof(send_buf)){
        log_info("Audio packet too large: %d bytes\n", packet->payload_len);
        return -1;
    }

    memset(send_buf, 0, sizeof(send_buf));
    // 准备CTR模式的计数器（基于nonce + 序列号）
    uint8_t ctr_block[16];
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    memcpy(ctr_block, m_traffic_state.media_param.nonce, 16);
    *(uint16_t*)&ctr_block[2] = htons(packet->payload_len);
    *(uint32_t*)&ctr_block[12] = htonl(++m_traffic_state.tx_seq); 
    size_t total_len = sizeof(ctr_block) + packet->payload_len;

    memcpy(send_buf, ctr_block, sizeof(ctr_block));    
    
    if (mbedtls_aes_crypt_ctr(&m_traffic_state.aes_ctx, 
                                packet->payload_len,
                                &nc_off,
                                ctr_block,
                                stream_block,
                                packet->payload,
        (uint8_t*)&send_buf[sizeof(ctr_block)]) != 0) {
        log_info("Failed to encrypt audio data");
        return -1;
    }

    // 通过UDP发送加密后的数据包
    int sent_bytes = sock_sendto(m_traffic_state.udp_fd,
                                send_buf,
                                total_len,
                                0,
                                (struct sockaddr*)&m_traffic_state.remote_addr,
                                sizeof(m_traffic_state.remote_addr));

    if (sent_bytes < 0) {
        log_info("UDP send failed\n");
        return -1;
    }
    
    //log_info("Sent encrypted audio packet: %d bytes (seq: %u, payload: %u bytes)\n",
    //         sent_bytes, m_traffic_state.tx_seq, packet->payload_len);
    return 0;
}



/**
 * 通道建立时发送空包, 创建上下行UDP通道
 */
static void traffic_uplink_empty_task(){
    char buf[2] = {0x58, 0x00};
    audio_stream_packet_t packet = {
            .payload_len = 120,
            .payload = buf
        };
    log_info("traffic demo task start\n");    
    while (audio_transmission_started == 0)
    {

        if (!check_if_session_in_call() || !m_traffic_state.udp_fd){
            log_info("demo task exit\n");
            return;
        } 

        //log_info("send demo audio packet\n");
        send_audio(&packet);
        os_time_dly(20);
        /* code */
    }
    
}



/* 重排窗口大小：可存放的最大乱序包数量 */
const int REORDER_WINDOW = 20;
typedef struct {
    uint32_t seq;
    uint8_t *payload;
    int filled;
} reorder_entry_t;

reorder_entry_t *m_cache_block = NULL;
uint8_t *m_cache_data_buf = NULL;
uint32_t m_timeout_cnt = 0;
static OS_MUTEX mutex;


static void traffic_receive_task(){
    int recv_len;
    struct sockaddr_in s_addr = {0};
    socklen_t len = sizeof(s_addr);
    static int nonce_size = 16;
    /**
     * 60ms帧, 最长不超过20K
     */
    int packet_len = m_traffic_state.downlink_audio_packet_len + nonce_size;
    int buf_len = 8*1024;
    u8* udp_recv_buf = malloc(buf_len);
    if(!udp_recv_buf){
        log_info("failed to alloc mem for udp recv buf");
        return;
    }

    audio_stream_packet_t audio_pack = {0};
    audio_pack.payload = malloc(m_traffic_state.downlink_audio_packet_len);
    if(!audio_pack.payload){
        log_info("failed to alloc mem for received data");
        return;
    }
    audio_pack.payload_len = m_traffic_state.downlink_audio_packet_len;
    uint8_t ctr_block[nonce_size];
    uint8_t stream_block[nonce_size];

    log_info("traffic receive task start, buffer size: %d\n", buf_len);
    m_traffic_state.rx_seq = 0;
    
    m_cache_block = calloc(REORDER_WINDOW, sizeof(reorder_entry_t));
    if (m_cache_block) {
        m_cache_data_buf = malloc(REORDER_WINDOW * m_traffic_state.downlink_audio_packet_len);
        if (!m_cache_data_buf) {
            free(m_cache_block);
            m_cache_block = NULL;
        } else {
            for (int i = 0; i < REORDER_WINDOW; i++) {
                m_cache_block[i].payload = m_cache_data_buf + i * m_traffic_state.downlink_audio_packet_len;
                m_cache_block[i].filled = 0;
                m_cache_block[i].seq = 0;
            }
        }
    }
    m_timeout_cnt = 0;
    for(;;){
        
        if(!check_if_session_in_call() || !m_traffic_state.udp_fd){
            log_info("traffic receive task exit\n");
            clear_traffic_tunnel();
            break;
        }
        
        recv_len = sock_recvfrom(m_traffic_state.udp_fd, udp_recv_buf, buf_len, 0, 
                                                        (struct sockaddr *)&s_addr, &len);

        //log_info("@@@@@@++++++++@@@@@@@@received %d Bytes\n", recv_len);
        if(recv_len <= nonce_size){
            log_info("received %d Bytes", recv_len);
            continue;
        }
        
        m_timeout_cnt = 0;

        //continue;
        int offset = 0;
        os_mutex_pend(&mutex, 0);
        while(offset < recv_len){
            if (recv_len - offset < packet_len){
                break;
            }

            size_t nc_off = 0;
            memcpy(ctr_block, udp_recv_buf + offset, nonce_size); // nonce from header

            long sequence = (long)ntohl(*(uint32_t*)&ctr_block[12]);
            if (m_traffic_state.rx_seq == 0) {
                m_traffic_state.rx_seq = sequence - 1; // 初始化序列号
            }

            if (sequence <= m_traffic_state.rx_seq) {
                // 已处理过或太旧，直接丢弃
                offset += packet_len;
                continue;
            }

            /* 解密到临时缓冲（audio_pack.payload 已分配） */
            int ret = mbedtls_aes_crypt_ctr(&m_traffic_state.aes_ctx,
                                            m_traffic_state.downlink_audio_packet_len,
                                            &nc_off,
                                            ctr_block,
                                            stream_block,
                                            udp_recv_buf + offset + nonce_size,
                                            audio_pack.payload);

            if (ret != 0) {
                log_info("AES decryption failed: %d\n", ret);
                offset += packet_len;
                continue;
            }

            /* 如果是期望的下一个序号，直接交付并消费后续缓冲中连续的包 */
            if ((long)(m_traffic_state.rx_seq + 1) == sequence) {
                dialog_audio_dec_frame_write(&audio_pack);
                m_traffic_state.rx_seq = sequence;

                /* 检查缓冲区中是否存在后续连续包 */
                if (m_cache_block) {
                    int progressed = 1;
                    while (progressed) {
                        progressed = 0;
                        uint32_t want = m_traffic_state.rx_seq + 1;
                        for (int i = 0; i < REORDER_WINDOW; i++) {
                            if (m_cache_block[i].filled && m_cache_block[i].seq == want) {
                                /* 将已解密数据放到 audio_pack.payload，然后交付 */
                                memcpy(audio_pack.payload, m_cache_block[i].payload, m_traffic_state.downlink_audio_packet_len);
                                dialog_audio_dec_frame_write(&audio_pack);
                                m_traffic_state.rx_seq = m_cache_block[i].seq;
                                m_cache_block[i].filled = 0;
                                progressed = 1;
                                break;
                            }
                        }
                    }
                }

                offset += packet_len;
                continue;
            }

            /* 序号大于期望：尝试放入重排缓冲区 */
            if (m_cache_block) {
                long gap = sequence - (m_traffic_state.rx_seq + 1);
                if (gap >= REORDER_WINDOW) {
                    log_info("@@@@@@Audio packet jump detected: expected seq %ld, received seq %d\n", m_traffic_state.rx_seq + 1, sequence);
                    /* 跳跃过大，视为重同步：清空缓冲并把当前包作为新的连续开始 */
                    for (int i = 0; i < REORDER_WINDOW; i++) m_cache_block[i].filled = 0;
                    /* 直接交付当前包并更新序号 */
                    dialog_audio_dec_frame_write(&audio_pack);
                    m_traffic_state.rx_seq = sequence;
                    offset += packet_len;
                    continue;
                } else {
                    log_info("@@@@@@Audio packet out-of-order detected: expected seq %ld, received seq %d\n", m_traffic_state.rx_seq + 1, sequence);
                    int idx = sequence % REORDER_WINDOW;
                    /* 存放到缓冲区（覆盖旧条目） */
                    memcpy(m_cache_block[idx].payload, audio_pack.payload, m_traffic_state.downlink_audio_packet_len);
                    m_cache_block[idx].seq = sequence;
                    m_cache_block[idx].filled = 1;
                    offset += packet_len;
                    continue;
                }
            } else {
                /* 无缓冲支持，直接检测丢包并更新序号为最新 */
                if (m_traffic_state.rx_seq + 1 != sequence) {
                    log_info("@@@@@@Audio packet loss detected: expected seq %ld, received seq %d\n", m_traffic_state.rx_seq + 1, sequence);
                }
                dialog_audio_dec_frame_write(&audio_pack);
                m_traffic_state.rx_seq = sequence;
                offset += packet_len;
                continue;
            }
        }
        os_mutex_post(&mutex);
    }
    if (m_cache_block) {
        free(m_cache_block);
        m_cache_block = NULL;
    }
    if (m_cache_data_buf) {
        free(m_cache_data_buf);
        m_cache_data_buf = NULL;
    }
    free(audio_pack.payload);
    audio_pack.payload = NULL;
}


static void traffic_receive_monitor_task(){

    audio_stream_packet_t audio_pack = {0};
    audio_pack.payload = malloc(m_traffic_state.downlink_audio_packet_len);
    if(!audio_pack.payload){
        log_info("failed to alloc mem for received data");
        return;
    }
    audio_pack.payload_len = m_traffic_state.downlink_audio_packet_len;

    while (1) {
        if(!check_if_session_in_call() || !m_traffic_state.udp_fd){
            break;
        }
        m_timeout_cnt++;
        if (m_timeout_cnt > 10){ // 200ms没有收到数据，flush缓存数据
            //log_info("traffic receive monitor: no data timeout, flush traffic data\n");

            os_mutex_pend(&mutex, 0);
            m_traffic_state.rx_seq = 0;
            if (m_cache_block) {
                for (int i = 0; i < REORDER_WINDOW; i++) {
                    if (m_cache_block[i].filled && m_cache_block[i].seq > m_traffic_state.rx_seq) {
                        /* 将已解密数据放到 audio_pack.payload，然后交付 */
                        memcpy(audio_pack.payload, m_cache_block[i].payload, m_traffic_state.downlink_audio_packet_len);
                        dialog_audio_dec_frame_write(&audio_pack);
                        m_traffic_state.rx_seq = m_cache_block[i].seq;
                        m_cache_block[i].filled = 0;
                    }        
                }
            }
            m_timeout_cnt = 0;
            os_mutex_post(&mutex);
        }
        os_time_dly(2);
    }

    free(audio_pack.payload);
    audio_pack.payload = NULL;
}



void traffic_init(void *priv){
    
    if (os_mutex_create(&mutex) != OS_NO_ERR) {
        log_error("os_mutex_create mutex fail\n");
        return;
    }

}

late_initcall(traffic_init);