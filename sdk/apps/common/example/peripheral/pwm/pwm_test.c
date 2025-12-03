#include "app_config.h"
#include "system/includes.h"
#include "asm/pwm.h"
#include "asm/gpio.h"
#include "device/device.h"

/*********************************PWM设备例子****************************************************
  支持使用board.h板级配置初始化、自定义配置初始化，IOCTL命令基本支持多通道控制
  PWM通过dev_ioctl控制，cmd命令查看pwm.h，形参arg类型查看pwm.h
************************************************************************************************/

#ifdef USE_PWM_TEST_DEMO

static void pwm_test_task(void *arg)
{
    void *pwm_dev_handl = NULL;

    struct pwm_platform_data pwm = {
        .timer_pwm_port             = {-1, -1, IO_PORTA_04, -1},
        .mcpwm_port                 = {-1, -1, -1, IO_PORTA_04, -1, -1, -1, -1},
        .pwm_config = {
            .pwm_ch                 = PWM_TIMER4_OPCH,
            .freq                   = 300000,
            .duty                   = 80,
            .point_bit              = 0,
        },
    };
    // 1.open 第二个传参为NULL则使用board.h板级文件进行初始化，若不为NULL按照自定义配置进行初始化
    pwm_dev_handl = dev_open("pwm1", &pwm);
    // pwm_dev_handl = dev_open("pwm1", NULL);
    if (!pwm_dev_handl) {
        printf("open pwm err !!!\n\n");
        return;
    }
    printf("pwm: ch=0x%x,duty=%2.2f%%,pbit=%d,freq=%dhz\n", pwm.pwm_config.pwm_ch, \
           pwm.pwm_config.duty, \
           pwm.pwm_config.point_bit, \
           pwm.pwm_config.freq);
    os_time_dly(500);

    /*
     * open PWM设备之后就会初始化PWM，PWM相关参数为board.c配置，在不需要更改参数
     * 时，只需要open就行，不需要进行以下操作。当改参数时，需要注意的是，除开
     * ADD和REMOVE通道，其余CMD命令传入参数均为pwm_config_t *，主要区分该结构体
     * 成员的pwm_ch，修改哪个通道的参数就要配置好对应的pwm_ch
     */

#if 1
    /*2.ioctl 配置与获取占空比*/
    pwm.pwm_config.duty = 80;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    pwm.pwm_config.duty = 0;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_GET_DUTY, (u32)&pwm.pwm_config);//获取占空比，返回数值在传入形参的pwm_config.duty
    printf("pwm1 read duty : %.2f \n", pwm.pwm_config.duty);
    os_time_dly(200);

    /* 3.ioctl控制PWM暂停、运行、正反向，均支持多通道控制 */
    printf("----pwm1 ioctl-------\n\n");
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_STOP, (u32)&pwm.pwm_config);//PWM停止
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_RUN, (u32)&pwm.pwm_config);//PWM运行
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_REVDIRC, (u32)&pwm.pwm_config);//PWM正向
    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_FORDIRC, (u32)&pwm.pwm_config);//PWM反向

    /* pwm.pwm_config.deathtime = 6;//最大值31 死区时间为系统时钟的(deathtime+1)倍,使用PWMCHx_H/L有效 */
    /* dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DEATH_TIME, (u32)&pwm.pwm_config);//PWM死区时间设置 */

    os_time_dly(200);
    printf("----pwm1 set freq-------\n\n");
    /*4.ioctl配置频率和占空比，以下传参如果没有额外修改pwm_ch就是修改PWMCH0_H和PWMCH0_L的参数*/
    pwm.pwm_config.pwm_ch = PWMCH0_H | PWMCH0_L;
    pwm.mcpwm_port[0] = IO_PORTC_00;
    pwm.mcpwm_port[1] = IO_PORTC_01;
    pwm.pwm_config.freq = 2000;
    pwm.pwm_config.duty = 20;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_FREQ, (u32)&pwm.pwm_config);//设置频率
    os_time_dly(200);

    pwm.pwm_config.duty = 50;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    os_time_dly(200);

    pwm.pwm_config.duty = 80;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    os_time_dly(200);

    /*5.中途可以添加TIMER2 PWM 任意IO,添加通道后关闭前必须删除 */
    printf("----timer add channel-------\n\n");
    pwm.timer_pwm_port[0] = IO_PORTB_15;
    pwm.pwm_config.pwm_ch = PWM_TIMER2_OPCH;
    pwm.pwm_config.duty = 10;
    pwm.pwm_config.freq = 1500;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_ADD_CHANNEL, (u32)&pwm);//中途添加通道，可以是PWMCHx_H/L和PWM_TIMER2_OPCH2或PWM_TIMER3_OPCH3

    pwm.pwm_config.duty = 60;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    pwm.pwm_config.duty = 0;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_GET_DUTY, (u32)&pwm.pwm_config);
    printf("pwm1 read duty : %.2f \n", pwm.pwm_config.duty);

    os_time_dly(200);


    printf("----timer ioctl pwm.pwm_ch = 0x%x-------\n\n", pwm.pwm_config.pwm_ch);
    /*6.ioctl控制PWM暂停、运行、正反向,调用1次ioctl只支持1组通道PWMCH_H/L控制*/
    os_time_dly(300);
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_STOP, (u32)&pwm.pwm_config);//PWM停止

    os_time_dly(200);
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_RUN, (u32)&pwm.pwm_config);//PWM运行
    os_time_dly(200);

    pwm.pwm_config.freq = 2000;
    pwm.pwm_config.duty = 20;
    printf("----timer set freq-------\n\n");
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_FREQ, (u32)&pwm.pwm_config);//设置频率
    os_time_dly(200);
    pwm.pwm_config.duty = 50;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    os_time_dly(200);
    pwm.pwm_config.duty = 80;
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_DUTY, (u32)&pwm.pwm_config);//设置占空比
    os_time_dly(200);

    /*7.关闭前把添加通道删除,*/
    dev_ioctl(pwm_dev_handl, IOCTL_PWM_SET_REMOV_CHANNEL, (u32)&pwm);
#endif
    dev_close(pwm_dev_handl);
    printf("pwm test end\n\n");

    while (1) {
        os_time_dly(2);
    }
}

static int c_main_pwm(void)
{
    os_task_create(pwm_test_task, NULL, 12, 1000, 0, "pwm_test_task");
    return 0;
}

late_initcall(c_main_pwm);

#endif ///< USE_PWM_TEST_DEMO
