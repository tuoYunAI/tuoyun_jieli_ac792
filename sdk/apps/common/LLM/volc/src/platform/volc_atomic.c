#if 0
#include "volc_atomic.h"
#include "os/os_api.h"
#include "volc_type.h"

void volc_atomic_store(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    // __atomic_store_n(p_atomic, val, __ATOMIC_SEQ_CST);
    __atomic_store_n(p_atomic, val, __ATOMIC_SEQ_CST);
}

bool volc_atomic_compare_exchange(volatile size_t *p_atomic, size_t *p_expected, size_t desired)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_compare_exchange_n(p_atomic, p_expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    size_t expected = *p_expected;
    bool success = __atomic_compare_exchange_n(
                       p_atomic, &expected, desired,
                       0, // weak参数，0表示strong
                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
                   );
    if (!success) {
        *p_expected = expected;
    }
    return success;
}

size_t volc_atomic_load(volatile size_t *p_atomic)
{
    // volc_debug("%s\n", __FUNCTION__);
//    return atomic_read(p_
    return __atomic_load_n(p_atomic, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_exchange(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    return __atomic_exchange_n(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_increment(volatile size_t *p_atomic)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_fetch_add(p_atomic, 1, __ATOMIC_SEQ_CST);
    return __atomic_add_fetch(p_atomic, 1, __ATOMIC_SEQ_CST);
//    return atomic_inc(p_atomic);
}

size_t volc_atomic_decrement(volatile size_t *p_atomic)
{
    // volc_debug("%s\n", __FUNCTION__);
//    return atomic_dec(p_atomic);
    // return __atomic_fetch_sub(p_atomic, 1, __ATOMIC_SEQ_CST);
    return __atomic_sub_fetch(p_atomic, 1, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_add(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
//    return atomic_add())
    // return __atomic_fetch_add(p_atomic, val, __ATOMIC_SEQ_CST);
    return __atomic_add_fetch(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_sub(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_fetch_sub(p_atomic, val, __ATOMIC_SEQ_CST);
    return __atomic_sub_fetch(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_and(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_fetch_and(p_atomic, val, __ATOMIC_SEQ_CST);
    return __atomic_and_fetch(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_or(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_fetch_or(p_atomic, val, __ATOMIC_SEQ_CST);
    return __atomic_or_fetch(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_xor(volatile size_t *p_atomic, size_t val)
{
    // volc_debug("%s\n", __FUNCTION__);
    // return __atomic_fetch_xor(p_atomic, val, __ATOMIC_SEQ_CST);
    return __atomic_xor_fetch(p_atomic, val, __ATOMIC_SEQ_CST);
}
#endif

#include "volc_atomic.h"
#include "os/os_api.h"
#include "volc_type.h"

void volc_atomic_store(volatile size_t *p_atomic, size_t val)
{
    *p_atomic = val;
}

bool volc_atomic_compare_exchange(volatile size_t *p_atomic, size_t *p_expected, size_t desired)
{
    return __atomic_compare_exchange_n(p_atomic, p_expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_load(volatile size_t *p_atomic)
{
    return *p_atomic;
}

size_t volc_atomic_exchange(volatile size_t *p_atomic, size_t val)
{
    return __atomic_exchange_n(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_increment(volatile size_t *p_atomic)
{
    *p_atomic += 1;
    return *p_atomic;
}

size_t volc_atomic_decrement(volatile size_t *p_atomic)
{
    *p_atomic -= 1;
    return *p_atomic;
}

size_t volc_atomic_add(volatile size_t *p_atomic, size_t val)
{
    *p_atomic += val;
    return *p_atomic;
}

size_t volc_atomic_sub(volatile size_t *p_atomic, size_t val)
{
    *p_atomic -= val;
    return *p_atomic;
}

size_t volc_atomic_and(volatile size_t *p_atomic, size_t val)
{
    return __atomic_fetch_and(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_or(volatile size_t *p_atomic, size_t val)
{
    return __atomic_fetch_or(p_atomic, val, __ATOMIC_SEQ_CST);
}

size_t volc_atomic_xor(volatile size_t *p_atomic, size_t val)
{
    return __atomic_fetch_xor(p_atomic, val, __ATOMIC_SEQ_CST);
}

