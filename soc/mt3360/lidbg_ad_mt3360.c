
#include "lidbg.h"
#include "ac83xx_keyadc.h"

unsigned int  soc_ad_read(unsigned int channel)
{
    unsigned int ret = 0;
    int i = 0;

    if((channel < 0) || (channel > 5))
    {
        //		printk("***** Error: unvalid channel %d!*****\n", channel);
        return ret;
    }

    ret = AuxGetKeyPadDat(channel);
    //	printk("***** adc read channel = 0x%x, voltage = %d *****\n",channel, ret);

    return ret;
}

void  soc_ad_init(void)
{
    printk("***** adc init *****\n");
}

EXPORT_SYMBOL(soc_ad_init);
EXPORT_SYMBOL(soc_ad_read);


