#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_flow_player.data.bss")
#pragma data_seg(".dev_flow_player.data")
#pragma const_seg(".dev_flow_player.text.const")
#pragma code_seg(".dev_flow_player.text")
#endif

#include "os/os_api.h"
#include "app_config.h"
#include "fs/fs.h"
#include "sdk_config.h"
#include "app_event.h"
#include "app_audio.h"
#include "app_protocol.h"
#include "recorder/tuoyun_voice_recorder.h"
#include "player/tuoyun_flow_player.h"  

#define LOG_TAG             "[AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "system/debug.h"

int tuoyun_player_event_callback(enum stream_event event);

extern media_parameter_t g_audio_enc_media_param;

extern media_parameter_t g_audio_dec_media_param;

static m_audio_received_frame = 0;
static MAX_CACHED_FRAME = 1;
static device_working_status_t m_mic_status = WORKING_STATUS_STOP;
static device_working_status_t m_speaker_status = WORKING_STATUS_STOP;
static m_speaker_inited = 0;



static void audio_stop_speaker(bool wait_play_end){
    
    if (m_speaker_status == WORKING_STATUS_STOP){
        log_info("speaker status already muted");   
        return;
    }
    if (wait_play_end && m_speaker_inited){
        if (m_speaker_status == WORKING_STATUS_STOP_PENDING){
            log_info("speaker status already awaiting for stop");   
            return;
        }
        m_speaker_status = WORKING_STATUS_STOP_PENDING;
    }else{
        log_info("clear the speaker");
        m_speaker_status = WORKING_STATUS_STOP;
        tuoyun_audio_player_clear();
        m_speaker_inited = 0;
        m_audio_received_frame = 0;
    }
    
#if 0
    int received_frame = m_audio_received_frame;
    m_audio_received_frame = 0;
    if (wait_play_end && received_frame > 0){
        os_time_dly(received_frame > 10 ? 10 : received_frame); //等待音频播放完毕
    }

    log_info("clear the speaker");
    tuoyun_audio_player_clear();
    m_speaker_inited = 0;
 #endif   
}

static void audio_start_speaker(){

    if (m_speaker_status == WORKING_STATUS_START){
        log_info("speaker status already open, %d, %d", m_speaker_inited, m_audio_received_frame);
        return;
    }
    log_info("@@@@@ Please listen...");
    m_speaker_status = WORKING_STATUS_START;
    if (!m_speaker_inited && m_audio_received_frame >= MAX_CACHED_FRAME){
        app_event_t ev = {
            .event = APP_EVENT_AUDIO_SPEAK_BEFORE_OPEN,
            .arg = NULL
        };
        app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
        tuoyun_audio_player_start(tuoyun_player_event_callback);
        m_speaker_inited = 1;
    }
}

static int proc_enc_audio_data(void *file, void *buf, int len)
{
      if (m_mic_status != WORKING_STATUS_START) {
        //静音状态不发送数据
        return len;
      }
      extern int send_audio(audio_stream_packet_ptr packet);
      audio_stream_packet_t packet = {0};
      packet.payload_len = len - 8;
      packet.payload = ((u8*) buf) + 8;      
      send_audio(&packet);
      return len;
}



static void audio_start_mic(){
    if (m_mic_status == WORKING_STATUS_START){
        log_info("mic status already open");
        return;
    }    
    log_info("@@@@@ Please start speak...");
    tuoyun_voice_param_t param = {0};
    param.code_type = AUDIO_CODING_OPUS;
    param.frame_ms = g_audio_enc_media_param.frame_duration;
    param.output = proc_enc_audio_data;
    param.sample_rate = g_audio_enc_media_param.sample_rate;
    param.format_mode = 3; //对应RAW模式
    tuoyun_voice_recorder_open(&param);

    void set_start_audio_transmission_flag();
    set_start_audio_transmission_flag();
    log_info("mic status open, sample_rate: %d, frame_ms: %d", param.sample_rate, param.frame_ms);
    m_mic_status = WORKING_STATUS_START;

    /**
     * 通知服务器, 麦克风打开, 扬声器关闭
     */
    app_event_t event = {
        .event = APP_EVENT_AUDIO_PLAY_END_NOTIFY,
        .arg = NULL
    };

    app_event_notify(APP_EVENT_FROM_AUDIO, &event);
}

static void audio_stop_mic(){
    if (m_mic_status == WORKING_STATUS_STOP){
        log_info("mic status already muted");
        return;
    }

    tuoyun_voice_recorder_close();
    m_mic_status = WORKING_STATUS_STOP;
    log_info("mic status mute");
}



int dialog_audio_init(media_parameter_ptr downlink_media_param)
{
    if (downlink_media_param != NULL) {
        memcpy(&g_audio_dec_media_param, downlink_media_param, sizeof(media_parameter_t));
    } else {
        log_info("audio_open param err");
        return -1;
    }
    m_speaker_inited = 0;
    m_audio_received_frame = 0;
    return 0;
}


int dialog_audio_close(void)
{
    audio_stop_mic();
    audio_stop_speaker(false);
    return 0;
}



int dialog_proc_speak_status(device_working_status_t  status){
    
    if(status == WORKING_STATUS_START){
        audio_start_speaker();
        audio_stop_mic();
    }else if(status == WORKING_STATUS_STOP){
        audio_stop_speaker(true);
        if (m_speaker_inited == 0){
            audio_start_mic();
        }
    }else{
        log_info("invalid speak status: %d", status);
    }
}

int dialog_proc_interrupt_speak(void){
    log_info("dialog_proc_interrupt_speak");
    audio_stop_speaker(false);
    return 0;   
}

static int tuoyun_player_event_callback(enum stream_event event)
{
    switch (event) {
    case STREAM_EVENT_INIT:
        break;
    case STREAM_EVENT_START:
        break;
    case STREAM_EVENT_STOP:
        log_info("tuoyun_player_event_callback: stop speaker: %d", m_speaker_status);
        m_speaker_inited = 0;
        if (m_speaker_status == WORKING_STATUS_STOP_PENDING){
            app_event_t ev = {
                .event = APP_EVENT_AUDIO_SPEAK_BEFORE_CLOSE,
                .arg = NULL
            };
            app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
            
            m_speaker_status = WORKING_STATUS_STOP;
            tuoyun_audio_player_clear();
            audio_start_mic();
            m_audio_received_frame = 0;
            log_info("tuoyun_player_event_callback: switch to mic");
            ev.event = APP_EVENT_AUDIO_SPEAK_AFTER_CLOSE;
            app_event_notify(APP_EVENT_FROM_AUDIO, &ev);
        }
        break;
    case STREAM_EVENT_END:
        break;
    default:
        break;
    }

    return 0;
}


int dialog_audio_dec_frame_write(audio_stream_packet_ptr packet){
  
    //log_info("**********audio player write data len: %d", packet->payload_len);
    int len = tuoyun_audio_write_data(packet->payload, packet->payload_len);
    if (len != packet->payload_len){
        log_info("audio player write data len err: %d, %d", len, packet->payload_len);
        return -1;
    }
    m_audio_received_frame++;
    
    /**
     * 启动播放器时，必须确保缓存中有数据，否则会出现读不到数据超时导致播放失败的问题
     */
    if(!m_speaker_inited && m_speaker_status != WORKING_STATUS_STOP && m_audio_received_frame >= MAX_CACHED_FRAME){
        log_info("@TIMING@ 4 **********audio player start");
        m_speaker_inited = 1;
        tuoyun_audio_player_start(tuoyun_player_event_callback);
    }
    
    return 0;
}


device_working_status_t get_speaker_status(){
    return m_speaker_status;
}


