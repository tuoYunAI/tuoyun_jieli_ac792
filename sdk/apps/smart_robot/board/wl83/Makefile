
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := 
SYS_INC_DIR := 
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := ../../../../tools/utils/fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := ../../../../cpu/wl83/tools/download.bat
RUN_POST_SCRIPT := ../../../../cpu/wl83/tools/download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := ../../../../cpu/wl83/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/wl83/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../../..

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-integrated-as \
	-mcpu=r3 \
	-mfprev1 \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-femulated-tls \
	-Wno-shift-negative-value \
	-Wframe-larger-than=2560 \
	-mllvm -pi32v2-large-program=true \
	-fms-extensions \
	-w \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_WL83 \
	-DMAX_AUDIO_ADC_CHANNEL_NUM=2 \
	-DCONFIG_UCOS_ENABLE \
	-DCONFIG_THREAD_ENABLE \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DVM_MAX_PAGE_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_MAX_SECTOR_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-D_POSIX_SOURCE \
	-DFIONREAD \
	-DFIONBIO \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib/c++ \
	-I../../../../include_lib/c++/include \
	-I../../../../include_lib/newlib/include \
	-I../../../../include_lib/c++/simple_pthread \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/wl83 \
	-I../../../../include_lib/media \
	-I../../../../include_lib/media/cpu/wl83 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/wl83 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/utils \
	-I../../../../include_lib/utils/syscfg \
	-I../../../../include_lib/utils/event \
	-I../../../../apps \
	-I../../../../apps/smart_robot/include \
	-I../../../../apps/smart_robot/protocol/sip/osip/include \
	-I../../../../apps/smart_robot/board/wl83 \
	-I../../../../apps/common \
	-I../../../../apps/common/include \
	-I../../../../include_lib/btstack \
	-I../../../../include_lib/net/mbedtls_3_4_0 \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/src/include/lwip \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/port \
	-I../../../../include_lib/net/lwip_2_2_0/lwip/app \	
	-I../../../../../apps/common/system/perf_counter \
	-I../../../../../apps/common/rf_fcc_tool/include \
	-I../../../../apps/common/ble/include \
	-I../../../../include_lib/media \
	-I../../../../include_lib/media/cvp \
	-I../../../../include_lib/media/cpu/wl83 \
	-I../../../../include_lib/media/cpu/wl83/asm \
	-I../../../../include_lib/media/framework/include \
	-I../../../../audio/smart_voice \
	-I../../../../audio/effect/spatial_effect \
	-I../../../../include_lib/media/aispeech/enc \
	-I../../../../include_lib/utils/btmesh \
	-I../../../../include_lib/utils/btmesh/adaptation \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/common/config/bt_profile_config.c \
	../../../../apps/common/config/log_config/app_config.c \
	../../../../apps/common/config/log_config/lib_btctrler_config.c \
	../../../../apps/common/config/log_config/lib_btstack_config.c \
	../../../../apps/common/config/log_config/lib_driver_config.c \
	../../../../apps/common/config/log_config/lib_power_config.c \
	../../../../apps/common/config/log_config/lib_system_config.c \
	../../../../apps/common/config/log_config/lib_update_config.c \
	../../../../apps/common/config/user_cfg.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/debug/debug_user.c \
	../../../../apps/common/key/adkey.c \
	../../../../apps/common/key/adkey_rtcvdd.c \
	../../../../apps/common/key/ctmu_touch_key.c \
	../../../../apps/common/key/irkey.c \
	../../../../apps/common/key/key_driver.c \
	../../../../apps/common/key/matrix_keyboard.c \
	../../../../apps/common/key/rdec_key.c \
	../../../../apps/common/key/slidekey.c \
	../../../../apps/common/key/tent600_key.c \
	../../../../apps/common/key/touch_key.c \
	../../../../apps/common/key/uart_key.c \
	../../../../apps/common/net/assign_macaddr.c \
	../../../../apps/common/net/config_network.c \
	../../../../apps/common/net/iperf_test.c \
	../../../../apps/common/net/platform_cfg.c \
	../../../../apps/common/net/wifi_conf.c \
	../../../../apps/common/rf_fcc_tool/bt.c \
	../../../../apps/common/rf_fcc_tool/rf_fcc_main.c \
	../../../../apps/common/rf_fcc_tool/wifi_adaptivity.c \
	../../../../apps/common/system/init.c \
	../../../../apps/common/system/init_expand.c \
	../../../../apps/common/system/perf_counter/example/main.c \
	../../../../apps/common/system/perf_counter/perf_counter.c \
	../../../../apps/common/system/version.c \
	../../../../apps/common/update/update.c \
	../../../../apps/smart_robot/app_main.c \
	../../../../apps/smart_robot/audio/app_audio.c \
	../../../../apps/smart_robot/board/wl83/board.c \
	../../../../apps/smart_robot/board/wl83/sdk_config.c \
	../../../../apps/smart_robot/event/app_event.c \
	../../../../apps/smart_robot/wifi/scan_list.c \
	../../../../apps/smart_robot/wifi/wifi.c \
	../../../../apps/smart_robot/bt_ble/ble.c \
	../../../../apps/smart_robot/bt_ble/ble_net_cfg.c \
	../../../../apps/smart_robot/ota/update.c \
	../../../../apps/smart_robot/protocol/protocol.c\
	../../../../apps/smart_robot/protocol/session/session.c\
	../../../../apps/smart_robot/protocol/traffic/traffic.c\
	../../../../apps/smart_robot/protocol/mcp/mcp_server.c\
	../../../../apps/smart_robot/protocol/sip/sip.c\
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_accept_encoding.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_accept_language.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_accept.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_alert_info.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_allow.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_authentication_info.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_authorization.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_body.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_call_id.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_call_info.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_contact.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_content_disposition.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_content_encoding.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_content_length.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_content_type.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_cseq.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_error_info.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_from.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_header.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_list.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_md5c.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_message_parse.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_message_to_str.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_message.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_mime_version.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_parser_cfg.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_port.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_proxy_authenticate.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_proxy_authentication_info.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_proxy_authorization.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_record_route.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_route.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_to.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_uri.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_via.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/osip_www_authenticate.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/sdp_accessor.c \
	../../../../apps/smart_robot/protocol/sip/osip/src/osipparser2/sdp_message.c
	../../../../audio/CVP/audio_cvp.c \
	../../../../audio/CVP/audio_cvp_3mic.c \
	../../../../audio/CVP/audio_cvp_ais_3mic.c \
	../../../../audio/CVP/audio_cvp_config.c \
	../../../../audio/CVP/audio_cvp_develop.c \
	../../../../audio/CVP/audio_cvp_dms.c \
	../../../../audio/CVP/audio_cvp_online.c \
	../../../../audio/CVP/audio_cvp_ref_task.c \
	../../../../audio/CVP/audio_cvp_sync.c \
	../../../../audio/common/amplitude_statistic.c \
	../../../../audio/common/audio_build_needed.c \
	../../../../audio/common/audio_dvol.c \
	../../../../audio/common/audio_event_handler.c \
	../../../../audio/common/audio_export_demo.c \
	../../../../audio/common/audio_general.c \
	../../../../audio/common/audio_node_config.c \
	../../../../audio/common/audio_noise_gate.c \
	../../../../audio/common/audio_ns.c \
	../../../../audio/common/audio_plc.c \
	../../../../audio/common/audio_utils.c \
	../../../../audio/common/audio_volume_mixer.c \
	../../../../audio/common/bt_audio_energy_detection.c \
	../../../../audio/common/common_func.c \
	../../../../audio/common/config/new_cfg_tool.c \
	../../../../audio/common/frame_length_adaptive.c \
	../../../../audio/common/jlstream_event_handler.c \
	../../../../audio/common/lyrics/lyrics_api.c \
	../../../../audio/common/online_debug/aud_data_export.c \
	../../../../audio/common/online_debug/aud_mic_dut.c \
	../../../../audio/common/online_debug/aud_spatial_effect_dut.c \
	../../../../audio/common/online_debug/audio_capture.c \
	../../../../audio/common/online_debug/audio_online_debug.c \
	../../../../audio/common/uartPcmSender.c \
	../../../../audio/cpu/wl83/audio_accelerator/hw_fft.c \
	../../../../audio/cpu/wl83/audio_config.c \
	../../../../audio/cpu/wl83/audio_dai/audio_iis.c \
	../../../../audio/cpu/wl83/audio_dai/audio_link.c \
	../../../../audio/cpu/wl83/audio_dai/audio_pdm.c \
	../../../../audio/cpu/wl83/audio_setup.c \
	../../../../audio/cpu/wl83/media_memory_manager.c \
	../../../../audio/effect/audio_dc_offset_remove.c \
	../../../../audio/effect/audio_pitch_speed_api.c \
	../../../../audio/effect/audio_voice_changer_api.c \
	../../../../audio/effect/bass_treble.c \
	../../../../audio/effect/effects_adj.c \
	../../../../audio/effect/effects_default_param.c \
	../../../../audio/effect/effects_dev.c \
	../../../../audio/effect/eq_config.c \
	../../../../audio/effect/esco_ul_voice_changer.c \
	../../../../audio/effect/node_param_update.c \
	../../../../audio/effect/scene_update.c \
	../../../../audio/effect/spatial_effect/spatial_effect.c \
	../../../../audio/effect/spatial_effect/spatial_effect_imu.c \
	../../../../audio/effect/spatial_effect/spatial_effect_tws.c \
	../../../../audio/effect/spatial_effect/spatial_effects_process.c \
	../../../../audio/effect/spatial_effect/spatial_imu_trim.c \
	../../../../audio/effect/spk_eq.c \
	../../../../audio/framework/nodes/a2dp_tx_node.c \
	../../../../audio/framework/nodes/agc_node.c \
	../../../../audio/framework/nodes/channle_swap_node.c \
	../../../../audio/framework/nodes/cvp_3mic_node.c \
	../../../../audio/framework/nodes/cvp_develop_node.c \
	../../../../audio/framework/nodes/cvp_dms_node.c \
	../../../../audio/framework/nodes/cvp_sms_node.c \
	../../../../audio/framework/nodes/data_export_node.c \
	../../../../audio/framework/nodes/dns_node.c \
	../../../../audio/framework/nodes/effect_dev0_node.c \
	../../../../audio/framework/nodes/effect_dev1_node.c \
	../../../../audio/framework/nodes/effect_dev2_node.c \
	../../../../audio/framework/nodes/effect_dev3_node.c \
	../../../../audio/framework/nodes/effect_dev4_node.c \
	../../../../audio/framework/nodes/esco_tx_node.c \
	../../../../audio/framework/nodes/iis_node.c \
	../../../../audio/framework/nodes/le_audio_source.c \
	../../../../audio/framework/nodes/lhdc_x_node.c \
	../../../../audio/framework/nodes/local_tws_source_node.c \
	../../../../audio/framework/nodes/multi_ch_iis_node.c \
	../../../../audio/framework/nodes/ns_node.c \
	../../../../audio/framework/nodes/pc_mic_node.c \
	../../../../audio/framework/nodes/plc_node.c \
	../../../../audio/framework/nodes/sink_dev0_node.c \
	../../../../audio/framework/nodes/sink_dev1_node.c \
	../../../../audio/framework/nodes/sink_dev2_node.c \
	../../../../audio/framework/nodes/sink_dev3_node.c \
	../../../../audio/framework/nodes/sink_dev4_node.c \
	../../../../audio/framework/nodes/spatial_effects_node.c \
	../../../../audio/framework/nodes/spdif_node.c \
	../../../../audio/framework/nodes/surround_demo_node.c \
	../../../../audio/framework/nodes/uart_node.c \
	../../../../audio/framework/nodes/usb_host_mic_node.c \
	../../../../audio/framework/nodes/vad_node.c \
	../../../../audio/framework/nodes/vir_data_rx_node.c \
	../../../../audio/framework/nodes/vir_data_tx_node.c \
	../../../../audio/framework/nodes/volume_node.c \
	../../../../audio/framework/plugs/source/a2dp_file.c \
	../../../../audio/framework/plugs/source/a2dp_streamctrl.c \
	../../../../audio/framework/plugs/source/adc_file.c \
	../../../../audio/framework/plugs/source/esco_file.c \
	../../../../audio/framework/plugs/source/fm_file.c \
	../../../../audio/framework/plugs/source/iis_file.c \
	../../../../audio/framework/plugs/source/le_audio_file.c \
	../../../../audio/framework/plugs/source/linein_file.c \
	../../../../audio/framework/plugs/source/local_tws_file.c \
	../../../../audio/framework/plugs/source/multi_ch_adc_file.c \
	../../../../audio/framework/plugs/source/multi_ch_iis_file.c \
	../../../../audio/framework/plugs/source/pc_spk_file.c \
	../../../../audio/framework/plugs/source/pdm_mic_file.c \
	../../../../audio/framework/plugs/source/source_dev0_file.c \
	../../../../audio/framework/plugs/source/source_dev1_file.c \
	../../../../audio/framework/plugs/source/source_dev2_file.c \
	../../../../audio/framework/plugs/source/source_dev3_file.c \
	../../../../audio/framework/plugs/source/source_dev4_file.c \
	../../../../audio/framework/plugs/source/spdif_file.c \
	../../../../audio/framework/plugs/source/usb_host_spk_file.c \
	../../../../audio/interface/player/a2dp_player.c \
	../../../../audio/interface/player/adda_loop_player.c \
	../../../../audio/interface/player/dev_flow_player.c \
	../../../../audio/interface/player/esco_player.c \
	../../../../audio/interface/player/file_player.c \
	../../../../audio/interface/player/fm_player.c \
	../../../../audio/interface/player/iis_player.c \
	../../../../audio/interface/player/key_tone_player.c \
	../../../../audio/interface/player/le_audio_player.c \
	../../../../audio/interface/player/linein_player.c \
	../../../../audio/interface/player/local_tws_player.c \
	../../../../audio/interface/player/net_file_player.c \
	../../../../audio/interface/player/pc_spk_player.c \
	../../../../audio/interface/player/reference_time.c \
	../../../../audio/interface/player/ring_player.c \
	../../../../audio/interface/player/tone_player.c \
	../../../../audio/interface/player/tts_player.c \
	../../../../audio/interface/player/tws_tone_player.c \
	../../../../audio/interface/player/usb_host_spk_player.c \
	../../../../audio/interface/player/video_dec_player.c \
	../../../../audio/interface/player/vir_dev_player.c \
	../../../../audio/interface/player/vir_source_player.c \
	../../../../audio/interface/recorder/ai_voice_recorder.c \
	../../../../audio/interface/recorder/dev_flow_recorder.c \
	../../../../audio/interface/recorder/esco_recorder.c \
	../../../../audio/interface/recorder/file_recorder.c \
	../../../../audio/interface/recorder/le_audio_recorder.c \
	../../../../audio/interface/recorder/pc_mic_recorder.c \
	../../../../audio/interface/recorder/usb_host_mic_recorder.c \
	../../../../audio/interface/recorder/video_rec_recorder.c \
	../../../../audio/interface/recorder/voiceprint_cfg_recorder.c \
	../../../../audio/jl_kws/jl_kws_algo.c \
	../../../../audio/jl_kws/jl_kws_audio.c \
	../../../../audio/jl_kws/jl_kws_event.c \
	../../../../audio/jl_kws/jl_kws_main.c \
	../../../../audio/le_audio/big.c \
	../../../../audio/le_audio/cig.c \
	../../../../audio/le_audio/le_audio_stream.c \
	../../../../audio/le_audio/wireless_trans_manager.c \
	../../../../audio/log_config/lib_media_config.c \
	../../../../audio/music/breakpoint.c \
	../../../../audio/music/music_decrypt.c \
	../../../../audio/music/music_id3.c \
	../../../../audio/music/music_player.c \
	../../../../audio/smart_voice/jl_kws_platform.c \
	../../../../audio/smart_voice/kws_event.c \
	../../../../audio/smart_voice/nn_vad.c \
	../../../../audio/smart_voice/smart_voice_config.c \
	../../../../audio/smart_voice/smart_voice_core.c \
	../../../../audio/smart_voice/user_asr.c \
	../../../../audio/smart_voice/voice_mic_data.c \
	../../../../audio/smart_voice/wanson_asr.c \
	../../../../cpu/wl83/adc_api.c \
	../../../../cpu/wl83/debug.c \
	../../../../cpu/wl83/setup.c \


# 需要编译的 .S 文件
S_SRC_FILES := \
	../../../../include_lib/btctrler/version.z.S \
	../../../../include_lib/btstack/version.z.S \
	../../../../include_lib/driver/version.z.S \
	../../../../include_lib/net/version.z.S \
	../../../../include_lib/system/version.z.S \
	../../../../include_lib/update/version.z.S \
	../../../../include_lib/utils/version.z.S \
	../../../../../lib/net/version.z.S \
	../../../../lib/utils/version.z.S \


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-emulated-tls \
	--sort-common \
	--plugin-opt=-used-symbol-file=../../../../cpu/wl83/sdk_used_list.used \
	--plugin-opt=-enable-movable-region=true \
	--plugin-opt=-movable-region-section-prefix=.movable.slot. \
	--plugin-opt=-movable-region-stub-section-prefix=.movable.stub. \
	--plugin-opt=-movable-region-prefix=.movable.region. \
	--plugin-opt=-movable-region-stub-prefix=__movable_stub_ \
	--plugin-opt=-movable-region-stub-swi-number=-2 \
	--plugin-opt=-movable-region-map-file-list=../../../../apps/common/movable/funcname.txt \
	--plugin-opt=-movable-region-section-map-file-list=../../../../apps/common/movable/section.txt \
	--plugin-opt=-movable-region-exclude-func-file-list=../../../../apps/common/movable/exclude.txt \
	--plugin-opt=-pi32v2-large-program=true \
	--plugin-opt=-pi32v2-csync-before-rti=true \
	--start-group \
	../../../../cpu/wl83/liba/cpu.a \
	../../../../cpu/wl83/liba/fs.a \
	../../../../cpu/wl83/liba/event.a \
	../../../../cpu/wl83/liba/system.a \
	../../../../cpu/wl83/liba/cfg_tool.a \

	../../../../cpu/wl83/liba/common_lib.a \
	../../../../cpu/wl83/liba/lib_mqtt.a \
	../../../../cpu/wl83/liba/libcrypto_mbedtls.a \
	../../../../cpu/wl83/liba/update.a \
	../../../../cpu/wl83/liba/wl_rf_common.a \
	../../../../cpu/wl83/liba/hsm.a \
	../../../../cpu/wl83/liba/wpasupplicant.a \
	../../../../cpu/wl83/liba/http_cli.a \
	../../../../cpu/wl83/liba/https_cli.a \
	../../../../cpu/wl83/liba/json.a \
	../../../../cpu/wl83/liba/iperf.a \
	../../../../cpu/wl83/liba/libmbedtls_3_4_0.a \
	../../../../cpu/wl83/liba/libcrypto_mbedtls.a \
	../../../../cpu/wl83/liba/audio_server.a \
	./../../../cpu/wl83/liba/echo_server.a \
	../../../../cpu/wl83/liba/media.a \
	../../../../cpu/wl83/liba/media/lib_mp3_dec.a \
	../../../../cpu/wl83/liba/media/lib_m4a_dec.a \
	../../../../cpu/wl83/liba/media/lib_mty_dec.a \
	../../../../cpu/wl83/liba/media/lib_f2a_dec.a \
	../../../../cpu/wl83/liba/media/lib_wav_dec.a \
	../../../../cpu/wl83/liba/media/lib_wtg_dec.a \
	../../../../cpu/wl83/liba/media/lib_wtgv2_dec.a \
	../../../../cpu/wl83/liba/media/lib_flac_dec.a \
	../../../../cpu/wl83/liba/media/lib_ape_dec.a \
	../../../../cpu/wl83/liba/media/lib_amr_dec.a \
	../../../../cpu/wl83/liba/media/lib_amr_wb_dec.a \
	../../../../cpu/wl83/liba/media/lib_dts_dec.a \
	../../../../cpu/wl83/liba/media/lib_wma_dec.a \
	../../../../cpu/wl83/liba/media/lib_sbc_dec.a \
	../../../../cpu/wl83/liba/media/lib_bt_aac_dec.a \
	../../../../cpu/wl83/liba/media/lib_ogg_opus_dec.a \
	../../../../cpu/wl83/liba/media/lib_ogg_vorbis_dec.a \
	../../../../cpu/wl83/liba/media/lib_ldac_dec.a \
	../../../../cpu/wl83/liba/media/lib_lhdcv4_dec.a \
	../../../../cpu/wl83/liba/media/lib_lhdcv5_dec.a \
	../../../../cpu/wl83/liba/media/lib_jla_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_v2_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_ll_codec.a \
	../../../../cpu/wl83/liba/media/lib_jla_lw_codec.a \
	../../../../cpu/wl83/liba/media/lib_lc3_codec.a \
	../../../../cpu/wl83/liba/media/lib_jl_codec_common.a \
	../../../../cpu/wl83/liba/media/lib_mp3_dec_stream.a \
	../../../../cpu/wl83/liba/media/lib_msbc_codec.a \
	../../../../cpu/wl83/liba/media/lib_speex_codec.a \
	../../../../cpu/wl83/liba/media/lib_mp2_codec.a \
	../../../../cpu/wl83/liba/media/lib_adpcm_codec.a \
	../../../../cpu/wl83/liba/media/lib_adpcm_ima_codec.a \
	../../../../cpu/wl83/liba/media/lib_aac_enc.a \
	../../../../cpu/wl83/liba/media/lib_amr_enc.a \
	../../../../cpu/wl83/liba/media/lib_amr_wb_enc.a \
	../../../../cpu/wl83/liba/media/lib_mp3_enc.a \
	../../../../cpu/wl83/liba/media/lib_opus_enc.a \
	../../../../cpu/wl83/liba/media/lib_sbc_eng.a \
	../../../../cpu/wl83/liba/media/lib_bt_hash_enc.a \
	../../../../cpu/wl83/liba/media/lib_advaudio_plc.a \
	../../../../cpu/wl83/liba/media/lib_esco_audio_plc.a \
	../../../../cpu/wl83/liba/media/libJLPhoneCall.a \
	../../../../cpu/wl83/liba/media/libDns.a \
	../../../../cpu/wl83/liba/media/libJLsp.a \
	../../../../cpu/wl83/liba/media/libJLspKws.a \
	../../../../cpu/wl83/liba/media/libJLspKwsFarKeyword.a \
	../../../../cpu/wl83/liba/media/libKwsCommon.a \
	../../../../cpu/wl83/liba/media/libAptFilt.a \
	../../../../cpu/wl83/liba/media/libEchoSuppress.a \
	../../../../cpu/wl83/liba/media/libNoiseSuppress.a \
	../../../../cpu/wl83/liba/media/libAdaptiveEchoSuppress.a \
	../../../../cpu/wl83/liba/media/libSplittingFilter.a \
	../../../../cpu/wl83/liba/media/libDelayEstimate.a \
	../../../../cpu/wl83/liba/media/libDualMicSystem.a \
	../../../../cpu/wl83/liba/media/libSMS_TDE.a \
	../../../../cpu/wl83/liba/media/libSAS.a \
	../../../../cpu/wl83/liba/media/libAec.a \
	../../../../cpu/wl83/liba/media/libFFT.a \
	../../../../cpu/wl83/liba/media/libDynamicEQ.a \
	../../../../cpu/wl83/liba/media/libDynamicEQPro.a \
	../../../../cpu/wl83/liba/media/libLoudness.a \
	../../../../cpu/wl83/liba/media/libLlns.a \
	../../../../cpu/wl83/liba/media/libHowling.a \
	../../../../cpu/wl83/liba/media/libCrossoverCoff.a \
	../../../../cpu/wl83/liba/media/libDrc.a \
	../../../../cpu/wl83/liba/media/libDrcDetect.a \
	../../../../cpu/wl83/liba/media/libDrcAdvance.a \
	../../../../cpu/wl83/liba/media/libVoiceChanger.a \
	../../../../cpu/wl83/liba/media/libFrequencyShift.a \
	../../../../cpu/wl83/liba/media/libFrequencyCompressor.a \
	../../../../cpu/wl83/liba/media/libReverb.a \
	../../../../cpu/wl83/liba/media/libReverbCal.a \
	../../../../cpu/wl83/liba/media/libPitchSpeed.a \
	../../../../cpu/wl83/liba/media/libHarmonicExciter.a \
	../../../../cpu/wl83/liba/media/libResampleFastCal.a \
	../../../../cpu/wl83/liba/media/libChorus.a \
	../../../../cpu/wl83/liba/media/libPlateReverbAdv.a \
	../../../../cpu/wl83/liba/media/libPcmDelay.a \
	../../../../cpu/wl83/liba/media/libPingPongDelay.a \
	../../../../cpu/wl83/liba/media/libThreeDEffect.a \
	../../../../cpu/wl83/liba/media/libAutoWah.a \
	../../../../cpu/wl83/liba/media/libSpectrum.a \
	../../../../cpu/wl83/liba/media/libSpectrumShow.a \
	../../../../cpu/wl83/liba/media/libStereoWidener.a \
	../../../../cpu/wl83/liba/media/libStereoSpatialWider.a \
	../../../../cpu/wl83/liba/media/libStereoMtapsEcho.a \
	../../../../cpu/wl83/liba/media/libSurroundEffect.a \
	../../../../cpu/wl83/liba/media/libVirtualBass.a \
	../../../../cpu/wl83/liba/media/libVirtualBassClassic.a \
	../../../../cpu/wl83/liba/media/libHowlingGate.a \
	../../../../cpu/wl83/liba/media/libDistortion.a \
	../../../../cpu/wl83/liba/media/libDistortionEXP.a \
	../../../../cpu/wl83/liba/media/libNoisegate.a \
	../../../../cpu/wl83/liba/media/libNoisegatePro.a \
	../../../../cpu/wl83/liba/media/libIIRFilter.a \
	../../../../cpu/wl83/liba/media/libLimiter.a \
	../../../../cpu/wl83/liba/media/libBfilterFun.a \
	../../../../cpu/wl83/liba/media/lib_phaser.a \
	../../../../cpu/wl83/liba/media/lib_flanger.a \
	../../../../cpu/wl83/liba/media/lib_chorus_adv.a \
	../../../../cpu/wl83/liba/media/lib_pingpong_echo.a \
	../../../../cpu/wl83/liba/media/lib_wanson_asr.a \
	../../../../cpu/wl83/liba/libvad.a \
	../../../../cpu/wl83/liba/lwip_2_2_0.a \
	../../../../cpu/wl83/liba/wl_wifi.a \
	../../../../cpu/wl83/liba/btctrler.a \
	../../../../cpu/wl83/liba/btstack.a \
	../../../../cpu/wl83/liba/crypto_toolbox_Osize.a \
	../../../../cpu/wl83/liba/lib_ccm_aes.a \
	../../../../cpu/wl83/liba/lib_sig_mesh.a \
	--end-group \
	../../../../include_lib/newlib/pi32v2-lib/libm.a \
	../../../../include_lib/newlib/pi32v2-lib/libc.a \
	../../../../include_lib/newlib/pi32v2-lib/libcompiler_rt.a \
	-T../../../../cpu/wl83/sdk.ld \
	-M=../../../../cpu/wl83/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/sdk_used_list.c -o ../../../../cpu/wl83/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../apps/common/movable/section.c -o ../../../../apps/common/movable/section.txt
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/sdk_ld.c -o ../../../../cpu/wl83/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/isd_config_rule.c -o ../../../../cpu/wl83/tools/isd_config.ini
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/wl83/tools/isd_config_rule_loader.c -o ../../../../cpu/wl83/tools/isd_config_loader.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : $(ROOT_PREFIX)/%.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cc.o : $(ROOT_PREFIX)/%.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

-include $(DEP_FILES)
