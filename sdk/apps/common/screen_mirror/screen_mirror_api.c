#include "system/includes.h"
#include "pipeline_core.h"
#include "app_config.h"
#include "action.h"
#include "lcd_config.h"
#include "screen_mirror_api.h"

#define LOG_TAG_CONST       SCREEN_MIRROR_API
#define LOG_TAG             "[SCREEN_MIRROR_API]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE

#include "system/debug.h"

#ifdef CONFIG_NET_SCR

struct __NET_SCR_INFO {
    u8 state;
    pipe_core_t *pipe_core;
    struct sockaddr_in cli_addr;
};
struct __NET_SCR_INFO scr_info = {0};
#define __this (&scr_info)

int net_scr_init(struct __NET_SCR_CFG *cfg)
{
    pipe_filter_t *source_filter, *jpeg_dec_filter, *imc_filter, *rep_filter, *disp_filter;
    struct video_format f = {0};

    if (__this->state) {
        log_error("%s multiple init\n", __func__);
        return 0;
    }
    memcpy(&__this->cli_addr, &cfg->cli_addr, sizeof(struct sockaddr_in));

    log_info("scr size: %d x %d", cfg->src_w, cfg->src_h);
    log_info("scr fps: %d, prot: %s, ack: %d", cfg->fps, (cfg->prot == 0) ? "tcp" : "udp", cfg->ack);

    __this->pipe_core = pipeline_init(NULL, NULL);
    ASSERT(__this->pipe_core);

    char *source_name = "scr0";
    __this->pipe_core->channel = plugin_source_to_channel(source_name);

    source_filter = pipeline_filter_add(__this->pipe_core, source_name);
    jpeg_dec_filter = pipeline_filter_add(__this->pipe_core, plugin_factory_find("jpeg_dec"));
    rep_filter = pipeline_filter_add(__this->pipe_core, plugin_factory_find("rep"));
    imc_filter = pipeline_filter_add(__this->pipe_core, find_use_for_display_plugin("imc"));
    disp_filter = pipeline_filter_add(__this->pipe_core, plugin_factory_find("disp"));

    //数据源数据格式
    f.src_width = cfg->src_w;
    f.src_height = cfg->src_h;
    //imc不做帧率控制,以下发帧率为准
    f.fps = cfg->fps;

    //显示配置
    f.win.left 	 = 0;
    f.win.top  	 = 0;
    f.win.width = f.src_width;
    f.win.height = f.src_height;
    f.win.combine = 1; //合成显示

    pipeline_param_set(__this->pipe_core, NULL, PIPELINE_SET_FORMAT, &f);
    pipeline_param_set(__this->pipe_core, NULL, PIPELINE_SCR_CLI_ADR, &cfg->cli_addr);
    int sock_type = (cfg->prot == 0) ? SOCK_STREAM : SOCK_DGRAM;
    pipeline_param_set(__this->pipe_core, NULL, PIPELINE_SCR_SOCK_TYPE, &sock_type);
    pipeline_param_set(__this->pipe_core, NULL, PIPELINE_SCR_ACK_CALLBACK, &cfg->ack_cb);
    int line_cnt = 16;
    pipeline_param_set(__this->pipe_core, NULL, PIPELINE_SET_BUFFER_LINE, (int)&line_cnt);

    pipeline_filter_link(source_filter, jpeg_dec_filter);
    pipeline_filter_link(jpeg_dec_filter, rep_filter);
    pipeline_filter_link(rep_filter, imc_filter);
    pipeline_filter_link(imc_filter, disp_filter);

    pipeline_prepare(__this->pipe_core);
    pipeline_start(__this->pipe_core);

    __this->state = 1;

    return 0;
}

int net_scr_uninit(struct __NET_SCR_CFG *cfg)
{
    if (!__this->state || memcmp((char *)&__this->cli_addr + 2, (char *)&cfg->cli_addr + 2, 6)) {
        log_error("cli addr not match.\n");
        return -1;
    }

    __this->state = 0;

    pipeline_stop(__this->pipe_core);
    pipeline_reset(__this->pipe_core);

    pipeline_uninit(__this->pipe_core);

    return 0;
}

u8 get_net_scr_status(void)
{
    return __this->state;
}

#endif

