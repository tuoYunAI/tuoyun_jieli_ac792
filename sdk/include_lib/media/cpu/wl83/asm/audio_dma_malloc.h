/*************************************************************************************************/
/*!
*  \file      audio_dma_malloc.h
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _AUDIO_DMA_MALLOC_H_
#define _AUDIO_DMA_MALLOC_H_

void *media_dma_malloc(unsigned long size);

void media_dma_free(void *rmem);

#define SRC_DMA_MALLOC          media_dma_malloc
#define SRC_DMA_FREE            media_dma_free

#define EQ_DMA_MALLOC           media_dma_malloc
#define EQ_DMA_FREE             media_dma_free

#define DAC_DMA_MALLOC          media_dma_malloc
#define DAC_DMA_FREE            media_dma_free

#define AUD_ADC_DMA_MALLOC      media_dma_malloc
#define AUD_ADC_DMA_FREE        media_dma_free

#define SBC_DMA_MALLOC          media_dma_malloc
#define SBC_DMA_FREE            media_dma_free

#define PLNK_DMA_MALLOC         media_dma_malloc
#define PLNK_DMA_FREE           media_dma_free

#define ALNK_DMA_MALLOC         media_dma_malloc
#define ALNK_DMA_FREE           media_dma_free

#endif
