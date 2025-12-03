#ifndef __SIMPLE_PTHREAD_COMM_H__
#define __SIMPLE_PTHREAD_COMM_H__

#include <stdint.h>
#include <time.h>

/***************************common start****************************/
/**
 * @name Mutex types.
 */
/**@{ */
#ifndef PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_NORMAL        0                        /**< Non-robust, deadlock on relock, does not remember owner. */
#endif
#ifndef PTHREAD_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_ERRORCHECK    1                        /**< Non-robust, error on relock,  remembers owner. */
#endif
#ifndef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE     2                        /**< Non-robust, recursive relock, remembers owner. */
#endif
#ifndef PTHREAD_MUTEX_DEFAULT
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL     /**< PTHREAD_MUTEX_NORMAL (default). */
#endif
/**@} */

typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0

/***************************common end****************************/


#if defined CONFIG_UCOS_ENABLE

#define PTHREAD_COND_INITIALIZER    (0)
#define PTHREAD_MUTEX_INITIALIZER   {0}

typedef int sem_t;
typedef int pthread_t;
typedef int pthread_mutex_t;
typedef int pthread_cond_t;
typedef void pthread_condattr_t;
typedef void *pthread_barrierattr_t;
typedef void pthread_barrier_t; //TODO
typedef int sem_t;   //TODO
typedef int mqd_t;

typedef struct pthread_attr {
    uint16_t stacksize;
    uint16_t priority;
} pthread_attr_t;

typedef struct {
    int type;
} pthread_mutexattr_t;


#elif defined CONFIG_FREE_RTOS_ENABLE

#include "FreeRTOS/FreeRTOS.h"


#define PTHREAD_COND_INITIALIZER    FREERTOS_POSIX_COND_INITIALIZER       /**< pthread_cond_t. */

#define PTHREAD_MUTEX_INITIALIZER    FREERTOS_POSIX_MUTEX_INITIALIZER /**< pthread_mutex_t. */

#define pdFALSE			( ( BaseType_t ) 0 )

#define FREERTOS_POSIX_COND_INITIALIZER \
    ( ( ( pthread_cond_internal_t )         \
    {                                       \
        .xIsInitialized = pdFALSE,          \
        .xCondWaitSemaphore = { { 0 } },    \
        .iWaitingThreads = 0                \
    }                                       \
        )                                   \
    )

#define FREERTOS_POSIX_MUTEX_INITIALIZER \
    ( ( ( pthread_mutex_internal_t )         \
    {                                        \
        .xIsInitialized = pdFALSE,           \
        .xMutex = { { 0 } },                 \
        .xTaskOwner = NULL,                  \
        .xAttr = { .iType = 0 }              \
    }                                        \
        )                                    \
    )

typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

typedef StaticQueue_t StaticSemaphore_t;

typedef struct {
    StaticSemaphore_t xSemaphore;
    int value;
} sem_internal_t;

typedef sem_internal_t PosixSemType_t;

typedef PosixSemType_t sem_t;

typedef void *TaskHandle_t;

typedef struct xSTATIC_LIST_ITEM StaticListItem_t;

typedef struct pthread_mutexattr_internal {
    int iType;
} pthread_mutexattr_internal_t;

typedef StaticQueue_t StaticSemaphore_t;

typedef struct pthread_mutex_internal {
    BaseType_t xIsInitialized;
    StaticSemaphore_t xMutex;
    TaskHandle_t xTaskOwner;
    pthread_mutexattr_internal_t xAttr;
} pthread_mutex_internal_t;

typedef struct pthread_cond_internal {
    BaseType_t xIsInitialized;
    StaticSemaphore_t xCondWaitSemaphore;
    unsigned iWaitingThreads;
} pthread_cond_internal_t;

typedef struct pthread_barrier_internal {
    unsigned uThreadCount;
    unsigned uThreshold;
    StaticSemaphore_t xThreadCountSemaphore;
    StaticEventGroup_t xBarrierEventGroup;
} pthread_barrier_internal_t;

typedef pthread_cond_internal_t PthreadCondType_t;
typedef pthread_mutex_internal_t PthreadMutexType_t;
typedef struct pthread_mutexattr {
    uint32_t ulpthreadMutexAttrStorage;
} PthreadMutexAttrType_t;

typedef PthreadCondType_t pthread_cond_t;
typedef void *pthread_condattr_t;
typedef PthreadMutexType_t pthread_mutex_t;
typedef PthreadMutexAttrType_t pthread_mutexattr_t;

typedef struct pthread_attr {
    uint32_t ulpthreadAttrStorage;
} PthreadAttrType_t;

typedef PthreadAttrType_t pthread_attr_t;

typedef pthread_barrier_internal_t PthreadBarrierType_t;
typedef PthreadBarrierType_t pthread_barrier_t;

typedef void *pthread_barrierattr_t;
typedef void *pthread_t;

#else
#error "no_os_defined"
#endif

#endif // __SIMPLE_PTHREAD_COMM_H__
