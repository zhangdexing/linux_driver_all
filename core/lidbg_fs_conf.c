
#include "lidbg.h"

//zone below [fs.confops.tools]
LIST_HEAD(lidbg_drivers_list);
LIST_HEAD(lidbg_core_list);
LIST_HEAD(lidbg_machine_info_list);
LIST_HEAD(fs_state_list);
static int g_pollfile_ms = 7000;
static int g_pollstate_ms = 1000;
static int machine_id = 0;
struct rtc_time precorefile_tm;
struct rtc_time predriverfile_tm;
struct rtc_time precmdfile_tm;
struct rtc_time pre_machine_info_tm;
static struct task_struct *fs_statetask;
static struct task_struct *filepoll_task;
//zone end


//zone below [fs.confops.driver]
void clean_all(void)
{
    lidbg_rmdir(LIDBG_LOG_DIR);
}
int launch_file_cmd(const char *filename)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    char *token, *file_ptr = NULL, *file_ptmp;
    int all_purpose;
    unsigned int file_len;

    filep = filp_open(filename, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        lidbg("[futengfei]err.open:<%s>\n", filename);
        return -1;
    }
    lidbg("[futengfei]succeed.open:<%s>\n", filename);

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    lidbg("[futengfei]warn.File_length:<%d>\n", file_len);

    file_ptr = (unsigned char *)vmalloc(file_len + 1);
    if(file_ptr == NULL)
    {
        lidbg( "[futengfei]err.vmalloc:<cannot kzalloc memory!>\n");
        return -1;
    }

    filep->f_op->llseek(filep, 0, 0);
    all_purpose = filep->f_op->read(filep, file_ptr, file_len, &filep->f_pos);
    if(all_purpose <= 0)
    {
        lidbg( "[futengfei]err.f_op->read:<read file data failed>\n");
        return -1;
    }
    set_fs(old_fs);
    filp_close(filep, 0);

    file_ptr[all_purpose] = '\0';
    file_ptmp = file_ptr;
    while((token = strsep(&file_ptmp, "\n")) != NULL )
    {
        if( token[0] != '#' && token[0] != '0' && token[1] == 'c' )
        {
            int loop;
            char p[2] ;
            p[0] = token[0];
            p[1] = '\0';
            loop = simple_strtoul(p, 0, 0);
            for(; loop > 0; loop--)
                fs_file_write(LIDBG_NODE, false, token + 1, 0, strlen(token + 1));
        }
    }
    vfree(file_ptr);
    lidbg_domineering_ack();
    return 1;
}
void show_tm(struct rtc_time *ptmp)
{
    FS_WARN("<file modify time:%d-%02d-%02d %02d:%02d:%02d>\n",
            ptmp->tm_year + 1900, ptmp->tm_mon + 1, ptmp->tm_mday, ptmp->tm_hour + 8, ptmp->tm_min, ptmp->tm_sec);
}
void update_file_tm(void)
{
    get_file_mftime(PATH_CORE_CONF, &precorefile_tm);
    get_file_mftime(PATH_DRIVERS_CONF, &predriverfile_tm);
    get_file_mftime(PATH_CMD_CONF, &precmdfile_tm);
    get_file_mftime(PATH_MACHINE_INFO_FILE, &pre_machine_info_tm);
}
static int thread_pollfile_func(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);

    if(fs_is_file_exist(PATH_CORE_CONF))
    {
        ssleep(20);
        update_file_tm();
    }
    while(!kthread_should_stop())
    {
        if(g_pollfile_ms && is_fs_work_enable)
        {
            msleep(g_pollfile_ms);
            if(is_file_tm_updated(PATH_CORE_CONF, &precorefile_tm))
            {
                show_tm(&precorefile_tm);
                update_list(PATH_CORE_CONF, &lidbg_core_list);
            }

            if(is_file_tm_updated(PATH_DRIVERS_CONF, &predriverfile_tm))
            {
                show_tm(&predriverfile_tm);
                update_list(PATH_DRIVERS_CONF, &lidbg_drivers_list);
            }

            if(is_file_tm_updated(PATH_CMD_CONF, &precmdfile_tm))
            {
                show_tm(&precmdfile_tm);
                launch_file_cmd(PATH_CMD_CONF);
            }

            if(fs_is_file_exist(PATH_MACHINE_INFO_FILE) && is_file_tm_updated(PATH_MACHINE_INFO_FILE, &pre_machine_info_tm))
            {
                show_tm(&pre_machine_info_tm);
                update_list(PATH_MACHINE_INFO_FILE, &lidbg_machine_info_list);
            }
        }
        else
            ssleep(1);
    }
    return 1;
}
void set_machine_id(void)
{
    char string[64];
    if(fs_is_file_exist(MACHINE_ID_FILE))
    {
        fs_file_read(MACHINE_ID_FILE, string, 0, sizeof(string));
        machine_id = simple_strtoul(string, 0, 0);
    }
    else
    {
        get_random_bytes(&machine_id, sizeof(int));
        machine_id = ABS(machine_id);
        sprintf(string, "%d", machine_id);
        FS_WARN("new machine_id:%s\n", string);
        bfs_file_amend(MACHINE_ID_FILE, string, 0);
    }
}
static int thread_pollstate_func(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    while(!kthread_should_stop())
    {
        if(g_pollstate_ms && is_fs_work_enable)
        {
            msleep(g_pollstate_ms);
            save_list_to_file(&fs_state_list, PATH_STATE_MEM);
        }
        else
            ssleep(1);
    }
    return 1;
}
//zone end


//zone below [fs.confops.interface]
void fs_clean_all(void)
{
    clean_all();
}
int get_machine_id(void)
{
    return machine_id;
}
void fs_save_state(void)
{
    if(is_fs_work_enable)
        fs_copy_file(PATH_STATE_MEM, PATH_STATE_CONF);
}
//zone end


void lidbg_fs_conf_init(void)
{
    fs_get_intvalue(&lidbg_core_list, "fs_pollfile_ms", &g_pollfile_ms, NULL);
    fs_get_intvalue(&lidbg_core_list, "fs_updatestate_ms", &g_pollstate_ms, NULL);
    filepoll_task = kthread_run(thread_pollfile_func, NULL, "ftf_filepolltask");
    fs_statetask = kthread_run(thread_pollstate_func, NULL, "ftf_statetask");
}

EXPORT_SYMBOL(lidbg_machine_info_list);
EXPORT_SYMBOL(lidbg_drivers_list);
EXPORT_SYMBOL(lidbg_core_list);
EXPORT_SYMBOL(fs_clean_all);
EXPORT_SYMBOL(get_machine_id);
EXPORT_SYMBOL(fs_save_state);

