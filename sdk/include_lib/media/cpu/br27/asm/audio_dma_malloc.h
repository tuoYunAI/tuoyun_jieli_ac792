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

#define SRC_DMA_MALLOC      	malloc
#define SRC_DMA_FREE        	free

#define EQ_DMA_MALLOC       	dma_malloc
#define EQ_DMA_FREE         	dma_free

#define DAC_DMA_MALLOC      	dma_malloc
#define DAC_DMA_FREE        	dma_free

#define AUD_ADC_DMA_MALLOC      dma_malloc
#define AUD_ADC_DMA_FREE        dma_free

#define SBC_DMA_MALLOC          dma_malloc
#define SBC_DMA_FREE            dma_free

#endif
