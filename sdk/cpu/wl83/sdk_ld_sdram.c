// *INDENT-OFF*

EXTERN(
_start
#include "sdk_used_list.c"
);

#include "app_config.h"

BOOT_INFO_SIZE = 60;

#if defined CONFIG_MMU_ENABLE
TLB_SIZE = 0X1000;
MMU_TAG_ADDR = 0x100000;
#else
TLB_SIZE = 0;
MMU_TAG_ADDR = 0;
#endif

#ifdef CONFIG_SFC_ENABLE
SDRAM_ORIGIN = 0x18000000;
SDRAM_SIZE = __SDRAM_SIZE__;
#else
SDRAM_ORIGIN = 0x18000120;
SDRAM_SIZE = __SDRAM_SIZE__ - 0x120;
#endif
#ifdef CONFIG_VIDEO_ENABLE
VIDEO_RAM_SIZE = 0;
#else
VIDEO_RAM_SIZE = 128K;
#endif
RAM0_SIZE = 0x11fe00 - 0x100000 - TLB_SIZE - BOOT_INFO_SIZE - 128;
UPDATA_BEG = 0x11fe00 - 128;
CODE_BEG = 0x8000120;

FREE_DCACHE_WAY = 0; //max is 7
FREE_ICACHE0_WAY = 0; //max is 7
FREE_ICACHE1_WAY = 0; //max is 7

DCACHE_RAM_SIZE = FREE_DCACHE_WAY*4K;
ICACHE0_RAM_SIZE = FREE_ICACHE0_WAY*4K;
ICACHE1_RAM_SIZE = FREE_ICACHE1_WAY*4K;

MEMORY
{
    rom(rx)             : ORIGIN =  CODE_BEG, LENGTH = __FLASH_SIZE__
    sdram(rwx)          : ORIGIN =  SDRAM_ORIGIN, LENGTH = SDRAM_SIZE
    ram0(rwx)           : ORIGIN =  0x100000 + TLB_SIZE, LENGTH = RAM0_SIZE
    boot_info(rwx)      : ORIGIN =  0x100000 + TLB_SIZE + RAM0_SIZE, LENGTH = BOOT_INFO_SIZE
    dcache_ram(rw)      : ORIGIN =  0x378000-((FREE_DCACHE_WAY)*4K), LENGTH = FREE_DCACHE_WAY*4K
    icache0_ram(rw)     : ORIGIN =  0x3c8000-((FREE_ICACHE0_WAY)*4K), LENGTH = FREE_ICACHE0_WAY*4K
    icache1_ram(rw)     : ORIGIN =  0x3d8000-((FREE_ICACHE1_WAY)*4K), LENGTH = FREE_ICACHE1_WAY*4K
    video_ram(rwx)      : ORIGIN =  0x7f00000, LENGTH = VIDEO_RAM_SIZE

    dlog0(r)     	  : ORIGIN = 0x20000000, LENGTH = 0x100000
}

SECTIONS
{
#ifdef CONFIG_SFC_ENABLE
    . = ORIGIN(rom);
#else
    . = ORIGIN(sdram);
#endif
    .text ALIGN(4):
    {
        _rom_text_code_begin = .;
        PROVIDE(rom_text_code_begin = .);

        *startup.S.o(.text)
        *(.boot_code)
#if defined CONFIG_TRUSTZONE_ENABLE
        . = ALIGN(8);
        _TRUSTZONE_CODE_START = .;
        *(.trustzone)
        . = ALIGN(8);
        _TRUSTZONE_CODE_END = .;
#endif
#if !defined CONFIG_SFC_ENABLE
        *(.dlmalloc_code)
        *(.mem_heap_code)
        *(.os_code)
        *(.os_port_code)
#endif
        #include "system/system_lib_text.ld"
        #include "common/movable/movable_text.ld"
#if defined CONFIG_AUDIO_ENABLE
        #include "server/media_text.ld"
#endif
#if defined CONFIG_BT_ENABLE
        #include "btctrler/btctler_lib_text.ld"
        #include "btstack/btstack_lib_text.ld"
#endif
#if !defined CONFIG_RF_TRIM_CODE_AT_RAM && !defined CONFIG_RF_TRIM_CODE_MOVABLE
        *(*.rf.text.cache.L2.rf_trim)
#endif
        #include "driver/cpu/wl83/asm/power/ld/power_driver.ld"
#if defined CONFIG_MEDIA_ENABLE
        #include "media/media_lib_text.ld"
        #include "media/framework/section_text.ld"
        *(.*.cache.L1)
        *(.*.cache.L2.run)
#endif
        *(.*.cache.L2.code)
        *(.*.text)
        *(.text*)

        . = ALIGN(4);
        _rom_text_code_end = .;
        PROVIDE(rom_text_code_end = .);

        *(.*.text.const)
        *(.*.cache.L2.const)
        *(.rodata*)

        #include "system/ui/ui.ld"
        . = ALIGN(4);
        #include "system/system.ld"
        . = ALIGN(4);
        #include "server/media.ld"

        . = ALIGN(32);
    }

#ifdef CONFIG_SFC_ENABLE
	>rom
    . = ORIGIN(sdram);
#else
	> sdram
#endif

    .data ALIGN(32):
    {
        _sdram_text_code_begin = .;
        PROVIDE(sdram_text_code_begin = .);
#if defined CONFIG_SFC_ENABLE
        *(.dlmalloc_code)
        *(.mem_heap_code)
        *(.os_code)
        *(.os_port_code)
#if defined CONFIG_RF_TRIM_CODE_AT_RAM
        *(*.rf.text.cache.L2.rf_trim)
#endif
#if defined CONFIG_MPU_DEBUG_ENABLE
        . = ALIGN(8192);
#else
        . = ALIGN(4);
#endif
#endif
        _sdram_text_code_end = .;
        PROVIDE(sdram_text_code_end = .);

        . = ALIGN(4);
        #include "system/system_data.ld"
        . = ALIGN(4);
        #include "net/server/net_server_data.ld"
#if defined CONFIG_BT_ENABLE
        . = ALIGN(4);
        #include "btctrler/btctler_lib_data.ld"
        . = ALIGN(4);
        #include "btstack/btstack_lib_data.ld"
#endif
        *(.dlmalloc_const)
        *(.mem_heap_const)
        *(.os_const)
        *(.os_str)
        *(.os_data)
        *(.os_port_data)
        *(.os_port_const)
#if defined CONFIG_DNS_ENC_ENABLE
        . = ALIGN(4);
        *(.jlsp_data)
#endif
#if defined CONFIG_MEDIA_ENABLE
        #include "media/media_lib_data.ld"
#endif
        . = ALIGN(4);
        *(.debug_exception.debug_text)
        *(.debug_exception.debug_const)
        *(.debug_exception.data)

        . = ALIGN(4);
        *(.data)

        . = ALIGN(4);
    } > sdram

    .bss ALIGN(32) (NOLOAD):
    {
#if defined CONFIG_BT_ENABLE
        #include "btctrler/btctler_lib_bss.ld"
        #include "btstack/btstack_lib_bss.ld"
#endif
#if defined CONFIG_MEDIA_ENABLE
        #include "media/media_lib_bss.ld"
#endif

        . = ALIGN(4);
        *(.debug_exception.data.bss)

        *(.bss)
#if defined CONFIG_CXX_SUPPORT
        *(.bss.*)
#endif
        *(COMMON)
        *(.mem_heap)
        *(.memp_memory_x)

        . = ALIGN(4);
        *(.os_port_bss)
        *(.os_bss)

        . = ALIGN(4);
    } > sdram

    _HEAP_BEGIN = ( . + 31 ) / 32 * 32 ;
    PROVIDE(HEAP_BEGIN = (. + 31) / 32 * 32);
    _HEAP_END = SDRAM_ORIGIN + SDRAM_SIZE - 32;
    PROVIDE(HEAP_END = SDRAM_ORIGIN + SDRAM_SIZE - 32);
    _MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
    PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);


    . =ORIGIN(ram0);
    .ram0_data ALIGN(4):
    {
        _ram_text_code_begin = .;
        PROVIDE(ram_text_code_begin = .);
        . = ALIGN(4);
        _SPI_CODE_START = .;
        *(.spi_code)
        . = ALIGN(4);
        _SPI_CODE_END = .;

        *(.cache_code)
        *(.volatile_ram_code)
        *(.volatile_ram_power)
        . = ALIGN(4);
        _ram_text_code_end = .;
        PROVIDE(ram_text_code_end = .);

        *(.volatile_ram)
        *(.non_volatile_ram)
        *(.fft_data)

        . = ALIGN(4);
        *(.power_driver.data)

        #include "common/movable/movable_data.ld"

        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > ram0

    .ram0_bss ALIGN(4):
    {
        . = ALIGN(4);
        *(.the_debug_isr_stack_c0)
        *(.the_debug_isr_stack_c1)
        *(.stack_magic)
        *(.stack_magic0)

        . = ALIGN(4);
        _stack_info_begin = .;
        PROVIDE(stack_info_begin = .);
        *(.cpu0_ustack)
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.cpu0_sstack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
        *(.cpu1_ustack)
        _cpu1_sstack_begin = .;
        PROVIDE(cpu1_sstack_begin = .);
        *(.cpu1_sstack)
        _cpu1_sstack_end = .;
        PROVIDE(cpu1_sstack_end = .);
        _stack_info_end = .;
        PROVIDE(stack_info_end = .);

        . = ALIGN(4);
        *(.power_driver.data.bss)

#if defined CONFIG_BT_ENABLE
        *(.bt_dma)
#endif
        *(.sram)

        . = ALIGN(4);
        #include "device/usb/usb_data.ld"

        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > ram0

    _RAM_HEAP_BEGIN = ( . + 31 ) / 32 * 32;
    PROVIDE(RAM_HEAP_BEGIN = (. + 31) / 32 * 32);
    _RAM_HEAP_END = 0x100000 + TLB_SIZE + RAM0_SIZE - 32;
    PROVIDE(RAM_HEAP_END = 0x100000 + TLB_SIZE + RAM0_SIZE - 32);
    _RAM_MALLOC_SIZE = _RAM_HEAP_END - _RAM_HEAP_BEGIN;
    PROVIDE(RAM_MALLOC_SIZE = _RAM_HEAP_END - _RAM_HEAP_BEGIN);


    . = ORIGIN(boot_info);
    .boot_info ALIGN(4):
    {
        *(.boot_info)
        *(.boot_info_dma)
        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > boot_info


    . = ORIGIN(dlog0);
    /* .dlog_data (NOLOAD):SUBALIGN(4) */
    .dlog_data :SUBALIGN(4)
    {
        dlog_seg_begin = .;
        // 头部的0x100空间
        KEEP(*(.dlog.rodata.head))
        /* . = 0x100 + ORIGIN(dlog0); */
        . = ALIGN(0x100);

        dlog_str_tab_seg_begin = .;
        dlog_str_tab_seg0_begin = .;
        *(.dlog.rodata.str_tab.0)

        dlog_str_tab_seg1_begin = .;
        *(.dlog.rodata.str_tab.1)

        dlog_str_tab_seg2_begin = .;
        *(.dlog.rodata.str_tab.2)

        dlog_str_tab_seg3_begin = .;
        *(.dlog.rodata.str_tab.3)

        dlog_str_tab_seg4_begin = .;
        *(.dlog.rodata.str_tab.4)

        dlog_str_tab_seg5_begin = .;
        *(.dlog.rodata.str_tab.5)

        dlog_str_tab_seg6_begin = .;
        *(.dlog.rodata.str_tab.6)

        dlog_str_tab_seg7_begin = .;
        *(.dlog.rodata.str_tab.7)

        dlog_str_tab_segAll_begin = .;
        *(.dlog.rodata.str_tab*)
        dlog_str_tab_segAll_end = .;
        dlog_str_tab_seg_end = .;

        . = ALIGN(32);
        dlog_str_seg_begin = .;
        dlog_str_seg0_begin = .;
        *(.dlog.rodata.string.0)

        dlog_str_seg1_begin = .;
        *(.dlog.rodata.string.1)

        dlog_str_seg2_begin = .;
        *(.dlog.rodata.string.2)

        dlog_str_seg3_begin = .;
        *(.dlog.rodata.string.3)

        dlog_str_seg4_begin = .;
        *(.dlog.rodata.string.4)

        dlog_str_seg5_begin = .;
        *(.dlog.rodata.string.5)

        dlog_str_seg6_begin = .;
        *(.dlog.rodata.string.6)

        dlog_str_seg7_begin = .;
        *(.dlog.rodata.string.7)

        dlog_str_segAll_begin = .;
        *(.dlog.rodata.string*)
        dlog_str_segAll_end = .;
        dlog_str_seg_end = .;

        dlog_seg_end = .;
    } > dlog0


/********************************************/
    //dcache_ram由于设计时CPU0访问优先级总比CPU1高，导致双核不能当代码区会导致内核死锁，只能用来做数据存储
    . = ORIGIN(dcache_ram);
    .dcache_ram_data ALIGN(4):
    {
        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > dcache_ram

    .dcache_ram_bss ALIGN(4) (NOLOAD):
    {
        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > dcache_ram


    . =ORIGIN(video_ram);
    .video_ram_data ALIGN(4):
    {
        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > video_ram

    .video_ram_bss ALIGN(4) (NOLOAD):
    {
        . = ALIGN(4); // must at tail, make ram0_data size align 4
    } > video_ram
}

    text_begin = ADDR(.text);
    text_size = SIZEOF(.text);
    PROVIDE(text_size = SIZEOF(.text));

    bss_begin = ADDR(.bss);
    bss_size  = SIZEOF(.bss);
    PROVIDE(bss_size = SIZEOF(.bss));

    _ram0_bss_vma = ADDR(.ram0_bss);
    _ram0_bss_size = SIZEOF(.ram0_bss);
    PROVIDE(ram0_bss_size = SIZEOF(.ram0_bss));

    _dcache_ram_bss_vma = ADDR(.dcache_ram_bss);
    _dcache_ram_bss_size = SIZEOF(.dcache_ram_bss);
    PROVIDE(dcache_ram_bss_size = SIZEOF(.dcache_ram_bss));

    _video_ram_bss_vma = ADDR(.video_ram_bss);
    _video_ram_bss_size = SIZEOF(.video_ram_bss);
    PROVIDE(video_ram_bss_size = SIZEOF(.video_ram_bss));

#ifdef CONFIG_LZ4_DATA_CODE_ENABLE

    _dcache_ram_data_vma = ADDR(.dcache_ram_data);
    _dcache_ram_data_lma = text_begin + SIZEOF(.text);
    _dcache_ram_data_size = SIZEOF(.dcache_ram_data);
    PROVIDE(dcache_ram_data_size = SIZEOF(.dcache_ram_data));

    _video_ram_data_vma = ADDR(.video_ram_data);
    _video_ram_data_lma = text_begin + SIZEOF(.text) + SIZEOF(.dcache_ram_data);
    _video_ram_data_size = SIZEOF(.video_ram_data);
    PROVIDE(video_ram_data_size = SIZEOF(.video_ram_data));

    _ram0_data_vma = ADDR(.ram0_data);
    _ram0_data_lma = text_begin + SIZEOF(.text) + SIZEOF(.dcache_ram_data) + SIZEOF(.video_ram_data);
    _ram0_data_size = SIZEOF(.ram0_data);
    PROVIDE(ram0_data_size = SIZEOF(.ram0_data));

    data_vma = ADDR(.data);
    data_lma = text_begin + SIZEOF(.text) + SIZEOF(.dcache_ram_data) + SIZEOF(.video_ram_data) + SIZEOF(.ram0_data);
    data_size = SIZEOF(.data);
    PROVIDE(data_size = SIZEOF(.data));

#else

    data_vma = ADDR(.data);
    data_lma = text_begin + SIZEOF(.text);
    data_size = SIZEOF(.data);
    PROVIDE(data_size = SIZEOF(.data));

    _dcache_ram_data_vma = ADDR(.dcache_ram_data);
    _dcache_ram_data_lma = text_begin + SIZEOF(.text) + SIZEOF(.data);
    _dcache_ram_data_size = SIZEOF(.dcache_ram_data);
    PROVIDE(dcache_ram_data_size = SIZEOF(.dcache_ram_data));

    _video_ram_data_vma = ADDR(.video_ram_data);
    _video_ram_data_lma = text_begin + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.dcache_ram_data);
    _video_ram_data_size = SIZEOF(.video_ram_data);
    PROVIDE(video_ram_data_size = SIZEOF(.video_ram_data));

    _ram0_data_vma = ADDR(.ram0_data);
    _ram0_data_lma = text_begin + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.dcache_ram_data) + SIZEOF(.video_ram_data);
    _ram0_data_size = SIZEOF(.ram0_data);
    PROVIDE(ram0_data_size = SIZEOF(.ram0_data));

#endif

    bank_code_load_addr = _ram0_data_lma;
    common_code_run_addr = _ram0_data_vma;

// *INDENT-ON*

EXTERN(
    lib_audio_server_version
    lib_system_version
    lib_btstack_version
    lib_btctler_version
    lib_btmesh_version
    lib_cfg_tool_version
    lib_common_version
    lib_crcm_version
    lib_cryto_toolbox_version
    lib_event_version
    lib_finsh_version
    lib_fs_version
    lib_driver_version
    lib_video_dec_version
    lib_video_rec_version
    lib_update_version
    lib_ui_font_version
    lib_wifi_version
    lib_wl83_rf_version
    lib_httpcli_version
    lib_airplay_version
    lib_aliyun_version
    lib_cjson_version
    lib_dcl_version
    lib_wpa_supplicant_version
    lib_device_vad_version
    lib_dlna_version
    lib_dui_version
    lib_echocloud_version
    lib_ftp_client_version
    lib_hsm_version
    lib_http_server_version
    lib_profile_version
    lib_iperf_version
    lib_jl_cloud_version
    lib_json_c_version
    lib_kcp_version
    lib_coap_version
    lib_duer_version
    lib_mpeg_version
    lib_lwip_2_2_0_version
    lib_mbedtls_3_4_0_version
    lib_mdns_version
    lib_mongoose_version
    lib_mqtt_version
    lib_network_download_version
    lib_rtp_version
    lib_net_server_version
    lib_media_server_version
    lib_stupid_ftpd_version
    lib_ftpserver_version
    lib_telecom_version
    lib_tencent_version
    lib_tmallgenie_version
    lib_tunnel_version
    lib_turing_version
    lib_uc_httpc_version
    lib_uip_version
    lib_websocket_version
    lib_wolfmqtt_version
    lib_wolfssl_version
    lib_wt_version
    lib_zliblite_version
    app_wifi_story_machine_version
    app_wifi_camera_version
    app_scan_box_version
    lib_font_version
    lib_res_version
    lib_ui_draw_version
    lib_ui_version
    lib_lvgl_v8_extend_version
    lib_lvgl_v9_extend_version
)

