
#ifndef __AUDIO__
#define __AUDIO__

#include "os/os_api.h"
#include "app_protocol.h"

/**
 * Audio module event
 */
typedef struct{
    int event;
    void *arg;
}audio_event_t, *audio_event_ptr;

/**
 * @brief  Initialize network audio module. Call this function when creating a session.
 * @param  downlink_media_param: Pointer to Media parameter
 * @return 0: Initialization successful
 *       Other: Initialization failed
 * @note
 */
int dialog_audio_init(media_parameter_ptr downlink_media_param);

/**
 * @brief  Close network audio tunnel. Call this function when the session ends.
 * @return 0: Close successful
 *       Other: Close failed
 * @note 
 */
int dialog_audio_close(void);

/**
 * @brief Receive and Process speaker status change notification from server
 * @param  status: Speaker working status
 * @return 0: Processing successful
 *       Other: Processing failed
 * @note 
 */
int dialog_proc_speak_status(device_working_status_t  status);

/**
 * @brief  Write decrypted audio frame data
 * @param  packet: Pointer to audio data packet structure
 * @return 0: Write successful
 *       Other: Write failed
 * @note 
 */
int dialog_audio_dec_frame_write(audio_stream_packet_ptr packet);

/**
 * @brief  Turn on wake word detection
 *         Wake word detection and recording module cannot work at the same time
 * @return void
 * @note 
 */
void tuoyun_asr_recorder_open();

/**
 * Turn off wake word detection
 */
int tuoyun_asr_recorder_close();

#endif