#ifndef SingleMicSystem_H
#define SingleMicSystem_H
#define TDE_InvalidDelay -10000
#define SINGLEMICSYSTEM_CONFIG_SETENABLEBIT 0
#define SINGLEMICSYSTEM_CONFIG_FREEZENOISEESTIMATION 1
#define SMS_AEC_EN 1
#define SMS_NLP_EN 2
#define SMS_ANS_EN 4
#define SMS_TDE_EN 32
#define SMS_TDEYE_EN 64

typedef struct {
    int TDE_Process_MaxFrequency;
    int TDE_Process_MinFrequency;
    int TDE_SM_MaxFrequency;
    int TDE_SM_MinFrequency;
    int TDE_AF_Length;
    float TDE_AdaptiveRate;
    float TDE_Disconverge_ERLE_Thr;
    float XEngThr;
    float ErleThr;
    float gain;
    int n;
    int ValidThr;
    int TDE_SimplexProc_Mode;
    int TDE_SimplexProcess_Thr;
} SingleMicSystem_TDEConfig;
typedef struct {
    int AEC_Process_MaxFrequency;
    int AEC_Process_MinFrequency;
    int AF_Length;
    float AdaptiveRate;
    float Disconverge_ERLE_Thr;
    float gain_erle;
} SingleMicSystem_AECConfig;
typedef struct {
    int NLP_Process_MaxFrequency;
    int NLP_Process_MinFrequency;
    float OverDrive;
    int Mode;
    int SP_Level;
    int SP_MaxFreq;
    int SP_MinFreq;
    int DynaODMode;
    float NLP_beta;
    float NLP_gamma;
    int OverSpMode;
    float NLPOSp_ResThreshold;
    float NLPODB_Threshold;
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
/*单声道参数数据回音消除*/
int SMS_TDE_GetMiniFrameSize(int is_wideband);
int SMS_TDE_GetProcessDelay(int is_wideband);
int SMS_TDE_QueryBufSize(SingleMicSystem_TDEConfig *tde_cfg,
                         SingleMicSystem_AECConfig *aec_cfg,
                         SingleMicSystem_NLPConfig *nlp_cfg,
                         SingleMicSystem_ANSConfig *ans_cfg,
                         int is_wideband,
                         int EnableBit);
int SMS_TDE_QueryTempBufSize(SingleMicSystem_TDEConfig *tde_cfg,
                             SingleMicSystem_AECConfig *aec_cfg,
                             SingleMicSystem_NLPConfig *nlp_cfg,
                             SingleMicSystem_ANSConfig *ans_cfg,
                             int is_wideband,
                             int EnableBit);
void SMS_TDE_Init(void *runbuf,
                  void *tmpbuf,
                  SingleMicSystem_TDEConfig *tde_cfg,
                  SingleMicSystem_AECConfig *aec_cfg,
                  SingleMicSystem_NLPConfig *nlp_cfg,
                  SingleMicSystem_ANSConfig *ans_cfg,
                  float GloabalMinSuppress,
                  int is_wideband,
                  int EnableBit);
void SMS_TDE_Process(void *runbuf,
                     void *tempbuf,
                     short *reference,
                     short *microphone,
                     short *output,
                     int *Delay,
                     int nMiniFrame);
void SMS_TDE_Config(void *runbuf, int ConfigType, int *ConfigPara);

/*立体声参数数据回音消除*/
int SMS_TDE_Stereo_GetMiniFrameSize(int is_wideband);
int SMS_TDE_Stereo_GetProcessDelay(int is_wideband);
int SMS_TDE_Stereo_QueryBufSize(SingleMicSystem_TDEConfig *tde_cfg,
                                SingleMicSystem_AECConfig *aec_cfg,
                                SingleMicSystem_NLPConfig *nlp_cfg,
                                SingleMicSystem_ANSConfig *ans_cfg,
                                int is_wideband,
                                int EnableBit);
int SMS_TDE_Stereo_QueryTempBufSize(SingleMicSystem_TDEConfig *tde_cfg,
                                    SingleMicSystem_AECConfig *aec_cfg,
                                    SingleMicSystem_NLPConfig *nlp_cfg,
                                    SingleMicSystem_ANSConfig *ans_cfg,
                                    int is_wideband,
                                    int EnableBit);
void SMS_TDE_Stereo_Init(void *runbuf,
                         void *tmpbuf,
                         SingleMicSystem_TDEConfig *tde_cfg,
                         SingleMicSystem_AECConfig *aec_cfg,
                         SingleMicSystem_NLPConfig *nlp_cfg,
                         SingleMicSystem_ANSConfig *ans_cfg,
                         float GloabalMinSuppress,
                         int is_wideband,
                         int EnableBit);
void SMS_TDE_Stereo_Process(void *runbuf,
                            void *tempbuf,
                            short *reference,
                            short *reference2,
                            short *microphone,
                            short *output,
                            int *Delay,
                            int nMiniFrame);
void SMS_TDE_Stereo_Config(void *runbuf, int ConfigType, int *ConfigPara);

#ifdef __cplusplus
}
#endif
#endif
