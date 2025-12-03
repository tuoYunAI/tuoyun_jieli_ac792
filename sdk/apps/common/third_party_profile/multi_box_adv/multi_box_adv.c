#include "multi_protocol_main.h"
#include "multi_box_adv.h"
#include "syscfg/syscfg_id.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & MULTI_BOX_ADV_EN)

#define LOG_TAG     		"[MULTI_BOX_ADV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_WARN_ENABLE
#include "system/debug.h"

static void *multi_box_adv_ble_hdl = NULL;

static u16 multi_box_adv_interval_min = 160;

static u8 multi_box_data;
static u8 multi_box_volume_data;

void set_multi_box_adv_volume_data(u8 volume)
{
    multi_box_volume_data = volume;
}

void set_multi_box_adv_data(u8 adv_data)
{
    multi_box_data = adv_data;
}

void set_multi_box_adv_interval_min(u16 adv_interval)
{
    multi_box_adv_interval_min = adv_interval * 8 / 5;
}

u8 get_multi_box_adv_data(void)
{
    return multi_box_data;
}

static u8 multi_box_fill_adv_data(u8 *adv_data)
{
    char ble_name[ADV_RSP_PACKET_MAX - 2 - 3];
    u8 offset = 0;
    u8 edr_addr[6];
    if (syscfg_read(CFG_BT_MAC_ADDR, edr_addr, 6) < 0) {
        memcpy(edr_addr, (void *)bt_get_mac_addr(), 6);
    }
    int name_len = snprintf(ble_name, sizeof(ble_name), MULTI_BOX_ADV_PREFIX"%02X%02X", edr_addr[1], edr_addr[0]);
    u8 manufacturer_specific_data[sizeof(company_id) + 2];
    memcpy(manufacturer_specific_data, company_id, sizeof(company_id));
    manufacturer_specific_data[sizeof(company_id)] = multi_box_data;
    manufacturer_specific_data[sizeof(company_id) + 1] = multi_box_volume_data;

    offset += make_eir_packet_val(&adv_data[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS, (u8 *)service_32bit_uuid, sizeof(service_32bit_uuid));
    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("adv_data overflow!!!!!!");
        return 0;
    }
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (u8 *)ble_name, name_len);
    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("adv_data overflow!!!!!!");
        return 0;
    }
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, manufacturer_specific_data, sizeof(manufacturer_specific_data));
    if (offset > ADV_RSP_PACKET_MAX) {
        log_error("adv_data overflow!!!!!!");
        return 0;
    }
    return offset;
}

static u8 multi_box_fill_rsp_data(u8 *rsp_data)
{
    return 0;
}

int multi_box_adv_adv_enable(u8 enable)
{
    uint8_t adv_type = ADV_NONCONN_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    uint8_t advData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t rspData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t len = 0;

    if (enable == app_ble_adv_state_get(multi_box_adv_ble_hdl)) {
        return 0;
    }

    if (enable) {
        app_ble_set_adv_param(multi_box_adv_ble_hdl, multi_box_adv_interval_min, adv_type, adv_channel);
        len = multi_box_fill_adv_data(advData);
        if (len) {
            app_ble_adv_data_set(multi_box_adv_ble_hdl, advData, len);
        }
        len = multi_box_fill_rsp_data(rspData);
        if (len) {
            app_ble_rsp_data_set(multi_box_adv_ble_hdl, rspData, len);
        }
    }
    app_ble_adv_enable(multi_box_adv_ble_hdl, enable);
    return 0;
}

void multi_box_adv_all_init(void)
{
    log_info("multi_box_adv_all_init");

    u8 mac_addr[6];
    u8 edr_addr[6];

    if (multi_box_adv_ble_hdl == NULL) {
        multi_box_adv_ble_hdl = app_ble_hdl_alloc();
        if (multi_box_adv_ble_hdl == NULL) {
            log_error("multi_box_adv_ble_hdl alloc err !");
            return;
        }

        if (syscfg_read(CFG_BT_MAC_ADDR, edr_addr, 6) < 0) {
            memcpy(edr_addr, (void *)bt_get_mac_addr(), 6);
        }

        for (u8 i = 0; i < 6; ++i) {
            mac_addr[i] = edr_addr[5 - i];
        }

        app_ble_set_mac_addr(multi_box_adv_ble_hdl, (void *)mac_addr);

        multi_box_adv_adv_enable(1);
    }
}

void multi_box_adv_all_exit(void)
{
    log_info("multi_box_adv_all_exit");

    // BLE exit
    if (multi_box_adv_ble_hdl) {
        multi_box_adv_adv_enable(0);

        if (app_ble_get_hdl_con_handle(multi_box_adv_ble_hdl)) {
            app_ble_disconnect(multi_box_adv_ble_hdl);
        }
        app_ble_hdl_free(multi_box_adv_ble_hdl);
        multi_box_adv_ble_hdl = NULL;
    }
}

#endif

