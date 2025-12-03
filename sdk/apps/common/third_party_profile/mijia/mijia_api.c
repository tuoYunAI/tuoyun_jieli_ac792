#include "system/includes.h"
#include "app_config.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & MIJIA_EN)

#include "custom_mi_config.h"
#include "mi_config.h"
#include "app_ble_spp_api.h"
#include "mible_api.h"
#include "mible_mcu.h"
#include "syscfg/syscfg_id.h"
#include "user_cfg_id.h"


extern void *mijia_ble_hdl;
const u8 *get_mijia_ble_addr(void);

/**
 *        GAP APIs
 */

/**
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note:   You should copy gap mac to mac[6]
 * */

mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    memcpy(mac, (void *)get_mijia_ble_addr(), 6);
    return MI_SUCCESS;
}

/**
 * @brief   Start scanning
 * @param   [in] scan_type: passive or active scaning
 *          [in] scan_param: scan parameters including interval, windows
 * and timeout
 * @return  MI_SUCCESS             Successfully initiated scanning procedure.
 *          MI_ERR_INVALID_STATE   Has initiated scanning procedure.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending
 * events and retry.
 * @note    Other default scanning parameters : public address, no
 * whitelist.
 *          The scan response is given through
 * MIBLE_GAP_EVT_ADV_REPORT event
 */
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Get scan param
 * @param   [out] scan_param: scan interval and window
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_param_get(mible_gap_scan_param_t *scan_param)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Start advertising
 * @param   [in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note    Other default advertising parameters: local public address , no
 * filter policy
 * */
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
#endif
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    u16 adv_interval_min = p_param->adv_interval_min;

    switch (p_param->adv_type) {
    case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
        break;
    case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
        break;
    case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
        break;
    }
    app_ble_adv_enable(mijia_ble_hdl, 0);
    app_ble_set_adv_param(mijia_ble_hdl, adv_interval_min, adv_type, 0x7);
    app_ble_adv_enable(mijia_ble_hdl, 1);
    return MI_SUCCESS;
}

/**
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */
mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("adv:");
    put_buf(p_data, dlen);
    printf("rsp:");
    put_buf(p_sr_data, srdlen);
#endif
    app_ble_adv_enable(mijia_ble_hdl, 0);
    app_ble_adv_data_set(mijia_ble_hdl, (u8 *)p_data, dlen);
    app_ble_rsp_data_set(mijia_ble_hdl, (u8 *)p_sr_data, srdlen);
    app_ble_adv_enable(mijia_ble_hdl, 1);
    return MI_SUCCESS;
}

/**
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
#endif
    app_ble_adv_enable(mijia_ble_hdl, 0);
    return MI_SUCCESS;
}

/**
 * @brief   Create a Direct connection
 * @param   [in] scan_param : scanning parameters, see TYPE
 * mible_gap_scan_param_t for details.
 *          [in] conn_param : connection parameters, see TYPE
 * mible_gap_connect_t for details.
 * @return  MI_SUCCESS             Successfully initiated connection procedure.
 *          MI_ERR_INVALID_STATE   Initiated connection procedure in connected state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again
 *          MIBLE_ERR_GAP_INVALID_BLE_ADDR    Invalid Bluetooth address
 * supplied.
 * @note    Own and peer address are both public.
 *          The connection result is given by MIBLE_GAP_EVT_CONNECTED
 * event
 * */
mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
                                 mible_gap_connect_t conn_param)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Disconnect from peer
 * @param   [in] conn_handle: the connection handle
 * @return  MI_SUCCESS             Successfully disconnected.
 *          MI_ERR_INVALID_STATE   Not in connnection.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and periphral
 * role.
 * */
mible_status_t mible_gap_disconnect(uint16_t conn_handle)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Update the connection parameters.
 * @param   [in] conn_handle: the connection handle.
 *          [in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and peripheral
 *role.
 * */
mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
        mible_gap_conn_param_t conn_params)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 *        GATT Server APIs
 */

/**
 * @brief   Add a Service to a GATT server
 * @param   [in|out] p_server_db: pointer to mible service data type
 * of mible_gatts_db_t, see TYPE mible_gatts_db_t for details.
 * @return  MI_SUCCESS             Successfully added a service declaration.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_NO_MEM          Not enough memory to complete operation.
 * @note    This function can be implemented asynchronous. When service inition complete, call mible_arch_event_callback function and pass in MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP event and result.
 * */
mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
#endif

    int i, j;
    for (i = 0; i < p_server_db->srv_num; i++) {
        printf("srv[%d]:", i);
        printf("type:0x%x handle:0x%x char_num:%d\n", \
               p_server_db->p_srv_db[i].srv_type, \
               p_server_db->p_srv_db[i].srv_handle, \
               p_server_db->p_srv_db[i].char_num \
              );
        if (p_server_db->p_srv_db[i].srv_uuid.type) {
            printf("uuid128");
            put_buf(p_server_db->p_srv_db[i].srv_uuid.uuid128, 16);
            p_server_db->p_srv_db[i].srv_handle = 0x0c;
        } else {
            printf("uuid16:0x%x", p_server_db->p_srv_db[i].srv_uuid.uuid16);
            p_server_db->p_srv_db[i].srv_handle = 0x01;
        }

        for (j = 0; j < p_server_db->p_srv_db[i].char_num; j++) {
            printf("char[%d]", \
                   j \
                  );
            if (p_server_db->p_srv_db[i].p_char_db[j].char_uuid.type) {
                printf("uuid128");
                put_buf(p_server_db->p_srv_db[i].p_char_db[j].char_uuid.uuid128, 16);
                if (p_server_db->p_srv_db[i].p_char_db[j].char_uuid.uuid128[12] == 0x01) {
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0013;
                } else {
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0020;
                }
            } else {
                printf("uuid16:0x%x", p_server_db->p_srv_db[i].p_char_db[j].char_uuid.uuid16);
                switch (p_server_db->p_srv_db[i].p_char_db[j].char_uuid.uuid16) {
                case 0x0004:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0003;
                    break;
                case 0x0005:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0005;
                    break;
                case 0x0010:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0008;
                    break;
                case 0x0019:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x000b;
                    break;
                case 0x0017:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x000e;
                    break;
                case 0x0018:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0011;
                    break;
                case 0x001a:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0014;
                    break;
                case 0x001b:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x0017;
                    break;
                case 0x001c:
                    p_server_db->p_srv_db[i].p_char_db[j].char_value_handle = 0x001a;
                    break;
                }
            }
        }
    }

    // server init complete callback
    mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.srv_init_cmp.status = MI_SUCCESS;
    param.srv_init_cmp.p_gatts_db = p_server_db;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
    return MI_SUCCESS;
}

/**
 * @brief   Set characteristic value
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [in] offset: the offset from which the attribute value has
 *to be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully retrieved the value of the
 *attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */

#define MAX_GATTS_VALUE_NUM     5
struct {
    uint16_t srv_handle;
    uint16_t value_handle;
    uint8_t offset;
    uint8_t *p_value;
    uint8_t len;
} gatts_value_array[MAX_GATTS_VALUE_NUM] = {0};

mible_status_t mible_gatts_value_set(uint16_t srv_handle,
                                     uint16_t value_handle, uint8_t offset, uint8_t *p_value, uint8_t len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("srv:%x, v_hdl:%x, offset:%d", srv_handle, value_handle, offset);
    put_buf(p_value, len);
#endif

    int i;
    for (i = 0; i < MAX_GATTS_VALUE_NUM; i++) {
        if (gatts_value_array[i].value_handle == value_handle) {
            if (gatts_value_array[i].p_value) {
                free(gatts_value_array[i].p_value);
            }
            gatts_value_array[i].p_value = zalloc(len);
            memcpy(gatts_value_array[i].p_value, p_value, len);
            gatts_value_array[i].len = len;
            return MI_SUCCESS;
        }
    }
    for (i = 0; i < MAX_GATTS_VALUE_NUM; i++) {
        if (gatts_value_array[i].value_handle == 0) {
            if (gatts_value_array[i].p_value) {
                printf("errrrr !");
                return MI_ERR_NOT_FOUND;
            }
            gatts_value_array[i].value_handle = value_handle;
            gatts_value_array[i].p_value = zalloc(len);
            memcpy(gatts_value_array[i].p_value, p_value, len);
            gatts_value_array[i].len = len;
            return MI_SUCCESS;
        }
    }

    printf("%s no idle array !", __FUNCTION__);

    return MI_SUCCESS;
}

/**
 * @brief   Get charicteristic value as a GATTS.
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [out] p_value: pointer to data which stores characteristic value
 *          [out] p_len: pointer to data length.
 * @return  MI_SUCCESS             Successfully get the value of the attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 **/
mible_status_t mible_gatts_value_get(uint16_t srv_handle,
                                     uint16_t value_handle, uint8_t *p_value, uint8_t *p_len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("srv:%x, v_hdl:%x", srv_handle, value_handle);
    printf("p_value:%x", p_value);
#endif

    int i;
    for (i = 0; i < MAX_GATTS_VALUE_NUM; i++) {
        if (gatts_value_array[i].value_handle == value_handle) {
            if (gatts_value_array[i].p_value == NULL) {
                printf("errrrr !");
                return MI_ERR_NOT_FOUND;
            }
            if (p_value) {
                memcpy(p_value, gatts_value_array[i].p_value, gatts_value_array[i].len);
            }
            *p_len = gatts_value_array[i].len;
            return MI_SUCCESS;
        }
    }

    printf("%s not found", __FUNCTION__);

    return MI_ERR_NOT_FOUND;
}

/**
 * @brief   Set characteristic value and notify it to client.
 * @param   [in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 *          [in] char_value_handle: characteristic  value handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
 *          [in] type : notification = 1; indication = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a notification or
 * indication for transmission,
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or notifications
 * and/or indications not enabled in the CCCD.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE   //Attributes are not modifiable by
 * the application.
 * @note    This function checks for the relevant Client Characteristic
 * Configuration descriptor value to verify that the relevant operation (notification or
 * indication) has been enabled by the client.
 * */
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle,
        uint16_t srv_handle, uint16_t char_value_handle, uint8_t offset,
        uint8_t *p_value, uint8_t len, uint8_t type)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("con_handle:%x srv_hdl:%x val_hdl:%x type:%x len:%d", conn_handle, srv_handle, char_value_handle, type, len);
    put_buf(p_value, len);
#endif
    int ret = app_ble_att_send_data(mijia_ble_hdl, char_value_handle, p_value, len, ATT_OP_AUTO_READ_CCC);
    if (ret != BLE_CMD_RET_SUCESS) {
        printf("%s err : %d\n", __FUNCTION__, ret);
    }
    return MI_SUCCESS;
}

/**
 * @brief   Respond to a Read/Write user authorization request.
 * @param   [in] conn_handle: conn handle
 *          [in] status:  1: permit to change value ; 0: reject to change value
 *          [in] char_value_handle: characteristic handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: Pointer to new value used to update the attribute value.
 *          [in] len: data length
 *          [in] type : read response = 1; write response = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a response to the peer, and in the case of a write operation, GATT updated.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or no authorization request pending.
 *          MI_ERR_INVALID_LENGTH  Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 * @note    This call should only be used as a response to a MIBLE_GATTS_EVT_READ/WRITE_PERMIT_REQ
 * event issued to the application.
 * */
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t *p_value, uint8_t len, uint8_t type)
{
#ifdef MIBLE_DEBUG_LOG
    printf("hb to do %s %d!!!", __func__, __LINE__);
    printf("conn_handle:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", conn_handle, status, char_value_handle, offset, p_value, len, type);
    put_buf(p_value, len);
#endif
    /* int ret = app_ble_att_send_data(mijia_ble_hdl, char_value_handle, p_value, len, ATT_OP_AUTO_READ_CCC); */
    int ret = 0;
    if (ret != BLE_CMD_RET_SUCESS) {
        printf("%s err : %d\n", __FUNCTION__, ret);
    }
    return MI_SUCCESS;
}

/**
 *        GATT Client APIs
 */

/**
 * @brief   Discover primary service by service UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for primary sevice
 *discovery procedure
 *          [in] p_srv_uuid: pointer to service uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    The response is given through
 *MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP event
 * */
mible_status_t mible_gattc_primary_service_discover_by_uuid(
    uint16_t conn_handle, mible_handle_range_t handle_range,
    mible_uuid_t *p_srv_uuid)
{

    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Discover characteristic by characteristic UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for characteristic discovery
 * procedure
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the
 * Characteristic Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_CHR_DISCOVER_BY_UUID_RESP event
 * */
mible_status_t mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
        mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Discover characteristic client configuration descriptor
 * @param   [in] conn_handle: connection handle
 *          [in] handle_range: search range
 * @return  MI_SUCCESS             Successfully started Clien Config Descriptor
 * Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    Maybe run the charicteristic descriptor discover procedure firstly,
 * then pick up the client configuration descriptor which att type is 0x2092
 *          The response is given through MIBLE_GATTC_CCCD_DISCOVER_RESP
 * event
 *          Only return the first cccd handle within the specified
 * range.
 * */
mible_status_t mible_gattc_clt_cfg_descriptor_discover(
    uint16_t conn_handle, mible_handle_range_t handle_range)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Read characteristic value by UUID
 * @param   [in] conn_handle: connnection handle
 *          [in] handle_range: search range
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Read
 * using Characteristic UUID procedure.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_EVT_READ_CHR_VALUE_BY_UUID_RESP event
 * */
mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
        mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Write value by handle with response
 * @param   [in] conn_handle: connection handle
 *          [in] handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write with response
 * procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through MIBLE_GATTC_EVT_WRITE_RESP event
 *
 * */
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle,
        uint16_t att_handle, uint8_t *p_value, uint8_t len)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Write value by handle without response
 * @param   [in] conn_handle: connection handle
 *          [in] att_handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write Cmd procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    no response
 * */
mible_status_t mible_gattc_write_cmd(uint16_t conn_handle,
                                     uint16_t att_handle, uint8_t *p_value, uint8_t len)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 *        SOFT TIMER APIs
 */

/**
 * @brief   Create a timer.
 * @param   [out] p_timer_id: a pointer to timer id address which can uniquely identify the timer.
 *          [in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 *          [in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */

struct mible_timer_hdl {
    mible_timer_handler timeout_handler;
    mible_timer_mode mode;
    u16 timer_hdl;
};

mible_status_t mible_timer_create(void **p_timer_id,
                                  mible_timer_handler timeout_handler, mible_timer_mode mode)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
#endif
    struct mible_timer_hdl *hdl = zalloc(sizeof(struct mible_timer_hdl));
    hdl->timeout_handler = timeout_handler;
    hdl->mode = mode;
    hdl->timer_hdl = 0;
    *p_timer_id = hdl;
#ifdef MIBLE_DEBUG_LOG
    printf("mible_timer_create hdl:%x handler:%x mode:%x", hdl, timeout_handler, mode);
#endif
    return MI_SUCCESS;
}

/**
 * @brief   Create a timer.
 * @param   [out] p_timer_id: a pointer to timer id address which can uniquely identify the timer.
 *          [in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 *          [in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */
mible_status_t mible_user_timer_create(void **p_timer_id,
                                       mible_timer_handler timeout_handler, mible_timer_mode mode)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
#endif
    struct mible_timer_hdl *hdl = zalloc(sizeof(struct mible_timer_hdl));
    hdl->timeout_handler = timeout_handler;
    hdl->mode = mode;
    hdl->timer_hdl = 0;
    *p_timer_id = hdl;
#ifdef MIBLE_DEBUG_LOG
    printf("mible_timer_create hdl:%x handler:%x mode:%x", hdl, timeout_handler, mode);
#endif
    return MI_SUCCESS;
}

/**
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void *timer_id)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("hdl:%x", timer_id);
#endif

    struct mible_timer_hdl *hdl = timer_id;
    if (hdl->mode == MIBLE_TIMER_REPEATED) {
        if (hdl->timer_hdl != 0) {
            sys_timer_del(hdl->timer_hdl);
            hdl->timer_hdl = 0;
        }
    } else {
        printf("timer mode error !");
    }
    free(hdl);
    return MI_SUCCESS;
}

/**
 * @brief   Start a timer.
 * @param   [in] timer_id: timer id
 *          [in] timeout_value: Number of milliseconds to time-out event
 * (minimum 10 ms).
 *          [in] p_context: parameters that can be passed to
 * timeout_handler
 *
 * @return  MI_SUCCESS             If the timer was successfully started.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   If the application timer module has not been
 * initialized or the timer has not been created.
 *          MI_ERR_NO_MEM          If the timer operations queue was full.
 * @note    If the timer has already started, it will start counting again.
 * */

//typedef void (*mible_timer_handler)(void*);
mible_status_t mible_timer_start(void *timer_id, uint32_t timeout_value,
                                 void *p_context)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("hdl:%x time:%d", timer_id, timeout_value);
#endif

    struct mible_timer_hdl *hdl = timer_id;
    if (hdl->timer_hdl == 0) {
        if (hdl->mode == MIBLE_TIMER_SINGLE_SHOT) {
            sys_timeout_add(p_context, hdl->timeout_handler, timeout_value);
        } else if (hdl->mode == MIBLE_TIMER_REPEATED) {
            hdl->timer_hdl = sys_timer_add(p_context, hdl->timeout_handler, timeout_value);
        } else {
            printf("timer mode error !");
        }
    }

    return MI_SUCCESS;
}

/**
 * @brief   Stop a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void *timer_id)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d timer_id:0x%x!!!", __func__, __LINE__, timer_id);
#endif

    struct mible_timer_hdl *hdl = timer_id;
    if (hdl->timer_hdl == 0) {
        //printf("timer hdl error !");
        return MI_SUCCESS;
    }
    if (hdl->mode == MIBLE_TIMER_REPEATED) {
        sys_timer_del(hdl->timer_hdl);
        hdl->timer_hdl = 0;
    }
    return MI_SUCCESS;
}

/**
 *        NVM APIs
 */

/**
 * @brief   Create a record in flash
 * @param   [in] record_id: identify a record in flash
 *          [in] len: record length
 * @return  MI_SUCCESS              Create successfully.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_NO_MEM,          Not enough flash memory to be assigned
 *
 * */

//#define RECORD_DEV_ID       0x01
//#define RECORD_KEY_INFO     0x10

#define MAX_MIJIA_RECORD_NUM    10
struct {
    uint16_t record_id;
    uint8_t *data;
    uint8_t len;
} mijia_record_array[MAX_MIJIA_RECORD_NUM] = {0};

mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d id:%x len:%d !!!", __func__, __LINE__, record_id, len);
#endif

#if 1
    u16 cfgid = 0;

    switch (record_id) {
    case 0x01:
        cfgid = CFG_MIJIA_RECORD_ID_01;     // len 20
        break;
    case 0x04:
        cfgid = CFG_MIJIA_RECORD_ID_04;     // len 4
        break;
    case 0x10:
        cfgid = CFG_MIJIA_RECORD_ID_10;     // len 28
        break;
    case 0x05:
        cfgid = CFG_MIJIA_RECORD_ID_05;     // len 16
        break;
    default:
        printf("unknow id: %d\n", record_id);
        ASSERT(0);
        return MI_ERR_NO_MEM;
        break;
    }

    return MI_SUCCESS;
#else
    int i;
    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == record_id) {
            printf("is already exist");
            return MI_SUCCESS;
        }
    }
    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == 0) {
            mijia_record_array[i].record_id = record_id;
            printf("suss\n");
            return MI_SUCCESS;
        }
    }
    printf("error\n");
    return MI_ERR_NO_MEM;
#endif
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash
 * @return  MI_SUCCESS              Delete successfully.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d id:%x !!!", __func__, __LINE__, record_id);
#endif

#if 1
    int ret;
    u16 cfgid = 0;

    switch (record_id) {
    case 0x01:
        cfgid = CFG_MIJIA_RECORD_ID_01;     // len 20
        break;
    case 0x04:
        cfgid = CFG_MIJIA_RECORD_ID_04;     // len 4
        break;
    case 0x10:
        cfgid = CFG_MIJIA_RECORD_ID_10;     // len 28
        break;
    case 0x05:
        cfgid = CFG_MIJIA_RECORD_ID_05;     // len 16
        break;
    default:
        printf("unknow id: %d\n", record_id);
        ASSERT(0);
        return MI_ERR_NO_MEM;
        break;
    }

    mible_arch_evt_param_t param;

    int vm_erase_the_index(u8 hdl);
    ret = vm_erase_the_index(cfgid);
    if (ret == VM_WRITE_OVERFLOW) {
        param.record.id = record_id;
        param.record.status = MI_ERR_INVALID_PARAM;
        mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);
        return MI_ERR_INVALID_PARAM;
    }
    if (ret == VM_DATA_LEN_ERR) {
        u8 buf[32] = {0};
        syscfg_write(cfgid, buf, sizeof(buf));
    }

    param.record.id = record_id;
    param.record.status = MI_SUCCESS;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);
    return MI_SUCCESS;

#else
    mible_arch_evt_param_t param;
    int i;
    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == record_id) {
            if (mijia_record_array[i].data) {
                free(mijia_record_array[i].data);
                mijia_record_array[i].data = NULL;
                mijia_record_array[i].len = 0;
            }
            printf("suss\n");
            param.record.id = record_id;
            param.record.status = MI_SUCCESS;
            mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);
            return MI_SUCCESS;
        }
    }
    printf("not found\n");
    param.record.id = record_id;
    param.record.status = MI_ERR_INVALID_PARAM;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
    return MI_ERR_INVALID_PARAM;
#endif
}

/**
 * @brief   Restore data to flash
 * @param   [in] record_id: identify an area in flash
 *          [out] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t *p_data,
                                 uint8_t len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d id:%x len:%d !!!", __func__, __LINE__, record_id, len);
#endif

#if 1
    int ret;
    u16 cfgid = 0;
    switch (record_id) {
    case 0x01:
        cfgid = CFG_MIJIA_RECORD_ID_01;     // len 20
        break;
    case 0x04:
        cfgid = CFG_MIJIA_RECORD_ID_04;     // len 4
        break;
    case 0x10:
        cfgid = CFG_MIJIA_RECORD_ID_10;     // len 28
        break;
    case 0x05:
        cfgid = CFG_MIJIA_RECORD_ID_05;     // len 16
        break;
    default:
        printf("unknow id: %d\n", record_id);
        ASSERT(0);
        return MI_ERR_NO_MEM;
        break;
    }

    ret = syscfg_read(cfgid, p_data, len);
    if (ret != len) {
        printf("mi ble vm read %d error error ret = %d\n", cfgid, ret);
        return MI_ERR_INVALID_PARAM;
    }

    return MI_SUCCESS;

#else
    int i;
    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == record_id) {
            if (mijia_record_array[i].data) {
                memcpy(p_data, mijia_record_array[i].data, mijia_record_array[i].len);
                printf("suss\n");
                return MI_SUCCESS;
            }
        }
    }
    printf("not found\n");
    return MI_ERR_INVALID_PARAM;
#endif
}

/**
 * @brief   Store data to flash
 * @param   [in] record_id: identify an area in flash
 *          [in] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAMS   p_data is not aligned to a 4 byte boundary.
 * @note    Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 *          When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result.
 * */
mible_status_t mible_record_write(uint16_t record_id, const uint8_t *p_data,
                                  uint8_t len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d id:%x len:%d !!!", __func__, __LINE__, record_id, len);
    put_buf(p_data, len);
#endif

#if 1
    int ret;
    u16 cfgid = 0;
    switch (record_id) {
    case 0x01:
        cfgid = CFG_MIJIA_RECORD_ID_01;     // len 20
        break;
    case 0x04:
        cfgid = CFG_MIJIA_RECORD_ID_04;     // len 4
        break;
    case 0x10:
        cfgid = CFG_MIJIA_RECORD_ID_10;     // len 28
        break;
    case 0x05:
        cfgid = CFG_MIJIA_RECORD_ID_05;     // len 16
        break;
    default:
        printf("unknow id: %d\n", record_id);
        ASSERT(0);
        return MI_ERR_NO_MEM;
        break;
    }

    mible_arch_evt_param_t param;

    ret = syscfg_write(cfgid, (void *)p_data, len);
    if (ret < 0) {
        printf("mi ble vm %d write error ret = %d\n", cfgid, ret);
        param.record.id = record_id;
        param.record.status = MI_ERR_INVALID_PARAM;
        mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
        return MI_ERR_NO_MEM;
    }

    param.record.id = record_id;
    param.record.status = MI_SUCCESS;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);

    return MI_SUCCESS;
#else
    int i;
    mible_arch_evt_param_t param;

try_again:

    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == record_id) {
            if (mijia_record_array[i].data) {
                free(mijia_record_array[i].data);
            }
            mijia_record_array[i].data = zalloc(len);
            memcpy(mijia_record_array[i].data, p_data, len);
            mijia_record_array[i].len = len;
            printf("suss\n");
            param.record.id = record_id;
            param.record.status = MI_SUCCESS;
            mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
            return MI_SUCCESS;
        }
    }

    // not found create
    for (i = 0; i < MAX_MIJIA_RECORD_NUM; i++) {
        if (mijia_record_array[i].record_id == 0) {
            mijia_record_array[i].record_id = record_id;
            printf("create suss\n");
            goto try_again;
        }
    }

    printf("write fail !\n");
    param.record.id = record_id;
    param.record.status =  MI_ERR_INVALID_PARAM;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
    return MI_ERR_INVALID_PARAM;
#endif
}

/**
 *        MISC APIs
 */

/**
 * @brief   Get ture random bytes .
 * @param   [out] p_buf: pointer to data
 *          [in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS          The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note    SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generator(uint8_t *p_buf, uint8_t len)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("p_buf:%x len:%d", p_buf, len);
#endif

    uint8_t wlen = len;
    while (wlen--) {
        *p_buf++ = rand32() & 0xFF;
    }
    put_buf(p_buf, len);
    return MI_SUCCESS;
}

/**
 * @brief   Encrypts a block according to the specified parameters. 128-bit
 * AES encryption. (zero padding)
 * @param   [in] key: encryption key
 *          [in] plaintext: pointer to plain text
 *          [in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note    SHOULD use synchronous mode to implement this function
 * */
#include "crypto_toolbox/rijndael.h"
mible_status_t mible_aes128_encrypt(const uint8_t *key,
                                    const uint8_t *plaintext, uint8_t plen, uint8_t *ciphertext)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d!!!", __func__, __LINE__);
    printf("len:%d", plen);
#endif

    uint32_t rk[RKLENGTH(KEYBITS)];
    int nrounds = rijndaelSetupEncrypt(rk, &key[0], KEYBITS);
    rijndaelEncrypt(rk, nrounds, plaintext, ciphertext);

    return MI_SUCCESS;
}

/**
 * @brief   Post a task to a task quene, which can be executed in a right place
 * (maybe a task in RTOS or while(1) in the main function).
 * @param   [in] handler: a pointer to function
 *          [in] param: function parameters
 * @return  MI_SUCCESS              Successfully put the handler to quene.
 *          MI_ERR_NO_MEM           The task quene is full.
 *          MI_ERR_INVALID_PARAM    Handler is NULL
 * */

#define TEST_HANDLER_NUM    50
static void *test_handler[TEST_HANDLER_NUM] = {0};
static void *test_handler_arg[TEST_HANDLER_NUM] = {0};

mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
#ifdef MIBLE_DEBUG_LOG
    printf("mijia %s %d handler:0x%x arg:0x%p!!!", __func__, __LINE__, handler, arg);
#endif
    handler(arg);
    return MI_SUCCESS;

#if 0
    int i;
    for (i = 0; i < TEST_HANDLER_NUM; i++) {
        if (test_handler[i] == NULL) {
            test_handler[i] = handler;
            test_handler_arg[i] = arg;
        }
    }

#endif

#if 0
    int msg[3];
    msg[0] = (int)handler;
    msg[1] = 1;
    msg[2] = (int)arg;
    int err = os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("%s post fail\n", __func__);
    }
    return MI_SUCCESS;
#endif
}

/**
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will
 * execute all events scheduled since the last time it was called.
 * */
void mible_tasks_exec(void)
{
#if 0
    int i;
    mible_handler_t handler = NULL;
    for (i = 0; i < TEST_HANDLER_NUM; i++) {
        if (test_handler[i] != NULL) {
            handler = test_handler[i];
            printf("exec 0x%x 0x%d\n", handler, test_handler_arg[i]);
            handler(test_handler_arg[i]);
            test_handler[i] = NULL;
        }
    }
#endif
    //printf("hb to do %s %d!!!", __func__, __LINE__);
}

/**
 *        IIC APIs
 */

/**
 * @brief   Function for initializing the IIC driver instance.
 * @param   [in] p_config: Pointer to the initial configuration.
 *          [in] handler: Event handler provided by the user.
 * @return  MI_SUCCESS              Initialized successfully.
 *          MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 *
 * */
mible_status_t mible_iic_init(const iic_config_t *p_config,
                              mible_handler_t handler)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Function for uninitializing the IIC driver instance.
 *
 *
 * */
void mible_iic_uninit(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);

}

/**
 * @brief   Function for sending data to a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [in] p_out:  Pointer to tx data
 *          [in] len:    Data length
 *          [in] no_stop: If set, the stop condition is not generated on the bus
 *          after the transfer has completed successfully (allowing for a repeated start in the next transfer).
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_out is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When tx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument.
 * */
mible_status_t mible_iic_tx(uint8_t addr, uint8_t *p_out, uint16_t len,
                            bool no_stop)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Function for receiving data from a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [out] p_in:  Pointer to rx data
 *          [in] len:    Data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_in is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When rx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument.
 * */
mible_status_t mible_iic_rx(uint8_t addr, uint8_t *p_in, uint16_t len)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

/**
 * @brief   Function for checking IIC SCL pin.
 * @param   [in] port:   SCL port
 *          [in] pin :   SCL pin
 * @return  1: High (Idle)
 *          0: Low (Busy)
 * */
int mible_iic_scl_pin_read(uint8_t port, uint8_t pin)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return 0;
}

#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"
#include "fs/resfile.h"
#if CONFIG_MIJIA_OTA_TO_FLASH
#include "net_update.h"
#define UPDATE_FILE_PATH    CONFIG_UPGRADE_OTA_FILE_NAME
static void *update_fd = NULL;
static FILE *update_file = NULL;
extern u32 get_update_file_size(void);
#else
#define UPDATE_FILE_PATH    CONFIG_ROOT_PATH"UPGRADE/update.ufw"
extern int storage_device_ready(void);
static FILE *update_file = NULL;
#endif

static u32 upfile_end_flag = 0;
static u32 dfu_nvm_end = 0;
static u32 dfu_write_addr = 0;
static u8 dfu_end_pack[67];
mible_status_t mible_nvm_init(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
#if CONFIG_MIJIA_OTA_TO_FLASH
    u8 wait;
    while (!storage_device_ready()) {//等待文件系统挂载完成
        os_time_dly(50);
        wait++;
        if (wait > 3) {
            printf("wait SD ready timeout!!");
            return MI_ERR_TIMEOUT;
        }
    }
    if (update_file) {
        fclose(update_file);
        update_file = NULL;
    }

    fmk_dir(CONFIG_ROOT_PATH, "/UPGRADE", 0);
    update_file = fopen(CONFIG_ROOT_PATH"UPGRADE/update.ufw", "w+");
    if (!update_file) {
        printf("open update_file fail!");
        return MI_ERR_INTERNAL;
    }

    if (update_fd) {
        net_fclose(update_fd, 0);
        update_fd = NULL;
    }
    update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (!update_fd) {
        printf("open update_fd fail!");
        return MI_ERR_INTERNAL;
    }
    return MI_SUCCESS;
#else
    u8 wait;
    while (!storage_device_ready()) {//等待文件系统挂载完成
        os_time_dly(50);
        wait++;
        if (wait > 3) {
            return MI_ERR_TIMEOUT;
        }
    }
    if (update_file) {
        fclose(update_file);
        update_file = NULL;
    }
    update_file = fopen(UPDATE_FILE_PATH, "w+");
    if (!update_file) {
        return MI_ERR_INTERNAL;
    }
    return MI_SUCCESS;
#endif
}

/**
 * @brief   Function for reading data from Non-Volatile Memory.
 * @param   [out] p_data:  Pointer to data to be restored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Address in Non-Volatile Memory to read.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */

mible_status_t mible_nvm_read(void *p_data, uint32_t length, uint32_t address)
{
    printf("hb to do %s %d %x %x %x %x!!!", __func__, __LINE__, p_data, length, address, m_config.dfu_start);
    int rlen = 0;
#if CONFIG_MIJIA_OTA_TO_FLASH
    fseek(update_file, (address - m_config.dfu_start), SEEK_SET);
    rlen = fread(p_data, length, 1, update_file);
#else
    fseek(update_file, (address - m_config.dfu_start), SEEK_SET);
    rlen = fread(p_data, length, 1, update_file);
#endif
    if (rlen > 0) {
        return MI_SUCCESS;
    } else {
        return MI_ERR_INTERNAL;
    }
}

/**
 * @brief   Writes data to Non-Volatile Memory.
 * @param   [in] p_data:   Pointer to data to be stored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Start address used to store data.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */
mible_status_t mible_nvm_write(void *p_data, uint32_t length, uint32_t address)
{
    printf("hb to do %s %d %x %x %x %x!!!", __func__, __LINE__, p_data, length, address, m_config.dfu_start);
    int rlen = 0;
#if CONFIG_MIJIA_OTA_TO_FLASH
    printf("%d", get_update_file_size());
    if (upfile_end_flag) {
        goto __end;
    }
    if (get_update_file_size() && ((address + length - m_config.dfu_start) > get_update_file_size())) {//未考虑末尾包小于67字节的情况
        rlen = net_fwrite(update_fd, p_data, length - (address + length - m_config.dfu_start - get_update_file_size()), 0);
        upfile_end_flag = 1;
    } else {
        rlen = net_fwrite(update_fd, p_data, length, 0);
    }

__end :
    fseek(update_file, (address - m_config.dfu_start), SEEK_SET);
    rlen = fwrite(p_data, length, 1, update_file);
#else
    fseek(update_file, (address - m_config.dfu_start), SEEK_SET);
    rlen = fwrite(p_data, length, 1, update_file);
#endif
    if (rlen > 0) {
        return MI_SUCCESS;
    } else {
        return MI_ERR_INTERNAL;
    }
}

extern int update_test(void);
mible_status_t mible_upgrade_firmware(void)
{
    u32 vm_clear = 0;
#if CONFIG_MIJIA_OTA_TO_FLASH
    syscfg_write(CFG_OTA_PART_SIZE, &vm_clear, sizeof(u32));
    if (update_file) {
        fclose(update_file);
        update_file = NULL;
    }
    if (update_fd) {
        net_fclose(update_fd, 0);
        update_fd = NULL;
    }
    return MI_SUCCESS;
#else
    if (update_file) {
        fseek(update_file, 67, SEEK_END);
        ftruncate(update_file, ftell(update_file));
        fclose(update_file);
        update_file = NULL;
    }
    update_test();
    return MI_SUCCESS;
#endif
}


/**
 *@brief    reboot device.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_reboot(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_ERR_BUSY;
}

/**
 *@brief    set node tx power.
 *@param    [in] power : TX power in 0.1 dBm steps.
 *@return   0: success, negetive value: failure
 */
mible_status_t mible_set_tx_power(int16_t power)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return MI_SUCCESS;
}

static char g_logBuf[256] = {0};
int mible_log_printf(const char *sFormat, ...)
{
    printf("[mible_log]->");
    int len = 0;

    va_list ap;
    va_start(ap, sFormat);
    /* len = vsnprintf(g_logBuf, sizeof(g_logBuf), sFormat, ap); */
    /* if (len > 0) { */
    /* printf("%s", g_logBuf); */
    /* } */
    len = print(NULL, NULL, sFormat, ap);
    va_end(ap);
    return MI_SUCCESS;
}

int mible_log_hexdump(void *array_base, uint16_t array_size)
{
    //put_buf(array_base, array_size);
    return MI_SUCCESS;
}

#if 0
mi_config_t m_config = {
    .developer_version  = MIBLE_DEVELOPER_VERSION,
    .model              = MODEL_NAME,
    .pid                = PRODUCT_ID,
    .io                 = OOB_IO_CAP,
    .have_msc           = HAVE_MSC,
    .have_reset_button  = HAVE_RESET_BUTTON,
    .have_confirm_button = HAVE_CONFIRM_BUTTON,
    .schd_in_mainloop   = MI_SCHD_PROCESS_IN_MAIN_LOOP,
    .max_att_payload    = MAX_ATT_PAYLOAD,
    .dfu_start          = DFU_NVM_START,
    .dfu_size           = DFU_NVM_SIZE,
};

uint8_t *m_otp_base = POTP_BASE;
uint16_t m_otp_size = POTP_FULL_SIZE;

#endif


#if 0

mible_status_t mible_mcu_cmd_send(mible_mcu_cmd_t cmd, void *arg)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    mible_status_t ret = MI_SUCCESS;
    switch (cmd) {
    case MIBLE_MCU_GET_VERSION:
        break;
    case MIBLE_MCU_READ_DFUINFO:
        break;
    case MIBLE_MCU_WRITE_DFUINFO:
        break;
    case MIBLE_MCU_WRITE_FIRMWARE:
        break;
    case MIBLE_MCU_VERIFY_FIRMWARE:
        break;
    case MIBLE_MCU_UPGRADE_FIRMWARE:
        break;
    case MIBLE_MCU_TRANSFER:
        break;
    default:
        break;
    }
    return ret;
}

mible_status_t mible_mcu_cmd_wait(mible_mcu_cmd_t cmd, void *arg)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    mible_status_t ret = MI_SUCCESS;
    switch (cmd) {
    case MIBLE_MCU_GET_VERSION:
        ret = MI_ERR_NOT_FOUND;
        break;
    case MIBLE_MCU_READ_DFUINFO:
        ret = MI_ERR_NOT_FOUND;
        break;
    case MIBLE_MCU_WRITE_DFUINFO:
        break;
    case MIBLE_MCU_WRITE_FIRMWARE:
        break;
    case MIBLE_MCU_VERIFY_FIRMWARE:
        break;
    case MIBLE_MCU_UPGRADE_FIRMWARE:
        break;
    case MIBLE_MCU_TRANSFER:
        break;
    default:
        break;
    }
    return ret;
}

void mible_mcu_init(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
}

void mible_mcu_deinit(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
}

void mible_mcu_send_buffer(const uint8_t *pSend_Buf, uint16_t vCount)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
}

void mible_mcu_send_byte(uint8_t byte)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
}

void mible_mcu_flushinput(void)
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
}

PT_THREAD(mible_mcu_recv_bytes(void *bytes, uint16_t req, uint32_t timeout))
{
    printf("hb to do %s %d!!!", __func__, __LINE__);
    return PT_EXITED;
}

#endif


#endif
