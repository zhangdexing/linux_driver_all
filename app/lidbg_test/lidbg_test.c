
#include "lidbg_servicer.h"


void *thrd_monitor_app(void *arg)
{

    while(1)
    {
        system("ps | grep ftf >  /dev/watchdog_app.txt");
        sleep(1);
        if(get_file_size("/dev/watchdog_app.txt") == 0)
        {
            system("am broadcast -a com.mypftf.mobile.alive");
            printf("am broadcast -a com.mypftf.mobile.alive\n");
        }
        sleep(10);
    }
}


pthread_t tid;

//"cat /sys/power/autosleep"    return off or mem
#define SYSTEM_STATE "/sys/power/autosleep"
#define AC_CHARGE_STATE "/sys/class/power_supply/ac/online"
#define USB_CHARGE_STATE "/sys/class/power_supply/usb/online"

int main(int argc, char **argv)
{

    int ret;
    int fd_autosleep = 0, fd_ac_state = 0, fd_usb_state = 0;
    char  buff[16];

    printf("enter %s\n", __FILE__);
    //system("su");
    //system("chmod 777 /sys/class/leds/button-backlight/brightness");
    system("chmod 777 /sys/class/leds/button-backlight/brightness");
    system("chmod 777 /sys/power/wake_lock");
    system("chmod 777 /sys/power/wake_unlock");
    system("chmod 777 /dev");
    system("chmod 777 /dev");


    printf("build time: %s\n", __TIME__);

    //pthread_create(&tid, NULL, thrd_monitor_app, NULL);

    while(1)
    {
        system("echo lidbg > /sys/power/wake_lock");
        //system("echo 1 > /sys/class/leds/button-backlight/brightness");
        system("echo 255 > /sys/class/leds/button-backlight/brightness");
        sleep(1);


        //system("echo 0 > /sys/class/leds/button-backlight/brightness");
        system("echo 0 > /sys/class/leds/button-backlight/brightness");
        system("echo lidbg > /sys/power/wake_unlock");
        sleep(2);


        if(is_file_exist("/dev/lidbg"))//lidbg_vibrator_warning
        {
            memset(buff, '\0', sizeof(buff));

            fd_autosleep = open(SYSTEM_STATE, O_RDONLY);
            if(fd_autosleep <= 0) printf("open %s err\n", SYSTEM_STATE);
            ret = read( fd_autosleep, buff, sizeof(buff) );
            close(fd_autosleep);

            printf("\n%s=%s", SYSTEM_STATE, buff);
            if(strstr(buff, "mem"))
            {
                memset(buff, '\0', sizeof(buff));

                fd_ac_state =  open(AC_CHARGE_STATE, O_RDONLY);
                if(fd_ac_state <= 0) printf("open %s err\n", AC_CHARGE_STATE);
                ret = read( fd_ac_state, buff, sizeof(buff) );
                close(fd_ac_state);

                printf("%s=%s", AC_CHARGE_STATE, buff);
                if(strstr(buff, "0"))
                {
                    memset(buff, '\0', sizeof(buff));

                    fd_usb_state = open(USB_CHARGE_STATE, O_RDONLY);
                    if(fd_usb_state <= 0) printf("open %s err\n", USB_CHARGE_STATE);
                    ret = read( fd_usb_state, buff, sizeof(buff) );
                    close(fd_usb_state);

                    printf("%s=%s", USB_CHARGE_STATE, buff);
                    if(strstr(buff, "0"))
                        system("echo 100 > /sys/class/timed_output/vibrator/enable");
                }
            }
        }
    }
    return 0;
}

