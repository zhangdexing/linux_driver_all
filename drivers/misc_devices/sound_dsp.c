
#include "lidbg.h"

#ifdef MUC_CONTROL_DSP

int thread_sound_dsp_init(void *data)
{
    DUMP_FUN;
    LPC_CMD_RADIORST_L;
    msleep(100);
    LPC_CMD_RADIORST_H;
    msleep(100);
    LPC_CMD_RADIO_INIT;
    msleep(2000);
    LPC_CMD_RADIO_SET;
    msleep(2000);
    LPC_CMD_RADIO_INIT2;
    msleep(2000);
    LPC_CMD_RADIO_INIT3;
    return 0;
}
#endif
