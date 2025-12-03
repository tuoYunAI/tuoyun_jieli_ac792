#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "dhcp_srv/dhcp_srv.h"
#include "event/net_event.h"
#include "net/assign_macaddr.h"
#include "syscfg_id.h"
#include "lwip/sockets.h"

char chl_tx_en_detpwr(char en);
static u8 exit_flag;

void det_pwr_status(void)
{
    exit_flag = 0;
    while (1) {
        chl_tx_en_detpwr(1);
        os_time_dly(1);

        if (exit_flag) {
            break;
        }
    }
}

void wifi_adaptivity_exit(void)
{
    exit_flag = 1;
}

static void wifi_adaptivity_task(void *priv)
{
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(100);
    }

    det_pwr_status();
}

void wifi_adaptivity_start(void)
{
    if (thread_fork("wifi_adaptivity_task", 10, 256, 0, NULL, wifi_adaptivity_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
