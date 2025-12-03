
#ifndef SplittingFilter_H
#ifdef __cplusplus
extern "C"
{
#endif
int SplittingFilter_QueryBufSize(int mode);
int SplittingFilter_QueryTempBufSize(int MaxFrameSize);
void SplittingFilter_Init(void *SpFiltRunBuffer, int mode);
void SplittingFilter_Analyse(void *SpFiltRunBuffer, void *SpFilterempBuffer, short *input, short *HBand, short *LBand, int nPoint); //nPoint must be evem
void SplittingFilter_Synthesize(void *SpFiltRunBuffer, void *SpFilterempBuffer, short *HBand, short *LBand, short *output, int nPointPerBand);
#ifdef __cplusplus
}
#endif
#endif