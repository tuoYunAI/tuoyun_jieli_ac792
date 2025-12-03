
#ifndef DelayEstimate_H
#define DelayEstimate_InvalidDelay -10000
#ifdef __cplusplus
extern "C"
{
#endif
int DelayEstimate_GetMiniFrameSize();
int DelayEstimate_QueryBufSize(int is_wideband);
int DelayEstimate_QueryTempBufSize();
void DelayEstimate_Init(void *DlyEstRunBuffer, int is_wideband, float EngThr, int ValidThr);
int DelayEstimate_Process(void *DlyEstRunBuffer, void *DelayEstimatetempBuffer, short *far, short *near, int npoint);
void DelayEstimate_Config(void *DlyEstRunBuffer, int ConfigType, int *ConfigData);
#ifdef __cplusplus
}
#endif
#endif