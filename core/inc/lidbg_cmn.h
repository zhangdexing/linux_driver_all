#ifndef _LIGDBG_CMN__
#define _LIGDBG_CMN__

#include "cmn_func.h"
#define NOTIFIER_MAJOR_SIGNAL_EVENT (0)
#define NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK (0)

struct mounted_volume
{
    char *device;
    char *mount_point;
    char *filesystem;
    char *others;
};
struct mounted_volume *find_mounted_volume_by_mount_point(char *mount_point);
int lidbg_readdir_and_dealfile(char *insure_is_dir, void (*callback)(char *dirname, char *filename));
u32 lidbg_get_random_number(u32 num_max);
int  lidbg_token_string(char *buf, char *separator, char **token);
void lidbg_strrpl(char originalString[], char key[], char swap[]);
bool lidbg_strstrrpl(char *des, char originalString[], char key[], char swap[]);
char *lidbgstrtrim(char *s);
int  lidbg_toast_show(char *who,char *what);
int lidbg_get_usb_device_type(struct usb_device * dev);
void lidbg_domineering_ack(void);
u32 lidbg_get_ns_count(void);
u32 get_tick_count(void);
int lidbg_task_kill_select(char *task_name);
char *lidbg_get_current_time(char *time_string, struct rtc_time *ptm);
void set_power_state(int state);
void lidbg_loop_warning(void);
void lidbg_system_switch(bool origin_system);
void lidbg_shell_cmd(char *shell_cmd);
char *get_bin_path( char *buf);
char *get_lidbg_file_path(char *buff, char *filename);
char *format_string(bool debug, const char *fmt, ... );
void set_cpu_governor(int state);
char *set_udisk_path(char *buff);
char *get_udisk_file_path(char *buff, char *filename);

#define MOUNT_PATH get_bin_path("mount")
#define INSMOD_PATH get_bin_path("insmod")
#define CHMOD_PATH  get_bin_path("chmod")
#define MV_PATH  get_bin_path("mv")
#define RM_PATH  get_bin_path("rm")
#define RMDIR_PATH  get_bin_path("rm")
#define MKDIR_PATH  get_bin_path("mkdir")
#define TOUCH_PATH  get_bin_path("touch")
#define REBOOT_PATH  get_bin_path("reboot")
#define SETPROP_PATH get_bin_path("setprop")



#define CREATE_KTHREAD(func,data)\
do{\
struct task_struct *task;\
lidbg("create kthread %s\n",""#func);\
task = kthread_create(func, data, ""#func);\
if(IS_ERR(task))\
{\
	lidbg("Unable to start thread.\n");\
}\
else wake_up_process(task);\
}while(0)\
 
#endif

