#ifndef AdaptiveEchoSuppress_H
#define AdaptiveEchoSuppress_H
#define AES_SetAggressiveFactor 0
#define AES_SetInitAggressiveFactor 1
#define AES_SetAggressiveFactorFadeSpeed 2
#ifdef __cplusplus
extern "C"
{
#endif
// mode = [0 , 1, 2]
//the bigger vaule of mode , the better performance of duplex,
//but the more ram and computational resource needed
int AdaptiveEchoSuppress_GetMiniFrameSize(int mode);
int AdaptiveEchoSuppress_QueryBufSize(int mode);
int AdaptiveEchoSuppress_QueryTempBufSize(int mode);
void AdaptiveEchoSuppress_Init(void *AESRunBuffer,
                               int mode,
                               float AggressFactor,
                               float RefEngThr
                              );
void AdaptiveEchoSuppress_Process(void *AESRunBuffer,
                                  void *AESTempBuffer,
                                  short *reference,
                                  short *aec_output,
                                  short *res_output,
                                  int nFrame);
void AdaptiveEchoSuppress_Config(void *AESRunBuffer, int ConfigType, int *ConfigPara);
#ifdef __cplusplus
}
#endif
#endif