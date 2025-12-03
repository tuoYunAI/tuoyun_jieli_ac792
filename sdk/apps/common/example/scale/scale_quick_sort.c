#include "scale_quick_sort.h"

#define isdigit(c) ((c) >= '0' && (c) <= '9')
static int skip_atoi(const char **s)
{
    int i = 0;

    //*s= *s+4;
    while (isdigit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }

    return i;
}

static void swap(void **e1, void **e2)
{
    void *tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

static int partition(void **array, int start, int end, CompareFunc compare)
{
    void *p;
    int i;
    int j;

    p = array[start];
    i = start + 1;
    j = end;
    while (i <= j) {
        if (compare(array[i], p) <= 0) {
            i++;
        } else if (compare(array[j], p) > 0) {
            j--;
        } else {
            swap(&array[i], &array[j]);
            i++;
            j--;
        }
    }
    swap(&array[start], &array[j]);
    return j;
}

static int qs_stack[1024];
static int qs_stack_ptr;
static int qs_stack_rear_ptr;

void scale_quick_sort(void **array, int start, int end, CompareFunc compare)
{
    int p;

    qs_stack_ptr = 0;
    qs_stack_rear_ptr = 0;

    qs_stack[qs_stack_ptr++] = start;
    qs_stack[qs_stack_ptr++] = end;

    while (qs_stack_ptr != qs_stack_rear_ptr) {
        //		printf("qs_stack_ptr:%d\n",qs_stack_ptr);
        //		printf("qs_stack_rear_ptr:%d\n",qs_stack_rear_ptr);

        start = qs_stack[qs_stack_rear_ptr++];
        end = qs_stack[qs_stack_rear_ptr++];

        //		printf("start:%d\n",start);
        //		printf("end:%d\n",end);

        if (start >= end) {
            continue;
        }

        if (qs_stack_rear_ptr >= 16 * 1024) {
            qs_stack_rear_ptr = 16 * 1024;
        }

        //		printf("qs_stack_ptr:%d\n",qs_stack_ptr);
        //		printf("qs_stack_rear_ptr:%d\n",qs_stack_rear_ptr);

        p = partition(array, start, end, compare);

        qs_stack[qs_stack_ptr++] = start;
        qs_stack[qs_stack_ptr++] = p - 1;

        if (qs_stack_ptr >= 16 * 1024) {
            qs_stack_ptr = 16 * 1024;
        }

        qs_stack[qs_stack_ptr++] = p + 1;
        qs_stack[qs_stack_ptr++] = end;

        if (qs_stack_ptr >= 16 * 1024) {
            qs_stack_ptr = 16 * 1024;
        }
    }
}

int scale_compare_string(void *ptr1, void *ptr2)
{
    //	printf("ptr1:%s\n",ptr1);
    //	printf("ptr2:%s\n",ptr2);
    int el1 = skip_atoi(&ptr1);
    int el2 = skip_atoi(&ptr2);

    //	printf("el1:%d\n",el1);
    //	printf("el2:%d\n",el2);

    if (el1 < el2) {
        return -1;
    }
    if (el1 == el2) {
        return 0;
    }
    return 1;
}
