#define ORIGIN_APP_PATH "/system/.origin_app/"
#define ORIGIN_TMP_PATH "/system/.origin_tmp/"


void cb_kv_lidbg_origin_system(char *key, char *value);
int thread_switch_to_origin_system(void *data)
{
    cb_kv_lidbg_origin_system(NULL, "1");
    return 0;
}
int thread_switch_to_flyaudio_system(void *data)
{
    cb_kv_lidbg_origin_system(NULL, "2");
    return 0;
}
void lidbg_system_switch(bool origin_system)
{
    if(origin_system)
        CREATE_KTHREAD(thread_switch_to_origin_system, NULL);
    else
        CREATE_KTHREAD(thread_switch_to_flyaudio_system, NULL);
}


void cb_kv_lidbg_origin_system(char *key, char *value)
{
    LIDBG_WARN("\n\n<------------------system switch -%s----------->\n\n", value[0] == '1' ? "origin system" : "flyaudio system");
    if(value && *value == '1')//origin
    {
        lidbg_shell_cmd("mount -o remount /system");
        lidbg_shell_cmd("mount -o remount /flysystem");
        lidbg_shell_cmd("mkdir -p /flyapdata/.out/temp");
        lidbg_shell_cmd("mkdir -p /flysystem/.out/temp");

        lidbg_shell_cmd("cp  /flyapdata/app/ESFileExplorer.apk /system/app");

        lidbg_shell_cmd("cp  "ORIGIN_APP_PATH"* /system/priv-app");
        lidbg_shell_cmd("cp "ORIGIN_TMP_PATH"* /system/app");
        lidbg_shell_cmd("rm  /system/priv-app/Launcher3.apk");
        while(fs_is_file_exist("/system/priv-app/Launcher3.apk"))
            ssleep(2);
        lidbg_shell_cmd("mv /flyapdata/* /flyapdata/.out/temp");
        lidbg_shell_cmd("mv /flysystem/* /flysystem/.out/temp");
        lidbg_shell_cmd("chmod 777 /system/app/ESFileExplorer.apk");
        lidbg_shell_cmd("chmod 777 /system/app/NfcNci.apk");
        lidbg_shell_cmd("chmod 777 /system/app/F*");
        goto suc;
    }
    else   if(value && *value == '2')//flyaudio
    {
        lidbg_shell_cmd("mount -o remount /system");
        lidbg_shell_cmd("mount -o remount /flysystem");
        lidbg_shell_cmd("mv /flysystem/.out/temp/* /flysystem");
        lidbg_shell_cmd("mv /flyapdata/.out/temp/* /flyapdata");
        lidbg_shell_cmd("cp  /flysystem/app/.sys-app1/* /system/priv-app");
        lidbg_shell_cmd("rm /system/app/ESFileExplorer.apk");
        lidbg_shell_cmd("rm /system/app/NfcNci.apk");
        lidbg_shell_cmd("rm /system/priv-app/Launcher2.apk");
        lidbg_shell_cmd("rm /system/app/FlyBootService.apk");
        lidbg_shell_cmd("rm /system/app/FastBoot.apk");
        while(fs_is_file_exist("/system/app/FastBoot.apk"))
            ssleep(2);
        goto suc;
    }

    LIDBG_WARN("<err>\n");
    return ;
suc:
    lidbg_shell_cmd("chmod 777 /system/priv-app/*");
    lidbg_shell_cmd("rm -r /data");
    ssleep(2);
            lidbg_shell_cmd("reboot");
}


void system_switch_init(void)
{

    lidbg_shell_cmd("echo ====system_switch_init:start==== > /dev/lidbg_msg" );

retry:
    if(SYSTEM_SWITCH_EN == 1)
    {
        if((g_var.is_fly) && (!g_var.recovery_mode))
        {
            if(g_var.is_first_update)
            {
                lidbg_shell_cmd("mount -o remount /system");
                lidbg_shell_cmd("mount -o remount /flysystem");

                lidbg_shell_cmd("chmod 777 /flysystem");
                lidbg_shell_cmd("chmod 777 /flysystem/app/sys-app/*");

                lidbg_shell_cmd("mkdir	"ORIGIN_APP_PATH);
                lidbg_shell_cmd("mkdir	"ORIGIN_TMP_PATH);
                LIDBG_WARN("<====system_switch_init:update====>\n");
                lidbg_shell_cmd("mv /system/priv-app/SystemUI.apk "ORIGIN_APP_PATH"SystemUI.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Contacts.apk "ORIGIN_APP_PATH"Contacts.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Dialer.apk "ORIGIN_APP_PATH"Dialer.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Keyguard.apk "ORIGIN_APP_PATH"Keyguard.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Mms.apk "ORIGIN_APP_PATH"Mms.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Settings.apk "ORIGIN_APP_PATH"Settings.apk" );
                lidbg_shell_cmd("mv /system/priv-app/Launcher2.apk "ORIGIN_APP_PATH"Launcher2.apk" );
                lidbg_shell_cmd("mv /system/app/NfcNci.apk "ORIGIN_TMP_PATH"NfcNci.apk" );
                lidbg_shell_cmd("mv /system/app/FastBoot.apk "ORIGIN_TMP_PATH"FastBoot.apk" );
                lidbg_shell_cmd("mv /system/app/FlyBootService.apk "ORIGIN_TMP_PATH"FlyBootService.apk" );
                lidbg_shell_cmd("mv /system/app/PackageInstaller.apk "ORIGIN_TMP_PATH"PackageInstaller.apk" );
                lidbg_shell_cmd("mv /system/app/Provision.apk "ORIGIN_TMP_PATH"Provision.apk" );
                lidbg_shell_cmd("chmod 777 /system/priv-app/*" );
                lidbg_shell_cmd("chmod 777 /flysystem/app/*" );
                lidbg_shell_cmd("chmod 777  "ORIGIN_APP_PATH"*" );
                lidbg_shell_cmd("chmod 777  "ORIGIN_TMP_PATH"*" );
                lidbg_shell_cmd("cp /flysystem/app/.sys-app1/* /system/priv-app/" );

                lidbg_shell_cmd("cp /flysystem/app/sys-app/* /system/priv-app/" );
                lidbg_shell_cmd("cp /flysystem/app/PackageInstaller.apk /system/app/PackageInstaller.apk" );
                lidbg_shell_cmd("chmod 777 /system/app/PackageInstaller.apk" );
                lidbg_shell_cmd("mv /flysystem/app/sys-app /flysystem/app/.sys-app1" );
                lidbg_shell_cmd("chmod 777 /system/priv-app/*");
            }
            if(is_out_updated)
            {
                lidbg_shell_cmd("mount -o remount /system");
                lidbg_shell_cmd("cp /flysystem/lib/out/* /system/lib/modules/out" );
            }

            lidbg_shell_cmd("echo ====system_switch_init:finish==== > /dev/lidbg_msg" );
            //check
            ssleep(10);
            if(fs_is_file_exist("/flysystem/app/sys-app/Launcher3.apk") || fs_is_file_exist("/system/priv-app/Launcher2.apk"))
            {
                lidbg("system_switch_init fail, retry\n");
                goto retry;
            }


        }
    }
    else
    {
#if 0
        if((g_var.is_fly) && (!g_var.recovery_mode) && (g_var.is_first_update))
        {

            lidbg_shell_cmd("echo ====copy apk ==== > /dev/lidbg_msg" );
            lidbg_shell_cmd("mount -o remount /system");
            lidbg_shell_cmd("cp /flysystem/app/sys-app/* /system/priv-app/" );
            lidbg_shell_cmd("rm -rf /flysystem/app/sys-app" );

            lidbg_shell_cmd("cp /flysystem/app/PackageInstaller.apk /system/app/PackageInstaller.apk" );
            lidbg_shell_cmd("rm /flysystem/app/PackageInstaller.apk" );

            lidbg_shell_cmd("chmod 777 /system/priv-app/*" );
            lidbg_shell_cmd("chmod 777 /system/app/PackageInstaller.apk" );
        }
#endif
    }


}
