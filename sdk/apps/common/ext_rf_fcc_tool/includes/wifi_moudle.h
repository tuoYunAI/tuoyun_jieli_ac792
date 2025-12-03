#ifndef __FCC_WIFI_MOUDLE_H__
#define __FCC_WIFI_MOUDLE_H__

#include "typedef.h"


int mp_start(int start);// enter MP mode
int mp_arx_start(u8 start);// start air Rx teseting, 0(stop), 1(start)

int mp_arx_mon(int enable) ;// enter air Rx monitor mode, 0(exit), 1(enter)


int mp_get_tx_power(u8 rfpath, u8 *idx); //rfpath:0('A'), 1('B'), idx:power level

int mp_set_tx_power(u8 rfpath_a, u8 rfpath_b, u8 rfpath_c, u8 rfpath_d); //rfpath_a: patha power level, rfpath_b: pathb power level,

int mp_set_ant_tx(u8 *antenna_x);//# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.

int mp_set_ant_rx(u8 *antenna_x); //# Select antenna A for operation,if device have 2x2 antennam select antenna "a" or "b" and "ab" for operation.

int mp_write_reg(u32 addr, u32 data, u32 data_size);

int mp_read_reg(u32 addr, void *data, u32 data_size);

int mp_write_rf(u8 rf_path, u8 addr, u32 data);

int mp_read_rf(u8 rf_path, u8 addr, u32 *data);

int mp_dump_mac_bb_rf(void);

int mp_set_thermal(u8 write);

int mp_set_psd(u32 psd_pts, u32 psd_start, u32 psd_stop);

int mp_reset_stats(void);

int mp_query(u32 *tx_ok, u32 *rx_ok);

int mp_get_version(void);

/*

"1M","2M","5.5M","11M","6M","9M","12M","18M","24M","36M","48M","54M",
"HTMCS0","HTMCS1","HTMCS2","HTMCS3","HTMCS4","HTMCS5","HTMCS6","HTMCS7",
"HTMCS8","HTMCS9","HTMCS10","HTMCS11","HTMCS12","HTMCS13","HTMCS14","HTMCS15",
"HTMCS16","HTMCS17","HTMCS18","HTMCS19","HTMCS20","HTMCS21","HTMCS22","HTMCS23",
"HTMCS24","HTMCS25","HTMCS26","HTMCS27","HTMCS28","HTMCS29","HTMCS30","HTMCS31",
"VHT1MCS0","VHT1MCS1","VHT1MCS2","VHT1MCS3","VHT1MCS4","VHT1MCS5","VHT1MCS6","VHT1MCS7","VHT1MCS8","VHT1MCS9",
"VHT2MCS0","VHT2MCS1","VHT2MCS2","VHT2MCS3","VHT2MCS4","VHT2MCS5","VHT2MCS6","VHT2MCS7","VHT2MCS8","VHT2MCS9",
"VHT3MCS0","VHT3MCS1","VHT3MCS2","VHT3MCS3","VHT3MCS4","VHT3MCS5","VHT3MCS6","VHT3MCS7","VHT3MCS8","VHT3MCS9",
"VHT4MCS0","VHT4MCS1","VHT4MCS2","VHT4MCS3","VHT4MCS4","VHT4MCS5","VHT4MCS6","VHT4MCS7","VHT4MCS8","VHT4MCS9"

*/
int mp_set_rate(const char *rate);

int mp_set_efuse(u32 addr, u32 val);

int mp_get_efuse(u8 efuse_map[0x200]);

int mp_efuse_get_mac(u8 buf[6]);

int mp_set_bandwidth(u8 bandwidth, u8 short_gi);//bandwidth:0(20M)/1(40M)/2(80M); short_gi:1/0

int mp_crystal_cap_adjust(u8 val);//Range 0~3F h, Default 20h


int mp_ctx_stop(void) ;//# stop continuous Tx

int mp_con_tx(void); //# start continuous Tx

int mp_con_pkt_tx(void); //# start continuous Packet Tx


int mp_single_tone_tx(void); //# start sending single tone signal


int mp_carrier_suppression_tx(void); //# start sending carrier suppression signal

int mp_count_pkt_tx(u32 npackets); //npackets of packets start packet Tx

int rtw_set_tx_power_idx(u8 val);//0-63

int rtw_get_best_ch(void (*get_best_ch_fn)(int, int));
int rtw_scan(void);

void wifi_fcc_moudle_init(void);
void wifi_set_mp_test_status(u8 status);
u8 wifi_get_mp_test_status(void) ;
int WiFi_Continuous_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime);
int WiFi_Continuous_Packet_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime);
int WiFi_Count_Packet_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower,  const char *mp_rate, u32 npackets);
int WiFi_Carrier_suppression_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime);
int WiFi_Single_Tone_Tx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u8 pathx_txpower, const char *mp_rate, u32 tx_durtime);
int WiFi_Air_Rx_testing(u8 mp_channel, u8 bandwidth, u8 short_gi, u8 *antenna_x, u32 rx_durtime);

#endif


