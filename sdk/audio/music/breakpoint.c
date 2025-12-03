#ifdef MEDIA_SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".breakpoint.data.bss")
#pragma data_seg(".breakpoint.data")
#pragma const_seg(".breakpoint.text.const")
#pragma code_seg(".breakpoint.text")
#endif
#include "breakpoint.h"
#include "app_config.h"
#include "syscfg/syscfg_id.h"

#if (defined(TCFG_DEC_APE_ENABLE) && (TCFG_DEC_APE_ENABLE))
#define BREAKPOINT_DATA_LEN   (2036 + 4)
#elif (defined(TCFG_DEC_FLAC_ENABLE) && (TCFG_DEC_FLAC_ENABLE))
#define BREAKPOINT_DATA_LEN	688
#elif (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
#define BREAKPOINT_DATA_LEN	536
#endif

#ifndef BREAKPOINT_DATA_LEN
#define BREAKPOINT_DATA_LEN	32
#endif

#define BREAKPOINT_VAILD_SIZE(p)		((p)->dbp.len + ((u32)(p)->dbp.data - (u32)(p)))

breakpoint_t *breakpoint_handle_creat(void)
{
    breakpoint_t *bp = (breakpoint_t *)zalloc(sizeof(breakpoint_t) + BREAKPOINT_DATA_LEN);
    bp->dbp.data_len = BREAKPOINT_DATA_LEN;
    return bp;
}

void breakpoint_handle_destroy(breakpoint_t **bp)
{
    if (bp && *bp) {
        free(*bp);
        *bp = NULL;
    }
}

bool breakpoint_vm_read(breakpoint_t *bp, const char *logo)
{
    if (bp == NULL || logo == NULL) {
        return false;
    }

    u16 vm_id;

    if (!strcmp(logo, "sd0")) {
        vm_id = VM_SD0_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "sd1")) {
        vm_id = VM_SD1_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "udisk0")) {
        vm_id = VM_USB0_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "udisk1")) {
        vm_id = VM_USB1_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "sdfile")) {
        vm_id = VM_FLASH_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "net")) {
        vm_id = VM_NET_BREAKPOINT_INDEX;
    } else {
        return false;
    }

    int rlen = syscfg_read(vm_id, (void *)bp, sizeof(breakpoint_t) + BREAKPOINT_DATA_LEN);
    if (rlen < 0) {
        memset(bp, 0, sizeof(breakpoint_t) + BREAKPOINT_DATA_LEN);
        bp->dbp.data_len = BREAKPOINT_DATA_LEN;
        return false;
    } else {
        /* put_buf((u8*)bp,36); */
    }

    return true;
}

void breakpoint_vm_write(breakpoint_t *bp, const char *logo)
{
    if (bp == NULL || logo == NULL) {
        return;
    }

    u16 vm_id;

    if (!strcmp(logo, "sd0")) {
        vm_id = VM_SD0_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "sd1")) {
        vm_id = VM_SD1_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "udisk0")) {
        vm_id = VM_USB0_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "udisk1")) {
        vm_id = VM_USB1_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "sdfile")) {
        vm_id = VM_FLASH_BREAKPOINT_INDEX;
    } else if (!strcmp(logo, "net")) {
        vm_id = VM_NET_BREAKPOINT_INDEX;
    } else {
        return;
    }

    syscfg_write(vm_id, (void *)bp, BREAKPOINT_VAILD_SIZE(bp));
}

