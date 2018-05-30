
#include "lidbg.h"
static int led_on_delay = 1000;
static int led_off_delay = 1000;

void led_on(void)
{
    LED_ON;
    msleep(led_on_delay);
    LED_OFF;
    msleep(led_off_delay);
}


int thread_led(void *data)
{
    int led_en ;
    FS_REGISTER_INT(led_en, "led_en", 1, NULL);
    if(led_en)
    {
        while(1)
        {
            led_on();
        }
    }
    return 0;
}
void led_resume(void)
{
    IO_CONFIG_OUTPUT(0, LED_GPIO);
    LED_ON;
}

void led_suspend(void)
{
    LED_OFF;
}
void led_time_set(int on,int off)
{
    led_on_delay=on;
    led_off_delay=off;
}

