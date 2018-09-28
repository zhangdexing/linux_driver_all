#include "../soc.h"
#include <pm8x41_adc.h>
#include "fly_target.h"

uint32_t fly_get_adc(uint16_t ch_num , uint16_t mpp_num)
{
    uint32_t calib_result = 0;
    pm8x41_enable_mpp_as_adc(mpp_num);
    calib_result = pm8x41_adc_channel_read(ch_num);
    return calib_result;
}

int adc_get(void)
{
    int ac_ch_val[ADC_KEY_CHNL] = {0};
    int i = 0;

#if (defined BOOTLOADER_MSM8909) || (defined BOOTLOADER_MSM8996)
    return 0;
#else
    for(i = 0; i < ADC_KEY_CHNL; i++)
    {
        ac_ch_val[i] = fly_get_adc(g_bootloader_hw.adc_info[i].ad_ch, g_bootloader_hw.adc_info[i].ad_ctrl_ch);
        if(ac_ch_val[i] <= g_bootloader_hw.adc_info[i].ad_vol)
            return 1;
    }

    return 0;
#endif
}
