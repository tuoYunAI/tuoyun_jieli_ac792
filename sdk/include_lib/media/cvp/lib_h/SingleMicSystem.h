#ifndef SingleMicSystem_H
#define SingleMicSystem_H
#define SINGLEMICSYSTEM_CONFIG_SETENABLEBIT 0
#define SINGLEMICSYSTEM_CONFIG_FREEZENOISEESTIMATION 1
#define SMS_AEC_EN 1
#define SMS_NLP_EN 2
#define SMS_ANS_EN 4
typedef struct {
    int AEC_Process_MaxFrequency;
    int AEC_Process_MinFrequency;
    int AF_Length;
    float AdaptiveRate;
    float Disconverge_ERLE_Thr;
} SingleMicSystem_AECConfig;
typedef struct {
    int NLP_Process_MaxFrequency;
    int NLP_Process_MinFrequency;
    float OverDrive;
} SingleMicSystem_NLPConfig;
typedef struct {
    float AggressFactor;
    float minSuppress;
    float init_noise_lvl;
} SingleMicSystem_ANSConfig;
#ifdef __cplusplus
extern "C"
{
#endif
int SingleMicSystem_GetMiniFrameSize(int is_wideband);
int SingleMicSystem_GetProcessDelay(int is_wideband);
int SingleMicSystem_QueryBufSize(SingleMicSystem_AECConfig *aec_cfg,
                                 SingleMicSystem_NLPConfig *nlp_cfg,
                                 SingleMicSystem_ANSConfig *ans_cfg,
                                 int is_wideband,
                                 int EnableBit);
int SingleMicSystem_QueryTempBufSize(SingleMicSystem_AECConfig *aec_cfg,
                                     SingleMicSystem_NLPConfig *nlp_cfg,
                                     SingleMicSystem_ANSConfig *ans_cfg,
                                     int is_wideband,
                                     int EnableBit);
void SingleMicSystem_Init(void *runbuf,
                          SingleMicSystem_AECConfig *aec_cfg,
                          SingleMicSystem_NLPConfig *nlp_cfg,
                          SingleMicSystem_ANSConfig *ans_cfg,
                          float GloabalMinSuppress,
                          int is_wideband,
                          int EnableBit);
void SingleMicSystem_Process(void *runbuf,
                             void *tempbuf,
                             short *reference,
                             short *microphone,
                             short *output,
                             int nMiniFrame);
void SingleMicSystem_Config(void *runbuf, int ConfigType, int *ConfigPara);
#ifdef __cplusplus
}
#endif
#endif