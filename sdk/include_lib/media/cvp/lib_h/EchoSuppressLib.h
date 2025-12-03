#ifndef EchoSuppressLib_H
#define EchoSuppressLib_H
#define ECHOSP_CONFIG_SETFIXSUPPRESS 	0
#define ECHOSP_CONFIG_GETSUPPRESSTATE 	1
#ifdef __cplusplus
extern "C"
{
#endif
int EchoSuppress_QueryBufSize();
int EchoSuppress_QueryTempBufSize();
void EchoSuppress_Init(void *EchoSpRunBuffer, float AggressFactor,
                       float minSuppress,
                       float OverSuppressThr,
                       float OverSuppress);
void EchoSuppress_Process(void *EchoSpRunBuffer,
                          void *EchoSpTempBuffer,
                          short *far,
                          short *near,
                          short *error,
                          short *output,
                          short *inputH,
                          short *outputH,
                          int npoint);
void EchoSuppress_Config(void *EchoSpRunBuffer,
                         int ConfigType,
                         int *ConfigPara);
#ifdef __cplusplus
}
#endif
#endif
