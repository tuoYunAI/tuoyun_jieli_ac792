#ifndef AptFilt_H
#define APTFILT_CONFIG_GETCONVERGENCE 0
#define APTFILT_CONFIG_SETPATHCHANGE 1
#ifdef __cplusplus
extern "C"
{
#endif
int AptFilt_QueryBufSize(int M, int B);
int AptFilt_QueryTempBufSize(int M, int B);
void AptFilt_Init(void *AptFilRunBuffer,
                  int M,
                  int B,
                  float ConverThr,
                  float RefEngThr,
                  float DT_AggressiveFactor); //DT = Double Talk; Bigger to trace better,but will be unstable,range = 1 ~ 5
void AptFilt_Process(void *AptFilRunBuffer,
                     void *AptFilTempBuffer,
                     short *far,
                     short *near,
                     short *output,
                     int npoint); //npoint must be multiply of 64
void AptFilt_Config(void *AptFilRunBuffer, int ConfigType, int *ConfigData);
#ifdef __cplusplus
}
#endif
#endif
