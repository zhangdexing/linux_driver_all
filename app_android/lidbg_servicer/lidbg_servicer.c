#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils/Log.h>
#include "lidbg_servicer.h"

//mount yaffs2 mtd@system /system rw remount
//...
//   chmod 777 /dev/lidbg_servicer
//    chmod 777 /dev/mlidbg0
//    chmod 777 /system/bin/lidbg_servicer

//service lidbg_servicer /system/bin/logwrapper /system/bin/lidbg_servicer
//    class late_start
//    user root
//    group root


#define LOG_COUNT_MAX (16)



#define SERVICER_DONOTHING  (0)
#define LOG_DMESG  (1)
#define LOG_LOGCAT (2)
#define LOG_ALL (3)
#define LOG_CONT    (4)

#define WAKEUP_KERNEL (10)

#define LOG_DVD_RESET (64)
#define LOG_CAP_TS_GT811 (65)
#define LOG_CAP_TS_FT5X06 (66)
#define LOG_CAP_TS_FT5X06_SKU7 (67)
#define LOG_CAP_TS_RMI (68)
#define LOG_CAP_TS_GT801 (69)
#define  CMD_FAST_POWER_OFF (70)

#define UMOUNT_USB (80)
//#define  DEBUG_USB_RST


pthread_t ntid;
int fd = 0;


int  servicer_handler(int signum)
{

    int cmd = 0;
    static int count = 0;
    int readlen;
    printf("servicer_handler+\n");




    readlen = read(fd, &cmd, 4);
	//printf("fd=%x,readlen=%d,cmd=%d\n",fd,readlen,cmd);
    LOGW("[futengfei]  fd=%x,readlen=%d,cmd=%d",fd,readlen,cmd);
    if(cmd != SERVICER_DONOTHING)

    {



        printf("cmd = %d\n", cmd);



        switch(cmd)
        {
        case LOG_DMESG:
        case LOG_LOGCAT:
		case LOG_ALL:
		case LOG_CONT:
        {
            if(count == 0)

            {
                system("date > /sdcard/log_logcat.txt");
                system("date > /sdcard/log_dmesg.txt");
				system("date > /sdcard/all_mesg.txt");
                //system("date > /sdcard/log_dmesg_lidbg.txt");

                //system("date > /sdcard/tflash/log_dmesg.txt");
            }
            count ++;
            printf("count=%d\n", count);

            if(count > LOG_COUNT_MAX)
            {
                printf("log count>%d break!\n", LOG_COUNT_MAX);

                break;
            }
            if(LOG_ALL == cmd)
            {
                system("date >> /sdcard/all_mesg.txt");
				system("logcat -f /dev/kmsg & dmesg >/sdcard/all_mesg.txt");

			}
            if(LOG_CONT == cmd)
            {
                
				system("logcat -f /dev/kmsg & cat /proc/kmsg >/sdcard/all_mesg.txt");

			}

            if(LOG_DMESG == cmd)
            {
                //printf("LOG_DMESG == cmd\n");

                system("date >> /sdcard/log_dmesg.txt");

                system("dmesg >> /sdcard/log_dmesg.txt");

                //system("date >> /sdcard/log_dmesg_lidbg.txt");

                //system("dmesg | grep -irn lidbg >> /sdcard/log_dmesg_lidbg.txt");

                //system("date >> /sdcard/tflash/log_dmesg.txt");

                // system("dmesg >> /sdcard/tflash/log_dmesg.txt");

            }

            else if(LOG_LOGCAT == cmd)
            {

                //printf("LOG_LOGCAT == cmd\n");
                system("date >> /sdcard/log_logcat.txt");
                system("logcat >> /sdcard/log_logcat.txt");
                //system("logcat | grep -irn lidbg >> /sdcard/log_logcat_lidbg.txt");



            }
            break;

        }
        //cap_ts
        case LOG_CAP_TS_GT811:
        case LOG_CAP_TS_FT5X06_SKU7:
        case LOG_CAP_TS_FT5X06:
        case LOG_CAP_TS_RMI:
		case LOG_CAP_TS_GT801:
		case CMD_FAST_POWER_OFF :
        {
            if(LOG_CAP_TS_GT811 == cmd)

            {
#if 1

                system("insmod /system/lib/modules/out/gt811.ko");
                system("insmod /flysystem/lib/out/gt811.ko");



#else
                void *module;
                module = read_file("/lib/modules/out/rmi_touch.ko", &size);
                if (!module)
                    printk("read_file rmi_touch.ko fail\n");
                else
                {
                    ret = init_module(module, size, "");
                    if(ret < 0)
                        printk("init_module rmi_touch.ko fail\n");
                    free(module);
                }

#endif
            }

            else if (LOG_CAP_TS_FT5X06 == cmd)
            {
                system("insmod /system/lib/modules/out/ft5x06_ts.ko");
                system("insmod /flysystem/lib/out/ft5x06_ts.ko");

            }

            else if (LOG_CAP_TS_FT5X06_SKU7 == cmd)
            {
                system("insmod /system/lib/modules/out/ft5x06_ts_sku7.ko");

            }

            else if (LOG_CAP_TS_RMI == cmd)
            {
                system("insmod /system/lib/modules/out/rmi_touch.ko");

            }
            else if (LOG_CAP_TS_GT801 == cmd)
            {
                system("insmod /system/lib/modules/out/gt801.ko");
                system("insmod /flysystem/lib/out/gt801.ko");

            }
            else if ( CMD_FAST_POWER_OFF == cmd)
            {
                system("am broadcast -a android.intent.action.FAST_BOOT_START");
            }
			
            //sleep(10);//delay to mount sdcard
            //system("dmesg > /sdcard/log_cap_ts_dmesg.txt");

            break;

        }

		case UMOUNT_USB:
		{

			system("umount /mnt/usbdisk");
			break;

		}
		case WAKEUP_KERNEL:
		{
			system("su");
			system("echo on > /sys/power/state");
			break;

		}

	  
        }
    }

    printf("servicer_handler-\n");
	return cmd;
}




int main(int argc , char **argv)
{
    int err = 0;
    int cmd = 0;
    int count = 0;
    int oflags;

    printf("lidbg_servicer start\n");
    //sleep(5);

    system("insmod /system/lib/modules/out/lidbg_ts_to_recov.ko");
    system("insmod /system/lib/modules/out/lidbg_common.ko");
    system("insmod /system/lib/modules/out/lidbg_servicer.ko");
    system("insmod /system/lib/modules/out/lidbg_touch.ko");
    system("insmod /system/lib/modules/out/lidbg_key.ko");
    system("insmod /system/lib/modules/out/lidbg_i2c.ko");
    system("insmod /system/lib/modules/out/lidbg_soc_msm8x25.ko");
    system("insmod /system/lib/modules/out/lidbg_io.ko");
    system("insmod /system/lib/modules/out/lidbg_ad.ko");
    system("insmod /system/lib/modules/out/lidbg_fly_soc.ko");
    system("insmod /system/lib/modules/out/lidbg_soc_devices.ko");
    system("insmod /system/lib/modules/out/lidbg_videoin.ko");
    system("insmod /system/lib/modules/out/lidbg_main.ko");
    system("insmod /system/lib/modules/out/lidbg_to_bpmsg.ko");

    //for flycar
    system("insmod /flysystem/lib/out/lidbg_ts_to_recov.ko");
    system("insmod /flysystem/lib/out/lidbg_common.ko");
    system("insmod /flysystem/lib/out/lidbg_servicer.ko");
    system("insmod /flysystem/lib/out/lidbg_touch.ko");
    system("insmod /flysystem/lib/out/lidbg_key.ko");
    system("insmod /flysystem/lib/out/lidbg_i2c.ko");
    system("insmod /flysystem/lib/out/lidbg_soc_msm8x25.ko");
    system("insmod /flysystem/lib/out/lidbg_io.ko");
    system("insmod /flysystem/lib/out/lidbg_ad.ko");
    system("insmod /flysystem/lib/out/lidbg_fly_soc.ko");
    system("insmod /flysystem/lib/out/lidbg_soc_devices.ko");
    system("insmod /flysystem/lib/out/lidbg_main.ko");
    system("insmod /flysystem/lib/out/lidbg_to_bpmsg.ko");


    sleep(1);
    system("chmod 0777 /dev/mlidbg0");
    system("chmod 0777 /dev/lidbg_servicer");

open_dev:
    fd = open("/dev/lidbg_servicer", O_RDWR);
    //printf("fd = %x\n",fd);
    if((fd == 0xffffffff) || (fd == 0))
    {
        printf("open lidbg_servicer fail\n");
        sleep(1);//delay wait for /dev/lidbg_servicer to creat
        //usleep(1000 * 100); //100ms
        goto open_dev;
    }

    //system("date > /sdcard/log_dmesg.txt");
    //system("date > /sdcard/log_dmesg_lidbg.txt");
    //system("date > /sdcard/log_logcat.txt");

#if 0	//block_read
    while(1)
    {
        servicer_handler(0);

    }
#else//sigal read
    signal(SIGIO, servicer_handler); //让input_handler()处理SIGIO信号
    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC); //调用驱动的fasync_helper

#endif


	//clear fifo
	while(1)
	{
		if(servicer_handler(0) == 0xffffffff)
			break;
	}

    system("insmod /system/lib/modules/out/lidbg_ts_probe.ko");
    system("insmod /flysystem/lib/out/lidbg_ts_probe.ko");
#if 1
    //for flycar
    sleep(1);
    system("insmod /flysystem/lib/modules/FlyDebug.ko");
    system("insmod /flysystem/lib/modules/FlyMmap.ko");
    system("insmod /flysystem/lib/modules/FlyPM.ko");
    system("insmod /flysystem/lib/modules/FlyPMDevice.ko");
    system("insmod /flysystem/lib/modules/FlyADC.ko");
    system("insmod /flysystem/lib/modules/FlySpeed.ko");
    system("insmod /flysystem/lib/modules/FlyBrake.ko");
    system("insmod /flysystem/lib/modules/FlyHardware.ko");
    system("insmod /flysystem/lib/modules/FlyHardwareDevice.ko");
    system("insmod /flysystem/lib/modules/FlyAudio.ko");
    system("insmod /flysystem/lib/modules/FlyAudioDevice.ko");
    system("insmod /flysystem/lib/modules/productinfo.ko");
    system("insmod /flysystem/lib/modules/vendor_flyaudio.ko");

    //chegnweidong
    system("insmod /flysystem/lib/mdrv/flysemdriver.ko");

    sleep(1);
    system("chmod 0777 /dev/FlyDebug");
    system("chmod 0777 /dev/FlyMmap ");
    system("chmod 0777 /dev/FlyHardware");
    system("chmod 0777 /dev/FlyAudio");
    system("chmod 0666 /dev/FlySpeed");
    system("chmod 0666 /dev/FlyBrake");

    //chegnweidong
    system("chmod 0777 /dev/flysemdriver");

#endif

#ifdef DEBUG_USB_RST	
{

		int err = 0;
		int errsd = 0;
		int retry=0;
		int tell_usb_rst=88;
		int print_count =0;
		static unsigned int usb_rst_count =0;
		char path[50];
		
		memset(path,0,50);
		sleep(60);
    while(1)
    {
        sleep(2);
        if(retry<50)
        	{
			err = access("/mnt/usbdisk/text1" , F_OK); 
				if ( err !=0 )
					{
						 LOGW("[futengfei]  makedir usb");
						 mkdir("/mnt/usbdisk/text1", 0777);
					}
				else 
					{ 
						 LOGW("[futengfei]  notmake usb");
					}   
			err = access("/mnt/sdcard/text1" , F_OK); 
				if ( err !=0 )
					{
						 LOGW("[futengfei]  makedir sdc");
						 mkdir("/mnt/sdcard/text1", 0777);
					}
				else 
					{ 
						 LOGW("[futengfei]  notmake sdc");
					}   
			sprintf(path,"%s","/mnt/usbdisk/text1");
			err = access (path,R_OK);
			sprintf(path,"%s","/mnt/sdcard/text1");
			errsd=access (path,R_OK);
				if(err == 0&&errsd == 0)
					{						
						write(fd, &tell_usb_rst, 4);
						retry=0;
						usb_rst_count++;
						LOGW("S[access] read file ok!sucess:=============111111111111111111111==================[%d]times ",usb_rst_count);
					}
				else  
					{
						retry++;
						LOGW("F[access] read file no!false::======22222====retry=[%d] err=[%d],errsd=[%d]",retry,err,errsd);
					}

				
		}	
	else
		{
		print_count++;//less than 10 times print
		if(print_count<10)
		LOGW("F[futengfei]==can't read file  stop and wait;==========222222222222222222=============sucess:[%d]times ",usb_rst_count);
		}
	}
}
#else

    while(1)
    {
        sleep(60);

    }

#endif	

    return 0;
}
