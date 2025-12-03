#ifndef NoiseSuppressLib_H
#define NoiseSuppressLib_H
#define NOISESP_CONFIG_FREEZE 0
#define NOISESP_CONFIG_NOISEFLOOR 1
#define NOISESP_CONFIG_LOWCUTTHR 2
#define NOISESP_CONFIG_MINSUPPRESS 3
#define NOISESP_CONFIG_AGGRESSFACTOR 4
#ifdef __cplusplus
extern "C"
{
#endif
int NoiseSuppress_GetMiniFrame(int is_wideband);
int NoiseSuppress_QueryProcessDelay(int mode, int is_wideband);
int NoiseSuppress_QueryBufSize(int mode, int is_wideband);
int NoiseSuppress_QueryTempBufSize(int mode, int is_wideband);
void NoiseSuppress_Init(void *NoiseSpRunBuffer,
                        float AggressFactor,
                        float minSuppress,
                        int mode,
                        int is_wideband,
                        float noise_lvl);
void NoiseSuppress_Process(void *NoiseSpRunBuffer,
                           void *NoiseSpTempBuffer,
                           short *input,
                           short *output,
                           short *inputH,
                           short *outputH,
                           int npoint);
void NoiseSuppress_Config(void *NoiseSpRunBuffer,
                          int ConfigType,
                          int *ConfigPara);
#ifdef __cplusplus
}
#endif
#endif