#include "app_config.h"
#include "asm/includes.h"
#include "system/includes.h"

static void video_eva_xbus_init(void)
{
    eva_xbus_seg = 0x2;
    eva_xbus_rsel |= BIT(rid_dmm) | BIT(rid_rep_ch0); //dmm独占eva xbus通道1
    eva_xbus_wsel |= BIT(wid_imc_ch1) | BIT(wid_imc_ch0); //dmm独占eva xbus通道1
}

void setup_arch(void)
{
#if 0
    JL_PORTA->DIR = 0;
    JL_PORTB->DIR = 0;
    JL_PORTC->DIR = 0;
    JL_PORTD->DIR = 0;
    JL_PORTE->DIR = 0;
    while (1) {
        JL_PORTB->OUT ^= 0XFFFF;
        JL_PORTC->OUT ^= 0XFFFF;
        JL_PORTA->OUT ^= 0XFFFF;
        JL_PORTD->OUT ^= 0XFFFF;
        JL_PORTE->OUT ^= 0XFFFF;
    }
#endif

    /* wdt_close(); */
    wdt_init(0xc);

    clk_early_init();

    interrupt_init();

#ifdef CONFIG_DEBUG_ENABLE
    extern void debug_uart_init();
    debug_uart_init();
#ifdef __LOG_ENABLE
    log_early_init(10 * 1024);
#endif
#if (defined(TCFG_DEBUG_DLOG_ENABLE) && TCFG_DEBUG_DLOG_ENABLE)
    //dlog提前初始化
    dlog_early_init();
    dlog_enable(1);
    dlog_output_type_set(TCFG_DLOG_OUTPUT_TYPE);
    if (dlog_output_type_get() & DLOG_OUTPUT_2_UART) {
        void dlog_uart_en_switch(u8 enable);
        dlog_uart_en_switch(1);
    }
    // 设置log的等级
    /* dlog_level_set(__LOG_INFO); */
#endif
#endif

    //让cpu0去初始化cpu1的icache, 或者run_cpu1 那边加, 否则CPU1跑不出来,BR27是把 cpu1_start 放内部ram, 但是WL83 icache代码没放mask, 所以浪费内部ram就算了
    IcuInitial(1);

    extern void code_movable_init(void);
    code_movable_init();

    power_early_flowing();

#if defined CONFIG_NET_ENABLE || defined CONFIG_BT_ENABLE
    wl_clock_early_init();
#endif

#if defined CONFIG_AUDIO_ENABLE
    audio_clock_early_init();
#endif

#if defined CONFIG_VIDEO_ENABLE || defined CONFIG_UI_ENABLE
    video_clock_early_init(TCFG_VIDEO_CLK);
    video_eva_xbus_init();
    jlgpu_clock_early_init(TCFG_GPU_CLK);
    clk_dvdd_vol_lev_update();
#endif

    debug_init();

    puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    printf("\n   WL83(AC792N) setup_arch %s %s", __DATE__, __TIME__);
    extern u32 SDRAM_SIZE, RAM0_SIZE, DCACHE_RAM_SIZE, VIDEO_RAM_SIZE, text_size, data_size, bss_size, ram0_bss_size, ram0_data_size, dcache_ram_bss_size, dcache_ram_data_size, video_ram_bss_size, video_ram_data_size, HEAP_END, HEAP_BEGIN;
    printf("\nsys_clk = %d, %s_clk = %d, hsb_clk = %d, lsb_clk = %d, sfc_clk = %d, eva_clk = %d gpu_clk = %d", clk_get("sys"), (JL_DMC->SDR_CON0 & BIT(27)) ? "ddr" : "sdram", clk_get("sdram"), clk_get("hsb"), clk_get("timer"), clk_get("sfc"), clk_get("eva"), clk_get("gpu"));
    printf("\nCODE+CONST SIZE = %d", (u32)&text_size);
    printf("\n%s_SIZE = %d, DATA_SIZE = %d, BSS_SIZE = %d, REMAIN_SIZE = %d", (JL_DMC->SDR_CON0 & BIT(27)) ? "DDR" : "SDRAM", (u32)&SDRAM_SIZE, (u32)&data_size, (u32)&bss_size, (u32)&SDRAM_SIZE - (u32)&data_size - (u32)&bss_size);
    printf("\nRAM_SIZE = %d, RAM_DATA_SIZE = %d, RAM_BSS_SIZE = %d, RAM_REMAIN_SIZE = %d", (u32)&RAM0_SIZE, (u32)&ram0_data_size, (u32)&ram0_bss_size, (u32)&RAM0_SIZE - (u32)&ram0_data_size - (u32)&ram0_bss_size);
    printf("\nDCACHE_RAM_SIZE = %d, DCACHE_RAM_DATA_SIZE = %d, DCACHE_RAM_BSS_SIZE = %d, DCACHE_RAM_REMAIN_SIZE = %d", (u32)&DCACHE_RAM_SIZE, (u32)&dcache_ram_data_size, (u32)&dcache_ram_bss_size, (u32)&DCACHE_RAM_SIZE - (u32)&dcache_ram_data_size - (u32)&dcache_ram_bss_size);
    printf("\nVIDEO_RAM_SIZE = %d, VIDEO_RAM_DATA_SIZE = %d, VIDEO_RAM_BSS_SIZE = %d, VIDEO_RAM_REMAIN_SIZE = %d", (u32)&VIDEO_RAM_SIZE, (u32)&video_ram_data_size, (u32)&video_ram_bss_size, (u32)&VIDEO_RAM_SIZE - (u32)&video_ram_data_size - (u32)&video_ram_bss_size);
    printf("\nHEAP_SIZE = %d", (u32)&HEAP_END - (u32)&HEAP_BEGIN);
    puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    __crc16_mutex_init();
}

