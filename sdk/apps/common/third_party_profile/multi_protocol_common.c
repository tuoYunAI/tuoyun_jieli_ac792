#include "multi_protocol_main.h"
#include "classic/tws_api.h"
#include "app_config.h"
#if TCFG_USER_TWS_ENABLE
#include "app_msg.h"
#include "bt_tws.h"
#endif


#if THIRD_PARTY_PROTOCOLS_SEL || (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN))


#define LOG_TAG     		"[MULT_PROTOCOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"


#if TCFG_USER_TWS_ENABLE

/*******************************************************
               BLE/SPP 中间层句柄 TWS 同步
*******************************************************/

#define TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC \
	(((u8)('M' + 'U' + 'L' + 'T' + 'I') << (3 * 8)) | \
	 ((u8)('P' + 'R' + 'O' + 'T' + 'O') << (2 * 8)) | \
	 ((u8)('C' + 'O' + 'L') << (1 * 8)) | \
	 ((u8)('T' + 'W' + 'S') << (0 * 8)))

#define MULTI_PROTOCOL_TWS_SYNC_BLE     (0x01)
#define MULTI_PROTOCOL_TWS_SYNC_SPP     (0x02)

extern const int config_tws_le_role_sw;    //是否支持ble跟随tws主从切换

void multi_protocol_tws_sync_send(void)
{
    u8 *temp_buf = 0;
    int size = 0;

    log_info("tws sync : %d %d", get_bt_tws_connect_status(), tws_api_get_role());

    if (!get_bt_tws_connect_status() || !(tws_api_get_role() == TWS_ROLE_MASTER)) {
        return;
    }

    if (config_tws_le_role_sw) {
        size = app_ble_all_sync_data_size();
        if (size) {
            temp_buf = zalloc(size + 1);
            temp_buf[0] = MULTI_PROTOCOL_TWS_SYNC_BLE;
            app_ble_all_sync_data_get(&temp_buf[1]);
            tws_api_send_data_to_sibling(temp_buf, size + 1, TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC);
            free(temp_buf);
        }
    }

    size = app_spp_all_sync_data_size();
    if (size) {
        temp_buf = zalloc(size + 1);
        temp_buf[0] = MULTI_PROTOCOL_TWS_SYNC_SPP;
        app_spp_all_sync_data_get(&temp_buf[1]);
        tws_api_send_data_to_sibling(temp_buf, size + 1, TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC);
        free(temp_buf);
    }
}

static void multi_protocol_tws_sync_in_task(u8 *data, int len)
{
    log_info("multi_protocol_tws_sync_in_task %d", len);
    log_info_hexdump(data, len);

    switch (data[0]) {
    case MULTI_PROTOCOL_TWS_SYNC_BLE:
        app_ble_all_sync_data_set(&data[1], len - 1);
        break;
    case MULTI_PROTOCOL_TWS_SYNC_SPP:
        app_spp_all_sync_data_set(&data[1], len - 1);
        break;
    }

    free(data);
}

static void multi_protocol_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    int argv[4];
    u8 *rx_data = NULL;

    log_info("multi_protocol_tws_sync_in_irq %d", len);
    log_info_hexdump(_data, len);

    if (get_bt_tws_connect_status()) {
        if (rx && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
            rx_data = malloc(len);
            if (rx_data == NULL) {
                return;
            }
            memcpy(rx_data, _data, len);
            argv[0] = (int)multi_protocol_tws_sync_in_task;
            argv[1] = 2;
            argv[2] = (int)rx_data;
            argv[3] = (int)len;

            int ret = os_taskq_post_type("app_core", Q_CALLBACK, 4, argv);
            if (ret) {
                log_error("%s taskq post err", __func__);
            }
        }
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_bt_hdl_sync) = {
    .func_id = TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC,
    .func = multi_protocol_tws_sync_in_irq,
};

#endif

#endif

