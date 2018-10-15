/* Copyright (c) 2012, swlee
 *
 */
#include "lidbg.h"
#include "cmn_func.c"

#if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
struct lidbg_dir_ctx {
        struct dir_context ctx;
        struct list_head names;
};
#endif

 static char str_append[768] = {0};
char *format_string(bool debug, const char *fmt, ... )
{
    va_list args;
    int n;
    memset(str_append, '\0', sizeof(str_append));
    va_start ( args, fmt );
    n = vsprintf ( str_append, (const char *)fmt, args );
    va_end ( args );
    if(debug)
        lidbg("format_string:%s\n", str_append);
    return str_append;
}

void lidbg_shell_cmd(char *shell_cmd)
{
    lidbg_uevent_shell(shell_cmd);
}

char *get_lidbg_file_path(char *buff, char *filename)
{
    char *path;
//    path = (gboot_mode == MD_FLYSYSTEM) ? "/flysystem/lib/out/" : "/system/lib/modules/out/";
	path = "/system/lib/modules/out/";
	printk("<<<<<<<<<path = %s>>>>>>>\n", path);
    if(gboot_mode == MD_DEBUG)
		path = "/data/out/";
    sprintf(buff, "%s%s", path, filename);
    return buff;
}
static char  udisk_path[64];
char *set_udisk_path(char *buff)
{
    if(buff != NULL)
        snprintf(udisk_path, sizeof(udisk_path), "%s", buff);
    return buff;
}
char *get_udisk_file_path(char *buff, char *filename)
{
    if(filename != NULL)
        sprintf(buff, "%s/%s", udisk_path, filename);
    else
        return udisk_path;
    return buff;
}

void set_power_state(int state)
{
    lidbg("set_power_state:%d\n", state);

    if(state == 0)
        lidbg_readwrite_file("/sys/power/state", NULL, "mem", sizeof("mem") - 1);
    else
        lidbg_readwrite_file("/sys/power/state", NULL, "on", sizeof("on") - 1);
}


void set_cpu_governor(int state)
{
    char buf[16];
    int len = -1;
    lidbg("set_cpu_governor:%d\n", state);
    if(state == 0)
    {
        len = sprintf(buf, "%s", "ondemand");
    }
    else if(state == 1)
    {
        len = sprintf(buf, "%s", "performance");

    }
    else if(state == 2)
    {
        len = sprintf(buf, "%s", "powersave");

    }
    lidbg_shell_cmd("chmod 777 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", NULL, buf, len);
}


ssize_t read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{

    int len = 0;
    struct task_struct *task_list;
    PROC_READ_CHECK;

    for_each_process(task_list)
    {
        len  += sprintf(buf + len, "%s %d \n", task_list->comm, task_list->pid);
    }
    *ppos += len;
    return len;
}

static const struct file_operations ps_list_fops =
{
    .read  = read_proc,
};

void create_new_proc_entry(void)
{
    proc_create("ps_list", 0, NULL, &ps_list_fops);
}

int lidbg_task_kill_select(char *task_name)
{
    struct task_struct *p;
    struct task_struct *selected = NULL;
    DUMP_FUN_ENTER;

    //read_lock(&tasklist_lock);
    for_each_process(p)
    {
        struct mm_struct *mm;
        struct signal_struct *sig;

        task_lock(p);
        mm = p->mm;
        sig = p->signal;
        task_unlock(p);

        selected = p;
        //lidbg( "process %d (%s)\n",p->pid, p->comm);

        if(!strcmp(p->comm, task_name))
        {
            lidbg("find %s to kill\n", task_name);

            if (selected)
            {
                force_sig(SIGKILL, selected);
                return 1;
            }
        }
        //read_unlock(&tasklist_lock);
    }
    DUMP_FUN_LEAVE;
    return 0;
}

u32 lidbg_get_ns_count(void)
{
    struct timespec t_now;
    getnstimeofday(&t_now);
    return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;

}

//return how many ms since boot
u32 get_tick_count(void)
{
    struct timespec t_now;
    //do_posix_clock_monotonic_gettime(&t_now);
    //monotonic_to_bootbased(&t_now);
	get_monotonic_boottime(&t_now);
    return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;
}

char *lidbg_get_current_time(char *time_string, struct rtc_time *ptm)
{
    struct timespec ts;
    struct rtc_time tm;
    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &tm);
    if(time_string)
        sprintf(time_string, "%d-%02d-%02d__%02d.%02d.%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, (((tm.tm_hour + 8)<24)?(tm.tm_hour + 8):(tm.tm_hour + 8 - 24)) , tm.tm_min, tm.tm_sec);
    if(ptm)
        *ptm = tm;
    return time_string;
}
u32 lidbg_get_random_number(u32 num_max)
{
    u32 ret;
    get_random_bytes(&ret, sizeof(ret));
    return ret % num_max;
}
void lidbg_domineering_ack(void)
{
    DUMP_FUN;
    lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK));
}
int  lidbg_toast_show(char *who,char *what)
{
    char para[128] = {0};
    sprintf(para, "am broadcast -a com.lidbg.flybootserver.action --es toast  \"%s%s\" &", who ? who : "",what ? what : "null");
    lidbg_shell_cmd(para);
    return 1;
}

struct name_list
{
    char name[33];
    struct list_head list;
};
static int readdir_build_namelist(void *arg, const char *name, int namlen,	loff_t offset, u64 ino, unsigned int d_type)
{
    if(!(name[0] == '.' || (name[0] == '.' && name[1] == '.'))) // ignore "." and ".."
    {
        #if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
	struct lidbg_dir_ctx *ctx = arg;
	#else
        struct list_head *names = arg;
	#endif
        struct name_list *entry;
        entry = kzalloc(sizeof(struct name_list), GFP_KERNEL);
        if (entry == NULL)
            return -ENOMEM;
        memcpy(entry->name, name, namlen);
        entry->name[namlen] = '\0';
	
        #if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
        list_add(&entry->list, &ctx->names);
	#else
        list_add(&entry->list, names);
	#endif
    }
    return 0;
}

int lidbg_readdir_and_dealfile(char *insure_is_dir, void (*callback)(char *dirname, char *filename))
{
    LIST_HEAD(names);
    struct file *dir_file;
    struct dentry *dir;
    int status;
    if(!insure_is_dir || !callback)
        return -1;

    dir_file = filp_open(insure_is_dir, O_RDONLY | O_DIRECTORY, 0);
    if (IS_ERR(dir_file))
    {
        LIDBG_ERR("open%s,%ld\n", insure_is_dir, PTR_ERR(dir_file));
        return -1;
    }
    else
    {
        struct name_list *entry;
        int count = 0;
       #if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
	struct lidbg_dir_ctx ctx = {
		.ctx.actor = readdir_build_namelist,
		.names = LIST_HEAD_INIT(ctx.names)
	};
	#endif
        LIDBG_SUC("open:<%s,%s>\n", insure_is_dir, dir_file->f_path.dentry->d_name.name);
        dir = dir_file->f_path.dentry;

        #if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
	status = iterate_dir(dir_file,&ctx.ctx);
        #else
        status = vfs_readdir(dir_file, readdir_build_namelist, &names);
	#endif

        if (dir_file)
            fput(dir_file);
	
        #if(LINUX_VERSION_CODE > KERNEL_VERSION(3, 11, 0))
        while (!list_empty(&ctx.names))
        {
            entry = list_entry(ctx.names.next, struct name_list, list);
	#else
        while (!list_empty(&names))
        {
            entry = list_entry(names.next, struct name_list, list);
	#endif	
            if (!status && entry)
            {
                count++;
                if(callback)
                    callback(insure_is_dir, entry->name);
                list_del(&entry->list);
                kfree(entry);
            }
            entry = NULL;
        }
        return count;
    }
}

//returns value in Ch9.h  likes USB_CLASS_MASS_STORAGE/USB_CLASS_VIDEO 
#define get_iface_desc(iface)	(&(iface)->desc)
int lidbg_get_usb_device_type(struct usb_device * dev)
{
    struct usb_host_interface *alts;
    struct usb_interface_descriptor *altsd ;
    struct usb_interface *iface;
    struct usb_host_config *config ;
    int i;

    if (dev == NULL || dev->actconfig == NULL)
    {
        LIDBG_WARN("dev == NULL||dev->actconfig==NULL\n");
        return -1;
    }
    config = dev->actconfig;
    if(config != NULL && (config->desc).bNumInterfaces != -1)
    {
        LIDBG_WARN("bNumInterfaces:%d\n", config->desc.bNumInterfaces);
    }
    else
    {
        LIDBG_WARN("bNumInterfaces==NULL\n");
        return -1;
    }
    if (config->desc.bNumInterfaces >= USB_MAXINTERFACES)
    {
        LIDBG_WARN("bNumInterfaces>=USB_MAXINTERFACES\n");
        return -1;
    }

    for (i = 0; i < config->desc.bNumInterfaces; i++)
    {
        iface = config->interface[i];
        if(iface == NULL || (& iface->altsetting[0]) == NULL)
        {
            LIDBG_WARN("iface == NULL\n");
            return -1;
        }
        alts = & iface->altsetting[0];
        altsd = get_iface_desc(alts);
        if(altsd != NULL)
        {
            LIDBG_WARN("[%d/bInterfaceClass:%d\n", i, altsd->bInterfaceClass);
            return altsd->bInterfaceClass;
        }
        else
            LIDBG_WARN("bInterfaceClass:NULL\n");
    }
    return -1;
}

/*
input:  c io w 27 1
output:  token[0]=c  token[4]=1  return:5
*/
int lidbg_token_string(char *buf, char *separator, char **token)
{
    char *token_tmp;
    int pos = 0;
    if(!buf || !separator)
    {
        LIDBG_ERR("buf||separator NULL?\n");
        return pos;
    }
    while((token_tmp = strsep(&buf, separator)) != NULL )
    {
        *token = token_tmp;
        token++;
        pos++;
    }
    return pos;
}
/*
input:  "/storage/udisk/hello.txt"
output:  "/storage/udisk-DEF4-DEEF/hello.txt"
*/
bool lidbg_strstrrpl(char *des, char originalString[], char key[], char swap[])
{
    char *toke2 = NULL;
    char *toke1 = strstr(originalString, key);
    if (toke1 == NULL)
    {
        sprintf(des, "%s", originalString);
        return false;
    }
    toke2 = toke1 + strlen(key) - 1;
    toke1[0] = '\0';
    sprintf(des, "%s%s%s", originalString, swap, toke2);
    return true;
}

/*
input:  "123/456/789"
output:  "123456789"
*/
void lidbg_strrpl(char originalString[], char key[], char swap[])
{
    int lengthOfOriginalString, lengthOfKey, lengthOfSwap, i, j , flag;
    char tmp[1000];

    lengthOfOriginalString = strlen(originalString);
    lengthOfKey = strlen(key);
    lengthOfSwap = strlen(swap);

    for( i = 0; i <= lengthOfOriginalString - lengthOfKey; i++)
    {
        flag = 1;
        for(j  = 0; j < lengthOfKey; j ++)
        {
            if(originalString[i + j] != key[j])
            {
                flag = 0;
                break;
            }
        }
        if(flag)
        {
            strcpy(tmp, originalString);
            strcpy(&tmp[i], swap);
            strcpy(&tmp[i + lengthOfSwap], &originalString[i  + lengthOfKey]);
            strcpy(originalString, tmp);
            i += lengthOfSwap - 1;
            lengthOfOriginalString = strlen(originalString);
        }
    }
}

/*
input:  "123 456 789"
output:  "123456789"
*/
char *lidbgstrtrim(char *s)
{
    char *p, *q, *o;
    while(*s != '\0' && *s == ' ')s++;
    p = q = o = s;
    while(*p != '\0')
    {
        if(*p != ' ')
        {
            *q = *p;
            q++;
        }
        p++;
    }
    *q = '\0';
    return o;
}
#define ITEM_MAX (35)
#define MOUNT_BUF_MAX (3 * 1024)
static int invaled_mount_point = 0;
static char buf_mount[MOUNT_BUF_MAX];
static struct mounted_volume g_mounts_state[ITEM_MAX];
static int read_file(const char *filename, char *rbuff, int readlen)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int read_len = 1;

    filep = filp_open(filename,  O_RDONLY, 0);
    if(IS_ERR(filep))
    {
        LIDBG_ERR("filp_open.%s\n",filename);
        return -1;
    }
    old_fs = get_fs();
    set_fs(get_ds());

    filep->f_op->llseek(filep, 0, 0);
    read_len = filep->f_op->read(filep, rbuff, readlen, &filep->f_pos);

    set_fs(old_fs);
    filp_close(filep, 0);
    return read_len;
}
bool scan_mounted_volumes(char *info_path)
{
    char *mount_item[ITEM_MAX] = {NULL};
    char 	*item_token[12] = {NULL};
    char	 *p = buf_mount;
    int	 ret = false;
    int mount_item_max;
    int item_token_len, loop;

    memset(p, '\0', MOUNT_BUF_MAX);
    if(read_file(info_path, p, MOUNT_BUF_MAX - 1) >= 0)
    {
        mount_item_max = lidbg_token_string(buf_mount, "\n", mount_item);
        invaled_mount_point = mount_item_max;
        for(loop = 0; loop < mount_item_max; loop++)
        {
            item_token_len = lidbg_token_string(mount_item[loop], " ", item_token);
            if(item_token_len >= 4)
            {
                g_mounts_state[loop].device = item_token[0];
                g_mounts_state[loop].mount_point = item_token[1];
                g_mounts_state[loop].filesystem = item_token[2];
                g_mounts_state[loop].others = item_token[3];
                ret = true;
            }
        }
    }
    else
        lidbg("err.scan_mounted_volumes:%d\n", invaled_mount_point);
    return ret;
}
struct mounted_volume *find_mounted_volume_by_mount_point(char *mount_point)
{
    int i;
    scan_mounted_volumes("/proc/mounts");
    msleep(300);
    if (invaled_mount_point <= 0)
    {
        lidbg("err.invaled_mount_point <= 0,%d\n", invaled_mount_point);
        return NULL;
    }

    for (i = 0; i < invaled_mount_point; i++)
    {
        if (g_mounts_state[i].mount_point != NULL && !strcmp(g_mounts_state[i].mount_point, mount_point))
        {
            LIDBG_SUC("%s\n", mount_point);
            return &g_mounts_state[i];
        }
    }
    LIDBG_ERR("%s\n", mount_point);
    return NULL;
}

static int __init cmn_init(void)
{
    create_new_proc_entry();
    LIDBG_MODULE_LOG;
    return 0;
}

static void __exit cmn_exit(void)
{
    remove_proc_entry("proc_entry", NULL);
}

module_init(cmn_init);
module_exit(cmn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");

EXPORT_SYMBOL(find_mounted_volume_by_mount_point);
EXPORT_SYMBOL(lidbg_readdir_and_dealfile);
EXPORT_SYMBOL(lidbg_token_string);
EXPORT_SYMBOL(lidbg_strrpl);
EXPORT_SYMBOL(lidbg_strstrrpl);
EXPORT_SYMBOL(lidbgstrtrim);
EXPORT_SYMBOL(lidbg_get_random_number);
EXPORT_SYMBOL(lidbg_toast_show);
EXPORT_SYMBOL(lidbg_get_usb_device_type);
EXPORT_SYMBOL(lidbg_domineering_ack);
EXPORT_SYMBOL(lidbg_get_ns_count);
EXPORT_SYMBOL(get_tick_count);
EXPORT_SYMBOL(lidbg_readwrite_file);
EXPORT_SYMBOL(lidbg_task_kill_select);
EXPORT_SYMBOL(lidbg_get_current_time);
EXPORT_SYMBOL(set_power_state);
EXPORT_SYMBOL(get_lidbg_file_path);
EXPORT_SYMBOL(format_string);
EXPORT_SYMBOL(set_udisk_path);
EXPORT_SYMBOL(get_udisk_file_path);
EXPORT_SYMBOL(lidbg_shell_cmd);
EXPORT_SYMBOL(set_cpu_governor);

