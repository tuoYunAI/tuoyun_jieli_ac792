#ifndef __SIMPLE_PTHREAD_H__
#define __SIMPLE_PTHREAD_H__

#include "simple_pthread_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                int *detachstate);
int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param);
int pthread_attr_getstacksize(const pthread_attr_t *attr,
                              size_t *stacksize);
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr,
                                int detachstate);
int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param);
int pthread_attr_setschedpolicy(pthread_attr_t *attr,
                                int policy);
int pthread_attr_setstacksize(pthread_attr_t *attr,
                              size_t stacksize);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_init(pthread_barrier_t *barrier,
                         const pthread_barrierattr_t *attr,
                         unsigned count);
int pthread_barrier_wait(pthread_barrier_t *barrier);
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*startroutine)(void *),
                   void *arg);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_timedwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           const struct timespec *abstime);
int pthread_cond_wait(pthread_cond_t *cond,
                      pthread_mutex_t *mutex);
int pthread_equal(pthread_t t1,
                  pthread_t t2);
void pthread_exit(void *value_ptr);
int pthread_getschedparam(pthread_t thread,
                          int *policy,
                          struct sched_param *param);
int pthread_join(pthread_t thread,
                 void **retval);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,
                              int *type);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr,
                              int type);
pthread_t pthread_self(void);
int pthread_setschedparam(pthread_t thread,
                          int policy,
                          const struct sched_param *param);
int nanosleep(const struct timespec *req, struct timespec *rem);
int sched_yield(void);
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
int pthread_once(pthread_once_t *once_control,
                 void (*init_routine)(void));
int pthread_detach(pthread_t thread);

#ifdef __cplusplus
}
#endif

#endif // __SIMPLE_PTHREAD_H__

