#ifndef BoneSystem_H
#define BoneSystem_H

#define BMS_AEC_EN 1
#define BMS_NLP_EN 2
#define BMS_ANS_EN 4
#define BMS_MAP_EN 8

typedef struct {
    int MIC_AEC_Process_MaxFrequency;
    int MIC_AEC_Process_MinFrequency;
    int Bone_AEC_Process_MaxFrequency;
    int Bone_AEC_Process_MinFrequency;
    int AF_Length;
} BoneSystem_AECConfig;

typedef struct {
    int NLP_Process_MaxFrequency;
    int NLP_Process_MinFrequency;
    float OverDrive;
} BoneSystem_NLPConfig;

typedef struct {
    int MAP_Process_MaxFrequency;
    int MAP_Process_MinFrequency;
    float bone_init_noise_lvl;
    int *MAP_Weight;

} BoneSystem_MAPConfig;

typedef struct {
    float AggressFactor;
    float minSuppress;
    float init_noise_lvl;
} BoneSystem_ANSConfig;
#ifdef __cplusplus
extern "C"
{
#endif
int BoneSystem_GetProcessDelay(int is_wideband);
int BoneSystem_GetMiniFrameSize(int is_wideband);
int BoneSystem_QueryBufSize(BoneSystem_AECConfig *aec_cfg,
                            BoneSystem_NLPConfig *nlp_cfg,
                            BoneSystem_MAPConfig *map_cfg,
                            BoneSystem_ANSConfig *ans_cfg,
                            int is_wideband,
                            int EnableBit
                           );
int BoneSystem_QueryTempBufSize(BoneSystem_AECConfig *aec_cfg,
                                BoneSystem_NLPConfig *nlp_cfg,
                                BoneSystem_MAPConfig *map_cfg,
                                BoneSystem_ANSConfig *ans_cfg,
                                int is_wideband,
                                int EnableBit
                               );
void BoneSystem_Init(void *runbuf,
                     void *tempbuf,
                     BoneSystem_AECConfig *aec_cfg,
                     BoneSystem_NLPConfig *nlp_cfg,
                     BoneSystem_MAPConfig *map_cfg,
                     BoneSystem_ANSConfig *ans_cfg,
                     int is_wideband,
                     int EnableBit
                    );
void BoneSystem_Process(void *runbuf,
                        void *tempbuf,
                        short *reference,
                        short *mic_data,
                        short *bone_data,
                        short *output,
                        int nMiniFrame
                       );
int *GetMAPWeight(void *runbuf);
int GetMAPWeightSize(BoneSystem_MAPConfig *map_cfg, int is_wideband);

#ifdef __cplusplus
}
#endif
#endif
