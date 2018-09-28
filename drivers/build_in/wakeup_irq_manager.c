
//	Manage.c (kernel\irq)


//	flyaudio+
//	#include <../drivers/flyaudio/wakeup_irq_manager.c>
{
    static char *white_list[] =
    {
        "qpnp_kpdpwr_status",
        "play",
        "qcom,smd-modem",
        NULL,
    };
    static int irqall = 0, irqskipd = 0, j = 0;
    if (desc->action && desc->action->name)
    {
        irqall++;
        for(j = 0; white_list[j] != NULL; j++)
        {
            if(strncmp(desc->action->name, white_list[j], strlen(white_list[j]) - 1) == 0)
            {
                printk( KERN_CRIT  "wakeupirq.name.protect:%d/%d[%s]\n", irqskipd, irqall, desc->action->name);
                break;
            }
        }
        if (white_list[j] == NULL)
        {
            irqskipd++;
            printk( KERN_CRIT  "wakeupirq.name.skip:%d/%d[%s]\n", irqskipd, irqall, desc->action->name);
            irq_put_desc_busunlock(desc, flags);
            return 0;
        }
    }
    else
        printk( KERN_CRIT "wakeupirq.name.null:[null,%d]\n", irq);
}


