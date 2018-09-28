
#include "lidbg.h"
#include "pwm.h"

#define PWM_DBG
#define BACKIGHT_PWM_ID 1
static struct pwm_device *pwm = NULL;
static pwm_config_code pwmConfig = {0};

unsigned int soc_pwm_set(int pwm_id, int duty_ns, int period_ns)
{
    return 1;
}

void soc_bl_init(void)
{
    int ret = 0;
    /*
    * PWM Frequency: f = BCLK/((PWMP+1)*(PWMRSN+1))
    * Duty Ratoin: High/(High+Low) = ((PWMH+1)/(PWMRSN+1))*100%
    */
#if 0
    pwmConfig.clk_id = CLKSRC_27M;
    pwmConfig.pin_id = 1;

    pwmConfig.pwm_en = 1;
    pwmConfig.pwm_mode = NORMAL_TRI_MODE;
    pwmConfig.pwm_high = 599;
    pwmConfig.pwm_rsn = 999;
    pwmConfig.pwm_prescale = 8;

    pwm = pwm_request(BACKIGHT_PWM_ID, "backlight");
    if(!pwm)
        lidbg("Request pwm failed !\n");

    memcpy((void *) & (pwm->pwm_cfg_code), (void *)&pwmConfig, sizeof(pwm_config_code));

    ret = pwm_config(pwm, 0, 0);//duty_ns, period_ns are not used
    if(!ret)
        lidbg("Config pwm failed !\n");
#endif
    //	pwm_enable(pwm);
}

/* bl_level: 1 ~ 100 */
unsigned int   soc_bl_set(u32 bl_level)
{
#if 0
    bool ret = 0;
    lidbg("<****** soc_bl_set bl_level = %d ******>\n", bl_level);

    ret = pwm_set_intensity(pwm, bl_level);
    if(!ret)
        lidbg("Set bl level %d failed !\n", bl_level);

    ret = pwm_mode_switch(pwm, pwm->pwm_cfg_code.pwm_mode);
    if (!ret)
        printk("Set mode failed !\n");

#ifdef PWM_DBG
    pwm_dump(BACKIGHT_PWM_ID);
#endif
#else
#define	FLY_IO_BASE_VA	0xFD000000
#define WriteREG(arg, val) *(volatile UINT32 *)(FLY_IO_BASE_VA + (arg)) = val
#define ReadREG(arg)       *(volatile UINT32 *)(FLY_IO_BASE_VA + (arg))
#define WriteREGMsk(arg, val, msk) WriteREG((arg), (ReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define WriteRegMsk(arg, val, msk) WriteREG((arg), (ReadREG(arg) & (~(msk))) | ((val) & (msk)))
    //static void flySetPWM(UINT32 dty_cyc)
    //{
    UINT32 u4PwmRsn = 0xFFF;
    UINT32 u4PwmP = 0x4;
    UINT32 u4PwmH = 0x800;
    UINT32 u4PwmH_FB = 0x800;
    UINT32 dty_cyc = bl_level;

    //	if(bl_level <= 100)
    //		dty_cyc = 100 - bl_level;
    //	else
    //		dty_cyc = 50;

    /************************************************************
       *                   Step3:  Configure Backlight PWM                               *
       ************************************************************/

    u4PwmRsn = (269) & 0xFFF;
    dty_cyc  = (dty_cyc > 100) ? 100 : dty_cyc;
    u4PwmH = (dty_cyc * u4PwmRsn / 100) & 0xFFF;
    u4PwmH_FB = (( 100 - dty_cyc) * u4PwmRsn / 100) & 0xFFF;

    WriteRegMsk(0x3220C, (u4PwmRsn << 20) | (u4PwmH << 8) | (4 << 2) | (1 << 0), \
                (0xFFF << 20) | (0xFFF << 8) | (0x3F << 2) | (1 << 0)); // Config EN (pwm3) to 2K Hz : PWMRSN(269) PWMP(49) PWMH(dty_cyc * PWMRSN)
    WriteRegMsk(0x32204, (u4PwmRsn << 20) | (u4PwmH_FB << 8) | (4 << 2) | (1 << 0), \
                (0xFFF << 20) | (0xFFF << 8) | (0x3F << 2) | (1 << 0)); // Config FB (pwm1) to 20K Hz: PWMRSN(269) PWMP(4) PWMH(dty_cyc * PWMRSN)
    WriteRegMsk(0x32218, (1 << 2) | (1 << 6), (1 << 2) | (1 << 6)); // PWM Output trigger

    lidbg("fly flySetPWM %d \n", dty_cyc );
    //}
#endif
    return 0;
}

EXPORT_SYMBOL(soc_bl_set);
EXPORT_SYMBOL(soc_pwm_set);

