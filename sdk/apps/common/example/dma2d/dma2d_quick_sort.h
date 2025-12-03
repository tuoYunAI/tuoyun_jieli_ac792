#ifndef __DMA2D_QUICK_SORT_H__
#define __DMA2D_QUICK_SORT_H__


typedef int(*CompareFunc)(void *, void *);


void dma2d_quick_sort(void **array, int start, int end, CompareFunc compare);

int dma2d_compare_string(void *ptr1, void *ptr2);

#endif
