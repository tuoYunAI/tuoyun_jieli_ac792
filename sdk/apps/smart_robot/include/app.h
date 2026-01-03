#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__


typedef enum{
    APP_NOT_INIT,
    APP_INIT_DONE,
    APP_RUNNING,
    APP_INTERRUPTED,
}app_status_t;


app_status_t get_network_status();

#endif