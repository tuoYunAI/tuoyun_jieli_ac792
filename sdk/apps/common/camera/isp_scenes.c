#include "system/includes.h"
#include "app_config.h"
#include "cpu.h"
#include "asm/debug.h"
#include "video/isp.h"

#define DEBUG_ISP   0

#if (DEBUG_ISP == 1)
#define isp_puts  puts
#define __isp_cfg_printf  printf
#else
#define isp_puts(...)
#define __isp_cfg_printf(...)
#endif

#define USER_ISP_MODE_NORMAL    0x0
#define USER_ISP_MODE_SPECIAL   0x1

#define USER_CFG_STATE_UNINIT    0x0
#define USER_CFG_STATE_INIT     0x1
#define USER_CFG_STATE_RUN      0x2
#define USER_CFG_STATE_SWITCH   0x3
#define USER_CFG_STATE_STOP     0x4

//isp场景效果总段数修改
#define MAX_ISP_SCENES 4

//添加场景名字从0开始往后递增0,1,2,3,4,5.........
#define SCENE_NIGHT             0
#define SCENE_INDOOR            1
#define SCENE_OUTCLUDY          2
#define SCENE_OUTDOOR           3

struct isp_scenes {
    u8 *buf;
    u32 size;
};

struct usr_isp_cfg_par {
    u32 levl;
    u32 levh;
};

struct user_isp_cfg {
    u8 enable;
    u8 state;
    u8 channel;
    struct usr_isp_cfg_par cfg_par[MAX_ISP_SCENES];
    char cfg_file[MAX_ISP_SCENES][64];
    u8 mode;
    s8 current_scene;
    int cur_lv;
    int prev_lv;
    struct isp_scenes scenes[MAX_ISP_SCENES];
    int timer;
    OS_SEM sem;
    spinlock_t lock;
};

/**********************************isp效果配置修改****************************************/

//将场景添加到scane_type数组中
unsigned char scane_type[] = {SCENE_NIGHT, SCENE_INDOOR, SCENE_OUTCLUDY, SCENE_OUTDOOR};

/**************************************************************************/
//isp0
//添加isp效果对应段的阈值范围
#define USER_ISP0_CFG_NIGHT_L    ((0 << 8))
#define USER_ISP0_CFG_NIGHT_H    ((1 << 8) 	- 40)

#define USER_ISP0_CFG_INDOOR_L   ((1 << 8) 	+ 10)
#define USER_ISP0_CFG_INDOOR_H   ((7 << 8) 	- 256)

#define USER_ISP0_CFG_OUTCLUDY_L ((7 << 8) 	+ 256)
#define USER_ISP0_CFG_OUTCLUDY_H ((12 << 8) 	- 128)

#define USER_ISP0_CFG_OUTDOOR_L  ((12 << 8) 	+ 128)
#define USER_ISP0_CFG_OUTDOOR_H  ((20 << 8)  + 128)

/**************************************************************************/
//isp1
//添加isp效果对应段的阈值范围
#define USER_ISP1_CFG_NIGHT_L    ((0 << 8))
#define USER_ISP1_CFG_NIGHT_H    ((1 << 8) - 40)

#define USER_ISP1_CFG_INDOOR_L   ((1 << 8) + 10)
#define USER_ISP1_CFG_INDOOR_H   ((7 << 8) - 256)

#define USER_ISP1_CFG_OUTCLUDY_L ((7 << 8) + 256)
#define USER_ISP1_CFG_OUTCLUDY_H ((12 << 8) - 128)

#define USER_ISP1_CFG_OUTDOOR_L  ((12 << 8) + 128)
#define USER_ISP1_CFG_OUTDOOR_H  ((20 << 8) + 128)

/**************************************************************************/

//0xff 为自适配通道, 填对应通道号则指定
#define ISP0_CFG_CAMERA_CHANNEL 0xff
#define ISP1_CFG_CAMERA_CHANNEL 0xff

#define USE_FILE_SCENES_CHANGE  1

static struct user_isp_cfg  isp0_cfg = {
    .channel = ISP0_CFG_CAMERA_CHANNEL,
    .cfg_par = {
        {USER_ISP0_CFG_NIGHT_L, 	USER_ISP0_CFG_NIGHT_H},
        {USER_ISP0_CFG_INDOOR_L, 	USER_ISP0_CFG_INDOOR_H},
        {USER_ISP0_CFG_OUTCLUDY_L, 	USER_ISP0_CFG_OUTCLUDY_H},
        {USER_ISP0_CFG_OUTDOOR_L,	USER_ISP0_CFG_OUTDOOR_H},
    },

    .cfg_file = {
        "mnt/sdfile/res/cfg/isp0_cfg_0.bin",
        "mnt/sdfile/res/cfg/isp0_cfg_1.bin",
        "mnt/sdfile/res/cfg/isp0_cfg_2.bin",
        "mnt/sdfile/res/cfg/isp0_cfg_3.bin",
    },
};

static struct user_isp_cfg  isp1_cfg = {
    .channel = ISP1_CFG_CAMERA_CHANNEL,
    .cfg_par = {
        {USER_ISP1_CFG_NIGHT_L, 	USER_ISP1_CFG_NIGHT_H},
        {USER_ISP1_CFG_INDOOR_L, 	USER_ISP1_CFG_INDOOR_H},
        {USER_ISP1_CFG_OUTCLUDY_L, 	USER_ISP1_CFG_OUTCLUDY_H},
        {USER_ISP1_CFG_OUTDOOR_L,	USER_ISP1_CFG_OUTDOOR_H},
    },

    .cfg_file = {
        "mnt/sdfile/res/cfg/isp1_cfg_0.bin",
        "mnt/sdfile/res/cfg/isp1_cfg_1.bin",
        "mnt/sdfile/res/cfg/isp1_cfg_2.bin",
        "mnt/sdfile/res/cfg/isp1_cfg_3.bin",
    },
};

static struct user_isp_cfg  *isp_cfg[2] = {&isp0_cfg, &isp1_cfg};

static void *isp_channel_select(u8 channel)
{
    for (int i = 0; i < 2; i++) {
        if (isp_cfg[i]->channel == channel) {
            return isp_cfg[i];
        }
    }
    return NULL;
}

static void *isp_cfg_init(u8 channel)
{
    for (int i = 0; i < 2; i++) {
        if (isp_cfg[i]->channel == 0xff) {
            isp_cfg[i]->channel = channel;
            spin_lock_init(&isp_cfg[i]->lock);
            return isp_cfg[i];
        }
    }
    return NULL;
}

static int isp_cfg_judge_first(struct user_isp_cfg *cfg)
{
    u32 i;
    u32 lev = cfg->cur_lv;
    u32 i_num = sizeof(cfg->cfg_par) / sizeof(cfg->cfg_par[0]);
    if (i_num >= 2) {
        for (i = 0; i < (i_num - 2); i++) {
            if (lev >= (cfg->cfg_par[i].levh + cfg->cfg_par[i + 1].levl) / 2  && lev <= (cfg->cfg_par[i + 1].levh + cfg->cfg_par[i + 2].levl) / 2) {
                //中间
                return (i + 1);
            }
        }

        if (lev >= (cfg->cfg_par[i_num - 2].levh + cfg->cfg_par[i_num - 1].levl) / 2) {
            //最高
            return (i_num - 1);
        }
        if (lev <= (cfg->cfg_par[0].levh + cfg->cfg_par[1].levl) / 2) {
            //最低
            return 0;
        }
    } else {
    }
    return 0;
}

static int isp_cfg_gap(struct user_isp_cfg *cfg, u32 lev, u32 i_pre)
{
    u32 i_gap;
    u32 i_num;
    i_num = sizeof(cfg->cfg_par) / sizeof(cfg->cfg_par[0]);
    for (i_gap = 0;  i_gap < (i_num - 1); i_gap++) {
        if (lev >= cfg->cfg_par[i_gap].levh && lev <= cfg->cfg_par[i_gap + 1].levl) {
            if (i_gap > i_pre || (i_gap + 1) < i_pre) {
                //非相邻段,重新配置
                return i_gap;
            }
        }
    }
    return i_pre;//相邻段
}

static int isp_cfg_judge(struct user_isp_cfg *cfg)
{
    static u32 i_pre;
    u32 lev = cfg->cur_lv;
    u32 i_num = sizeof(cfg->cfg_par) / sizeof(cfg->cfg_par[0]);
    u32 i;
    for (i = 0; i < i_num; i++) {
        if (lev > cfg->cfg_par[i].levl && lev < cfg->cfg_par[i].levh) {
            i_pre = i;
            return i;
        }
    }
    if (lev >= cfg->cfg_par[i_num - 1].levh) {
        i_pre = i_num - 1;
        return (i_num - 1);
    }
    if (lev <= cfg->cfg_par[0].levl) {
        i_pre = 0;
        return 0;
    }
    i_pre = isp_cfg_gap(cfg, lev, i_pre);
    //__isp_cfg_printf("\ni_pre %d\n", i_pre);
    return i_pre;
}
int isp_scr_work_hdl(u8 scene)
{
#if 0
    if (get_usb_in_status()) {
        return 0;
    }

    if ((get_video_disp_state() == DISP_BACK_WIN)
        || (get_parking_status() != 0)) {
        //后拉全屏显示（录像时），屏显效果固定为场景1
        scene = 1;
        __this->current_scene = scene;
    }

    struct isp_scr_work *work = NULL;
    extern struct isp_scr_work isp_scr_begin[];
    extern struct isp_scr_work isp_scr_end[];

    for (work = isp_scr_begin; work < isp_scr_end; work++) {
        if (scene == work->parm) {
            work->handler(work->parm);
            goto __END;
        }
    }

    if (work == isp_scr_end && work > isp_scr_begin) {
        //处理屏幕效果场景个数，少于isp效果场景数
        work = (isp_scr_end - 1);
        if (work->parm < scene) {
            work->handler(work->parm);
        }
    }

__END:
#endif

    return 0;

}


static int user_isp_cfg_init(struct user_isp_cfg *cfg)
{
    int i;
    if (cfg->state == USER_CFG_STATE_UNINIT) {
        memset(cfg->scenes, 0x0, sizeof(cfg->scenes));

        for (i = 0; i < MAX_ISP_SCENES; i++) {
            void *fp;
            fp = fopen((char *)cfg->cfg_file[i], "r");
            if (fp) {
                int len = flen(fp);
                cfg->scenes[i].buf = malloc(len);
                if (!cfg->scenes[i].buf) {
                    isp_puts("no mem.\n");
                    return -ENOMEM;
                }
                cfg->scenes[i].size =  len;
                __isp_cfg_printf("open %s sucess:%d\n", (char *)cfg->cfg_file[i], len);
                len = fread(cfg->scenes[i].buf, cfg->scenes[i].size, 1, fp);
                fclose(fp);
                /* printf_buf(cfg->scenes[i].buf,cfg->scenes[i].size); */
            }
        }
        cfg->state = USER_CFG_STATE_INIT;
    }

    return 0;
}

int load_default_camera_config(u32 lv, void *arg)
{
#if 0
    u8 i;
    u8 scene;
    struct ispt_customize_cfg *cfg = (struct ispt_customize_cfg *)arg;
    /*
     *如果已经有效果，则不需要配置
     */
    if (__this->state != USER_CFG_STATE_UNINIT) {
        return -EINVAL;
    }
    /*
     *效果配置初始化
     */
    user_isp_cfg_init();
    __this->cur_lv = lv;
    __this->prev_lv = lv;

    i = isp_cfg_judge_first(__this->cur_lv);
    scene = scane_type[i];

    /*
     *获取当前效果文件
     */
    cfg->mode = ISP_CUSTOMIZE_MODE_FILE;
    cfg->data = __this->scenes[scene].buf;
    cfg->len = __this->scenes[scene].size;

    __this->state = USER_CFG_STATE_INIT;
#endif
    return 0;
}

static int flush_isp_cfg(struct user_isp_cfg *cfg)
{
    u8 channel = cfg->channel;
    u8 scene_type = cfg->current_scene;

    struct ispt_customize_cfg cus_cfg;

    if (scene_type > MAX_ISP_SCENES) {
        return -EINVAL;
    }

    if (cfg->scenes[scene_type].buf) {
        cus_cfg.mode = ISP_CUSTOMIZE_MODE_FILE;
        cus_cfg.channel = channel;
        cus_cfg.data = cfg->scenes[scene_type].buf;
        cus_cfg.len = cfg->scenes[scene_type].size;

        isp_ioctl(ISP_IOCTL_SET_CUSTOMIZE_CFG, (unsigned int)&cus_cfg);
    }

    return 0;
}

static void isp_scene_switch(struct user_isp_cfg *cfg, u8 force)
{
    int i = 0;
    int ret = 0;
    int scene = cfg->current_scene;
    cfg->prev_lv = cfg->cur_lv;

    struct isp_generic_cfg gen_cfg = {0};
    gen_cfg.cmd = ISP_GET_LV;
    gen_cfg.channel = cfg->channel;
    ret = isp_ioctl(ISP_IOCTL_GET_GENERIC_CFG, (unsigned int)&gen_cfg);
    if (!ret) {
        cfg->cur_lv = gen_cfg.lv;
    }
    __isp_cfg_printf("\nch%d: cur lv %d\n", cfg->channel, cfg->cur_lv);
    if (force) {
        i = isp_cfg_judge_first(cfg);
    } else {
        i = isp_cfg_judge(cfg);
        __isp_cfg_printf("\nch%d: cur scene %d\n", cfg->channel, scane_type[i]);
    }
    scene = scane_type[i];

    if ((!force)  && (scene == cfg->current_scene)) {
        return;
    }

    __isp_cfg_printf("ch%d: scene %d\n", cfg->channel, scene);
    cfg->current_scene = scene;
    flush_isp_cfg(cfg);

    //屏效跟随isp效果变化
    void lcd_scenes_switch(int model);
    lcd_scenes_switch(scene);
}

static void isp_scene_switch_timer(struct user_isp_cfg *cfg)
{
    int err;

    err = os_sem_pend(&cfg->sem, 10);
    if (err) {
        puts("isp scene switch timeout.\n");
        return;
    }

    spin_lock(&cfg->lock);
    if (cfg->enable) {
        spin_unlock(&cfg->lock);
        isp_scene_switch(cfg, 0);
        spin_lock(&cfg->lock);
    }
    spin_unlock(&cfg->lock);

    os_sem_post(&cfg->sem);
}

int stop_update_isp_scenes(u8 channel)
{
#if USE_FILE_SCENES_CHANGE
    struct user_isp_cfg *cfg = isp_channel_select(channel);
    if (!cfg) {
        printf("can't find ch %d isp scenes.", channel);
        return -1;
    }

    if (cfg->timer) {
        sys_timer_modify(cfg->timer, 10);//modify by dhx
        os_sem_pend(&cfg->sem, 0);
        sys_timer_del(cfg->timer);
        cfg->timer = 0;
    }

    if ((cfg->state == USER_CFG_STATE_UNINIT) ||
        (cfg->state == USER_CFG_STATE_INIT) ||
        (cfg->state == USER_CFG_STATE_STOP)) {
        return 0;
    }

    cfg->state = USER_CFG_STATE_STOP;

#endif
    return 0;
}


int init_update_isp_scenes(u8 channel)
{
    struct user_isp_cfg *cfg = isp_channel_select(channel);
    if (!cfg) {
        cfg = isp_cfg_init(channel);
        ASSERT(cfg, "isp_cfg_init failed.");
    }

    if ((cfg->state != USER_CFG_STATE_STOP) &&
        (cfg->state != USER_CFG_STATE_INIT) &&
        (cfg->state != USER_CFG_STATE_UNINIT)) {
        stop_update_isp_scenes(channel);
    }

    user_isp_cfg_init(cfg);

    return 0;
}
/*
 *开始使能更新isp效果配置
 *
 */
int start_update_isp_scenes(u8 channel)
{
#if USE_FILE_SCENES_CHANGE
    struct user_isp_cfg *cfg = isp_channel_select(channel);
    if (!cfg) {
        cfg = isp_cfg_init(channel);
        ASSERT(cfg, "isp_cfg_init failed.");
    }

    if ((cfg->state != USER_CFG_STATE_STOP) &&
        (cfg->state != USER_CFG_STATE_INIT) &&
        (cfg->state != USER_CFG_STATE_UNINIT)) {
        stop_update_isp_scenes(channel);
    }

    os_sem_create(&cfg->sem, 1);

    if (cfg->state == USER_CFG_STATE_UNINIT) {
        user_isp_cfg_init(cfg);
    }

    if (cfg->state == USER_CFG_STATE_INIT) {
        cfg->enable = 1;
        isp_scene_switch(cfg, 1);
    } else if (cfg->state == USER_CFG_STATE_STOP) {
        isp_scene_switch(cfg, 1);
    }
    cfg->state = USER_CFG_STATE_RUN;


    if (!cfg->timer) {
        cfg->timer = sys_timer_add_to_task("app_core", cfg, isp_scene_switch_timer, 1000);
    }

#endif
    return 0;
}

/*
   手机APP控制录像不能直接调用stop_update_isp_scenes，需要判断isp_scene_switch_timer函数执行完才能调用
   stop_update_isp_scenes，否则会死等信号量
   */
int isp_scenes_switch_timer_done(void)
{
    //TODO
    return 1;
}

int user_isp_get_current_scene(int channel)
{
    struct user_isp_cfg *cfg = isp_channel_select(isp0_get_id());

    if (!cfg || cfg->state != USER_CFG_STATE_RUN) {
        printf("isp cfg select err \n");
        return -1;
    }

    return cfg->current_scene;
}

static u8 prev_scene_flag;
static int prev_scene_lv;
int user_prev_set_lv(int lv)
{
    prev_scene_lv = lv;
    prev_scene_flag = 1;

    return 0;
}

int update_prev_init(int channel)
{
    if (!prev_scene_flag) {
        return 0;
    }

    prev_scene_flag = 0;

    struct user_isp_cfg *cfg = isp_channel_select(channel);
    if (!cfg) {
        ASSERT(cfg, "isp_cfg_init failed.");
    }

    cfg->cur_lv = prev_scene_lv;
    int i = isp_cfg_judge_first(cfg);
    cfg->current_scene = scane_type[i];
    flush_isp_cfg(cfg);

    return 0;
}


//*****************************************************************//
//添加色彩模式名字
#define COLOR_DEFAULT           0
#define COLOR_WHITE_BLACK       1
#define COLOR_RETRO             2
unsigned char color_type[] = {COLOR_DEFAULT, COLOR_WHITE_BLACK, COLOR_RETRO};
char color_file[][64] = {
    "mnt/sdfile/res/cfg/isp_color0.bin",
    "mnt/sdfile/res/cfg/isp_color1.bin",
    "mnt/sdfile/res/cfg/isp_color2.bin",
};


int user_isp_color_switch(int channel, int mode)
{
#if USE_FILE_SCENES_CHANGE
    struct user_isp_cfg *cfg = isp_channel_select(channel);
    if (!cfg || cfg->state != USER_CFG_STATE_RUN) {
        return -1;
    }

    spin_lock(&cfg->lock);
    if (color_type[mode] == COLOR_DEFAULT) {
        cfg->enable = 1;
        flush_isp_cfg(cfg);
        spin_unlock(&cfg->lock);
        return 0;
    } else {
        cfg->enable = 0;
    }
    spin_unlock(&cfg->lock);
#endif

    void *fp = fopen(color_file[mode], "r");
    if (!fp) {
        return -1;
    }
    int len = flen(fp);
    void *buf = malloc(len);
    if (!buf) {
        return -1;
    }
    len = fread(buf, len, 1, fp);
    fclose(fp);

    struct ispt_customize_cfg cus_cfg;
    cus_cfg.mode = ISP_CUSTOMIZE_MODE_FILE;
    cus_cfg.channel = channel;
    cus_cfg.data = buf;
    cus_cfg.len = len;

    isp_ioctl(ISP_IOCTL_SET_CUSTOMIZE_CFG, (unsigned int)&cus_cfg);

    free(buf);

    return 0;
}
















