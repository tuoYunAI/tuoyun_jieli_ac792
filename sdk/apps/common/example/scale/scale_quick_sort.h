#ifndef __SCALE_QUICK_SORT_H__
#define __SCALE_QUICK_SORT_H__


typedef int(*CompareFunc)(void *, void *);


void scale_quick_sort(void **array, int start, int end, CompareFunc compare);

int scale_compare_string(void *ptr1, void *ptr2);

#endif
