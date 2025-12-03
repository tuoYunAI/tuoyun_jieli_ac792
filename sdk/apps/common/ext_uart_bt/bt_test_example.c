#include "app_config.h"
#include "system/includes.h"
#include "uart_manager.h"

#if TCFG_INSTR_DEV_UART_ENABLE
/**** *************
蓝牙测试项例子参考文件
*******************/
//#define BT_ON_OFF_TEST_ENABLE      //设置蓝牙开关闭测试项   OK
// #define BT_NAME_SET_TEST_ENABLE    //设置蓝牙名测试项       OK
// #define BT_NAME_GET_TEST_ENABLE    //获取蓝牙名测试项       OK
// #define BT_MAC_GET_TEST_ENABLE     //获取蓝牙MAC地址测试项  OK
// #define BT_MAC_SET_TEST_ENABLE     //设置蓝牙MAC地址测试项  OK
// #define BT_START_CONN_TEST_ENABLE  //蓝牙开始连接测试项     OK

// #define BT_MUSIC_PP_TEST_ENABLE          //控制蓝牙音乐暂停/播放测试项    //OK
// #define BT_MUSIC_PREV_TEST_ENABLE        //控制蓝牙音乐上一首测试项      //OK
// #define BT_MUSIC_NEXT_TEST_ENABLE        //控制蓝牙音乐下一首测试项      //ok
// #define BT_MUSIC_VOL_UP_TEST_ENABLE      //控制蓝牙音增加音量测试项      //OK
// #define BT_MUSIC_VOL_DOWN_TEST_ENABLE    //控制蓝牙音减小音量测试项      //OK
// #define BT_MUSIC_VOL_GET_TEST_ENABLE     //获取蓝牙当前音量测试项       //OK
// #define BT_GET_REMOTE_NAME_TEST_ENABLE   //远程获取蓝牙名测试项  //从机主动发,点配对之前发送 .OK

// #define BT_PAIR_RSP_TEST_ENABLE          //发送确定或取消配对测试项  //OK
// #define BT_PHONE_CALL_CRL_TEST_ENABLE    //电话接听/挂断测试项      //OK
// #define BT_PHONE_GET_PBAP_TEST_ENABLE    //获取蓝牙电话本测试项     //OK
// #define BT_OTA_GET_VERSION_TEST_ENABLE      //获取JL710固件版本测试项     //OK
// #define BT_MCU_CTRL_SHUTDOWN_TEST_ENABLE      //控制从机关机版本测试项     //ok
// #define BT_GET_POWERON_STATUS_TEST_ENABLE      //获取从机的开机状态测试项     //ok
// #define BT_PHONE_3WAYCALL_CTRL_TEST_ENABLE      //控制第三方通个话接听/挂断测试项     //ok
// #define BT_SET_RF_FREQ_TEST_ENABLE                  //测试设置从机RF工作频点测试项目    //ok


void bt_on_off_test()
{
    printf("%s %d\n", __func__, __LINE__);

    static u8 flag = 0;

    if (!flag) {
        bt_on_off_control(1);
    } else {
        bt_on_off_control(0);
    }

    flag = !flag;
}

void bt_mac_set_test()
{
    printf("%s %d\n", __func__, __LINE__);

    u8 bt_mac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x22};
    bt_mac_set_control(bt_mac, 6);
}

void bt_mac_get_test()
{
    printf("%s %d\n", __func__, __LINE__);
    bt_mac_get_control();
}

void bt_name_set_test()
{
    u8 bt_name[] = {'a', '4', '5', 'e', '3', 'l'};
    bt_name_set_control(bt_name, sizeof(bt_name));
}

void bt_name_get_test()
{
    bt_name_get_control();
}

void bt_start_connec_test()
{
    bt_start_connec_control();
}

void bt_music_pp_test()
{
    bt_music_pp_control();
}

void bt_music_prev_test()
{
    bt_music_prev_control();
}

void bt_music_next_test()
{
    bt_music_next_control();
}

void bt_music_vol_up_test()
{
    bt_music_vol_up_control();
}

void bt_music_vol_down_test()
{
    bt_music_vol_down_control();
}

void bt_music_vol_get_test()
{
    bt_music_vol_get_control();
}

void bt_get_remote_name_test()
{
    bt_get_remote_name_control();
}

void bt_pair_rsp_test()
{
    static u8 flag = 0;

    if (!flag) {
        bt_pair_rsp_control(1); //确认
    } else {
        bt_pair_rsp_control(0); //取消
    }
    flag = !flag;
}

void bt_phone_call_ctrl_test()
{
    static u8 flag = 0;

    if (!flag) {
        bt_phone_call_conctrl(0);
    } else {
        bt_phone_call_conctrl(1);
    }
    flag = !flag;
}

void bt_phone_get_pbap_test()
{
    bt_phone_get_pbap_info();
}

void bt_ota_get_version_test()
{
    printf("%s %d\n", __func__, __LINE__);
    bt_ota_request_version_info();
}

void bt_mcu_ctrl_shutdown_test()
{
    bt_shutdown_control();
}

void bt_get_power_status_test()
{
    bt_get_poweron_status_control();
}

void bt_phone_3waycall_test()
{
    static u8 flag = 0;

    if (!flag) {
        bt_phone_3WayCall_conctrl(1);    //接听
    } else {
        bt_phone_3WayCall_conctrl(2);    //挂断
    }

    flag = !flag;
}

void bt_set_rf_freq_test()
{
    printf("%s %d\n", __func__, __LINE__);

    bt_on_off_control(1);//需要先打开蓝牙，等蓝牙打开成功后，才能设置频点
    os_time_dly(50);    //建议是等打开蓝牙命令回复后，再设置蓝牙频点
    bt_set_rf_frequency_control();
}

void bt_list_test_func()
{
#ifdef BT_ON_OFF_TEST_ENABLE
    bt_on_off_test();
#endif

#ifdef BT_MAC_SET_TEST_ENABLE
    bt_mac_set_test();
#endif

#ifdef BT_MAC_GET_TEST_ENABLE
    bt_mac_get_test();
#endif

#ifdef BT_NAME_SET_TEST_ENABLE
    bt_name_set_test();
#endif

#ifdef BT_NAME_GET_TEST_ENABLE
    bt_name_get_test();
#endif

#ifdef BT_START_CONN_TEST_ENABLE
    bt_start_connec_test();
#endif

#ifdef BT_MUSIC_PP_TEST_ENABLE
    bt_music_pp_test();
#endif

#ifdef BT_MUSIC_PREV_TEST_ENABLE
    bt_music_prev_test();
#endif

#ifdef BT_MUSIC_NEXT_TEST_ENABLE
    bt_music_next_test();
#endif

#ifdef BT_MUSIC_VOL_UP_TEST_ENABLE
    bt_music_vol_up_test();
#endif

#ifdef BT_MUSIC_VOL_DOWN_TEST_ENABLE
    bt_music_vol_down_test();
#endif

#ifdef BT_MUSIC_VOL_GET_TEST_ENABLE
    bt_music_vol_get_test();
#endif

#ifdef BT_GET_REMOTE_NAME_TEST_ENABLE
    bt_get_remote_name_test();
#endif

#ifdef BT_PAIR_RSP_TEST_ENABLE
    bt_pair_rsp_test();
#endif

#ifdef BT_PHONE_CALL_CRL_TEST_ENABLE
    bt_phone_call_ctrl_test();
#endif

#ifdef BT_PHONE_GET_PBAP_TEST_ENABLE
    bt_phone_get_pbap_test();
#endif

#ifdef BT_OTA_GET_VERSION_TEST_ENABLE
    bt_ota_get_version_test();
#endif

#ifdef BT_MCU_CTRL_SHUTDOWN_TEST_ENABLE
    bt_mcu_ctrl_shutdown_test();
#endif

#ifdef BT_GET_POWERON_STATUS_TEST_ENABLE
    bt_get_power_status_test();
#endif

#ifdef BT_PHONE_3WAYCALL_CTRL_TEST_ENABLE
    bt_phone_3waycall_test();
#endif

#ifdef BT_SET_RF_FREQ_TEST_ENABLE
    bt_set_rf_freq_test();
#endif
}
#endif

