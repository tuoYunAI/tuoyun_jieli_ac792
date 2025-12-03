#include "app_config.h"
#include "generic/typedef.h"
#include "system/malloc.h"
#include "system/init.h"

#if defined CONFIG_MMU_ENABLE && defined CONFIG_NO_SDRAM_ENABLE

void *malloc(unsigned long size)
{
    return mmu_malloc(size);
}

void *calloc(unsigned long count, unsigned long size)
{
    return mmu_calloc(count, size);
}

void *zalloc(unsigned long size)
{
    return mmu_calloc(1, size);
}

void *realloc(void *rmem, unsigned long newsize)
{
    return mmu_realloc(rmem, newsize);
}

void free(void *p)
{
    mmu_free(p);
}

void *pvPortMalloc(unsigned long size)
{
    return mmu_malloc(size);
}

void vPortFree(void *pv)
{
    mmu_free(pv);
}

void *kmalloc(unsigned long size, int flags)
{
    return mmu_malloc(size);
}

void *vmalloc(unsigned long size)
{
    return mmu_malloc(size);
}

void vfree(void *addr)
{
    mmu_free(addr);
}

void *kzalloc(unsigned int len, int a)
{
    return mmu_calloc(1, len);
}

void kfree(void *p)
{
    mmu_free(p);
}

void memory_init(void)
{
    mmu_memory_init();
}

void malloc_dump(void)
{

}

void malloc_stats(void)
{
    mmu_stats();
}

#endif



#if defined CONFIG_MMU_ENABLE

void *ram_malloc(unsigned long size)
{
    return mmu_malloc(size);
}

void ram_free(void *rmem)
{
    mmu_free(rmem);
}

void *ram_calloc(unsigned long count, unsigned long size)
{
    return mmu_calloc(count, size);
}

void *ram_realloc(void *rmem, unsigned long newsize)
{
    return mmu_realloc(rmem, newsize);
}

void *ram_zalloc(unsigned long size)
{
    return mmu_calloc(1, size);
}

void ram_malloc_stats(void)
{
    mmu_stats();
}

#if !defined CONFIG_NO_SDRAM_ENABLE
static int ram_memory_init(void)
{
    mmu_memory_init();
    return 0;
}
early_initcall(ram_memory_init);
#endif

#endif



#if !defined CONFIG_MMU_ENABLE && defined CONFIG_NO_SDRAM_ENABLE

void *ram_malloc(unsigned long size)
{
    return malloc(size);
}

void ram_free(void *rmem)
{
    free(rmem);
}

void *ram_calloc(unsigned long count, unsigned long size)
{
    return calloc(count, size);
}

void *ram_realloc(void *rmem, unsigned long newsize)
{
    return realloc(rmem, newsize);
}

void *ram_zalloc(unsigned long size)
{
    return zalloc(size);
}

void ram_malloc_stats(void)
{
    malloc_stats();
}

#endif


u32 get_memory_heap_total_size(void)
{
    extern u32 HEAP_BEGIN;
    extern u32 HEAP_END;
    return (u32)&HEAP_END - (u32)&HEAP_BEGIN;
}
