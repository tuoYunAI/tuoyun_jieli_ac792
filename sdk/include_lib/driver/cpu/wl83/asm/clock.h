#ifndef _CPU_CLOCK_
#define _CPU_CLOCK_

#include "generic/typedef.h"

enum CLK_OUT0_SOURCE {
    CLK_OUT0_DISABLE,
    CLK_OUT0_RC16M,
    CLK_OUT0_LRC_200K,
    CLK_OUT0_STD_12M,
    CLK_OUT0_STD_24M,
    CLK_OUT0_STD_48M,
    CLK_OUT0_PLL_96M,
    CLK_OUT0_HSB,
    CLK_OUT0_LSB,
    CLK_OUT0_RC250K,
    CLK_OUT0_RTC_OSC,
    CLK_OUT0_XOSC_24M,
};
enum CLK_OUT1_SOURCE {
    CLK_OUT1_DISABLE,
    CLK_OUT1_RC16M,
    CLK_OUT1_LRC_200K,
    CLK_OUT1_STD_12M,
    CLK_OUT1_STD_24M,
    CLK_OUT1_STD_48M,
    CLK_OUT1_PLL_96M,
    CLK_OUT1_HSB,
    CLK_OUT1_LSB,
    CLK_OUT1_RC250K,
    CLK_OUT1_RTC_OSC,
    CLK_OUT1_XOSC_24M,
};
enum CLK_OUT2_SOURCE {
    CLK_OUT2_DISABLE,
    CLK_OUT2_HCO,
    CLK_OUT2_HSB,
    CLK_OUT2_PLL_480M,
    CLK_OUT2_PLL_192M,
    CLK_OUT2_SYSPLL_D1P0,
    CLK_OUT2_SYSPLL_D1P5,
    CLK_OUT2_USBPLL_D1P0,
    CLK_OUT2_USBPLL_D2P5,
    CLK_OUT2_WLPLL_D1P0,
    CLK_OUT2_WLPLL_D1P5,
    CLK_OUT2_WLPLL_D2P5,
    CLK_OUT2_WLPLL_D3P5,
    CLK_OUT2_WLPLL_D4P5,
    CLK_OUT2_DDRPLL_D1P0,
    CLK_OUT2_MIPIPLL_D1P0,
};
enum CLK_OUT2_DIV {
    CLK_OUT2_DIV1 = 0,
    CLK_OUT2_DIV3 = 1,
    CLK_OUT2_DIV5 = 2,
    CLK_OUT2_DIV7 = 3,
    CLK_OUT2_DIV2 = 1 << 2,
    CLK_OUT2_DIV4 = 2 << 2,
    CLK_OUT2_DIV8 = 3 << 2,
};
enum CLK_OUT3_SOURCE {
    CLK_OUT3_DISABLE,
    CLK_OUT3_HCO,
    CLK_OUT3_HSB,
    CLK_OUT3_PLL_480M,
    CLK_OUT3_PLL_192M,
    CLK_OUT3_SYSPLL_D1P0,
    CLK_OUT3_SYSPLL_D1P5,
    CLK_OUT3_USBPLL_D1P0,
    CLK_OUT3_USBPLL_D2P5,
    CLK_OUT3_WLPLL_D1P0,
    CLK_OUT3_WLPLL_D1P5,
    CLK_OUT3_WLPLL_D2P5,
    CLK_OUT3_WLPLL_D3P5,
    CLK_OUT3_WLPLL_D4P5,
    CLK_OUT3_DDRPLL_D1P0,
    CLK_OUT3_MIPIPLL_D1P0,
};
enum CLK_OUT3_DIV {
    CLK_OUT3_DIV1 = 0,
    CLK_OUT3_DIV3 = 1,
    CLK_OUT3_DIV5 = 2,
    CLK_OUT3_DIV7 = 3,
    CLK_OUT3_DIV2 = 1 << 2,
    CLK_OUT3_DIV4 = 2 << 2,
    CLK_OUT3_DIV8 = 3 << 2,
};

/*
 * system enter critical and exit critical handle
 * */
struct clock_critical_handler {
    void (*enter)();
    void (*exit)();
};

#define CLOCK_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler clock_##name \
		 SEC_USED(.clock_critical_txt) = {enter, exit};

extern struct clock_critical_handler clock_critical_handler_begin[];
extern struct clock_critical_handler clock_critical_handler_end[];

#define list_for_each_loop_clock_critical(h) \
	for (h=clock_critical_handler_begin; h<clock_critical_handler_end; h++)

void clk_dvdd_vol_lev_update(void);

int clk_early_init(void);

void video_clock_early_init(u32 clk);

void video_clock_early_uninit(void);

void jlgpu_clock_early_init(u32 clk);

void jlgpu_clock_early_uninit(void);

void audio_clock_early_init(void);

void wl_clock_early_init(void);

int clk_get(const char *name);

//name : hsb lsb sys ddr sdram sfc
int clk_set(const char *name, int clk);

int sys_clk_get(void);

void clk_out0(u32 gpio, enum CLK_OUT0_SOURCE index);
void clk_out1(u32 gpio, enum CLK_OUT1_SOURCE index);
void clk_out2(u32 gpio, enum CLK_OUT2_SOURCE index, enum CLK_OUT2_DIV div);
void clk_out_disable(u32 gpio, u8 clock_out_ch);

u32 clk_get_osc_cap(void);

#endif

