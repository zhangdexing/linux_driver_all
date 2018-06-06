
#ifndef _MISC_DEV_SOC__
#define _MISC_DEV_SOC__

typedef enum
{
    ALL_PORT_ON,
    ALL_PORT_OFF,
    PORT1_ON,//udisk
    PORT1_OFF,
    PORT2_ON,//usb camera front
    PORT2_OFF,
    PORT3_ON,//usb camera back
    PORT3_OFF,
} USB_PORT_CTRL;

void lidbg_device_main(int argc, char **argv);
int thread_key(void *data);
int thread_button_init(void *data);
int thread_led(void *data);
int thread_thermal(void *data);
void led_resume(void);
void led_suspend(void);
void led_time_set(int on,int off);
void led_heartbeat_set(int times);
void temp_init(void);

int thread_sound_detect(void *data);
int sound_detect_init(void);
int SOC_Get_System_Sound_Status_func(void *para, int length);
int  iGPS_sound_status(void);
void set_system_performance(int type);
void cb_kv_show_temp(char *key, char *value);
int thread_antutu_test(void *data);
void set_system_performance(int type);
int thread_sound_dsp_init(void *data);




// PANNE_PEN
#define LCD_ON  do{if((g_var.led_hal_status == 0) || (g_var.led_app_status == 0)){lidbg("LCD_ON break\n"); break;}lidbg("LCD_ON\n");LPC_CMD_LCD_ON;}while(0)
#define LCD_OFF do{if(g_var.keep_lcd_on) break;lidbg("LCD_OFF\n");LPC_CMD_LCD_OFF;}while(0)

#endif
