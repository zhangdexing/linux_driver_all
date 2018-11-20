
#include "lidbg_servicer.h"
#include "lidbg_insmod.h"

#include <cutils/properties.h>
#include <sys/statfs.h>
#define TAG "dlidbg_load:"
static int boot_completed = 0;

int getPathFreeSpace(char *path)
{
    struct statfs diskInfo;
    statfs(path, &diskInfo);
    unsigned long long totalBlocks = diskInfo.f_bsize;
    unsigned long long freeDisk = diskInfo.f_bfree * totalBlocks;
    size_t mbFreedisk = freeDisk >> 20;
    return mbFreedisk;
}

static void *thread_check_boot_complete(void *data)
{
    data = data;
    while(1)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("sys.boot_completed", value, "0");
        if (value[0] == '1' && is_file_exist("/dev/lidbg_interface"))
        {
            lidbg( TAG" send message  :boot_completed = %c,delay 2S \n", value[0]);
            sleep(2);
            
            LIDBG_WRITE("/dev/lidbg_interface", "BOOT_COMPLETED");
            boot_completed = 1;
            break;
        }
        lidbg( TAG" send message  :boot_completed = %c,delay 2S \n", value[0]);
        sleep(1);
    }
    return ((void *) 0);
}

char value[PROPERTY_VALUE_MAX];
int main(int argc, char **argv)
{
    pthread_t ntid;
    argc = argc;
    argv = argv;
    pthread_t lidbg_uevent_tid;
    int recovery_mode, checkout = 0; //checkout=1 origin ; checkout=2 old flyaudio;checkout=3 new flyaudio
    int ret;

    lidbg(TAG"20171129.selinux.bootcom.\n");

    //system("setenforce 0");
    system("chmod 777 /dev/dbg_msg");
    system("logcat -b main -G 10M");
    system("logcat -b crash -G 1M");
    pthread_create(&ntid, NULL, thread_check_boot_complete, NULL);

    if(is_file_exist("/data/coldBootLogcat.txt"))
    {
        lidbg("lidbg_iserver: start coldBootLogcat.in./data/coldBootlogcat.txt\n");
        system("mkdir /data/logcat_pre");
        system("mv /data/*.txt /data/logcat_pre/");
        system("logcat -b main -b system -v threadtime -f /data/coldBootlogcat.txt &");
        system("chmod 777 /data");
        system("chmod 777 /data/coldBootlogcat.txt");
        system("chmod 777 /data/logcat_pre");
        system("chmod 777 /data/logcat_pre/*");
        lidbg("lidbg_iserver: start coldBootLogcat.out\n");
    }


    DUMP_BUILD_TIME_FILE;
    lidbg(TAG"Build Time:iserver start\n");
//    system("mkdir /dev/log");
//    system("chmod 777 /dev/log");
    if(is_file_exist("/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter"))
        system("cat /dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter > /dev/flyparameter");
    else if(is_file_exist("/dev/block/bootdevice/by-name/flyparameter"))
        system("cat /dev/block/bootdevice/by-name/flyparameter > /dev/flyparameter");
    else
        system("cat /flysystem/lib/out/flyparameter > /dev/flyparameter");

    system("chmod 444 /dev/flyparameter");

//    system( "mount -o rw,remount rootfs /" );
//    system( "chmod 777 /flyconfig" );
//    system( "/flysystem/bin/decodeFlyconfig" );

    module_insmod("/system/lib/modules/out/lidbg_immediate.ko");
    module_insmod("/flysystem/lib/out/lidbg_immediate.ko");
#if 0
    //wait flysystem mount
    while(is_file_exist("/flysystem/lib") == 0)
    {
        static int cnt = 0;
        if(is_file_exist("/sbin/recovery"))
        {
            lidbg(TAG"this is flyaudio recovery\n");
            break;
        }
        sleep(1);
        if(++cnt >= 5)
            break;
    }
#endif

    if(is_file_exist("/flysystem/lib/out/lidbg_loader.ko"))
    {
        if(is_file_exist("/system/etc/build_origin"))
        {
            checkout = 1;
            lidbg(TAG"this is origin system\n");
        }
        else
        {
            checkout = 2;
            lidbg(TAG"this is old flyaudio system\n");
        }
    }
    else if(is_file_exist("/system/vendor/lib/out/lidbg_loader.ko"))
    {
        checkout = 3;
        lidbg(TAG"this is new flyaudio system\n");

    }
    else
    {
        checkout = 1;
        lidbg(TAG"this is origin system\n");
    }

    if(is_file_exist("/sbin/recovery"))
    {
        recovery_mode = 1;
        checkout = 1;
        lidbg(TAG"recovery_mode=1\n=====force use origin system=====\n");
    }
    else
    {
        recovery_mode = 0;
    }

//    system("mkdir /data/lidbg");
//    system("mkdir /data/lidbg/lidbg_osd");
//    system("chmod 777 /data/lidbg");
//    system("chmod 777 /data/lidbg/lidbg_osd");

    if(checkout == 1)
    {
        if(is_file_exist("/system/etc/build_origin") && (recovery_mode == 0))
        {
            system("mount -o remount /flysystem");
            system("rm -rf /flysystem/bin");
            system("rm -rf /flysystem/lib");
            system("rm -rf /flysystem/app");
        }

        module_insmod("/system/lib/modules/out/lidbg_uevent.ko");
        module_insmod("/system/lib/modules/out/lidbg_loader.ko");
        //system("insmod /system/lib/modules/out/lidbg_uevent.ko");
        //system("insmod /system/lib/modules/out/lidbg_loader.ko");
        while(1)
        {
            if(access("/system/lib/modules/out/lidbg_userver", X_OK) == 0)
            {
                system("/system/lib/modules/out/lidbg_userver &");
                lidbg(TAG"origin userver start\n");
                break;
            }
            system("mount -o remount /system");
            system("chmod 777 /system/lib/modules/out/lidbg_userver");
            system("chmod 777 /system/lib/modules/out/*");
            lidbg(TAG"waitting origin lidbg_uevent...\n");
            sleep(1);
        }
    }
    else if(checkout == 3)
    {
        system("mkdir -p /flysystem/lib/out");
        system("chmod 777 /flysystem");
        system("chmod 777 /flysystem/lib");
        system("chmod 777 /flysystem/lib/out");

        system("cp -rf /system/vendor/lib/out/* /flysystem/lib/out/*");
        checkout = 2;
    }

    if(checkout == 2)
    {

        module_insmod("/flysystem/lib/out/lidbg_uevent.ko");
        module_insmod("/flysystem/lib/out/lidbg_loader.ko");
        //module_insmod("/flysystem/lib/out/lidbg_hal_comm.ko");
        //module_insmod("/flysystem/lib/out/lidbg_module_comm.ko");
        //system("insmod /flysystem/lib/out/lidbg_uevent.ko");
        //system("insmod /flysystem/lib/out/lidbg_loader.ko");
        while(1)
        {
            if(access("/flysystem/lib/out/lidbg_userver", X_OK) == 0)
            {
                system("/flysystem/lib/out/lidbg_userver &");
                lidbg(TAG"flyaudio userver start\n");
                break;
            }
            system("chmod 777 /flysystem/lib/out/lidbg_userver");
            system("chmod 777 /flysystem/lib/out/*");
            lidbg(TAG"waitting flyaudio lidbg_uevent...\n");
            usleep(100 * 1000);
        }
    }

    sleep(1);
    system("chmod 777 /dev/lidbg_uevent");

    if(recovery_mode == 1)
    {
        sleep(5);// wait for ts load
        system("setprop service.recovery.start 1");
        //system("/sbin/recovery &");
    }

    //disk space full reboot
#if 1
    sleep(5 * 60); // wait for ts load
    while(1)
    {
        property_get("persist.fly.system.cleanup", value, "0");
        lidbg(TAG"persist.fly.system.cleanup = %c\n", value[0]);
        if (value[0] != '1')
        {
            break;
        }
        sleep(1);
    }
    while(1)
    {
        int size;
        property_get("lidbg.fly.debugmode", value, "0");
        if (value[0] == '1')
        {
            sleep(10 * 60);
            continue;
        }
        size = getPathFreeSpace("/data");
        if(( size  < 100) && (size  != 0))
        {
            lidbg(TAG"getPathFreeSpace:%d\n", size);
            //system("echo ws toast data_size_low 1 > /dev/lidbg_pm0");
            //sleep(2);
            system("reboot data_size_low");
        }
        sleep(1 * 60);
    }
#endif
    return 0;
}
