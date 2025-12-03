#ifndef __JLSP_LLNS_H__
#define __JLSP_LLNS_H__


typedef struct {
    float gainfloor;
    float suppress_level;
} llns_dns_param_t;


typedef struct {
    int agc_en;
    int min_mag_db_level;
    int max_mag_db_level;
    int addition_mag_db_level;
    int clip_mag_db_level;
    int floor_mag_db_level;

} llns_dns_agc_param_t;

/*
gain_floor: 增益的最小值控制,范围0~1,建议值(0~0.2)之间
suppress_level: 控制降噪强度:
0 < suppress_level < 1，越小降噪强度越轻，太小噪声会很大；
suppress_level = 1,正常降噪
suppress_level > 1,降噪强度加强，越大降噪强度越强，太大会吃音
建议调节范围0.3~3之间来控制降噪强度的强弱
*/

void *JLSP_llns_dns_init(char *private_buffer, int private_size, char *shared_buffer, int share_size, int samplerate, float gain_floor, float suppress_level);
int JLSP_llns_dns_get_heap_size(int *private_size, int *shared_size, int samplerate);

int JLSP_llns_dns_reset(void *m);
void JLSP_llns_dns_update_shared_buffer(void *m, char *shared_buffer);
int JLSP_llns_dns_process(void *m, void *input, void *output, int *outsize);
int JLSP_llns_dns_free(void *m);

int JLSP_llns_dns_set_winsize(void *m, int winsize);
int JLSP_llns_dns_set_parameters(void *m, llns_dns_param_t *p);
int JLSP_llns_dns_set_subframe_div(void *m, int div);

void JLSP_llns_dns_set_noiselevel(void *m, float noise_level_init);
//设置降噪等级，分为4档，0：不降噪，1：降噪强度较轻，2：降噪强度适中，3：降噪强度较强；默认选择2
int JLSP_llns_dns_set_level(void *m, int level);
//agc模块通过const变量LLNS_AGC_EN使能，0：不使能，1：使能；如果不使用该模块也可以通过外部配置drc来实现同样功能
int JLSP_llns_dns_set_agc(void *m, void *cfg);

#endif



