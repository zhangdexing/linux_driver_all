
#include "lidbg.h"

//zone below [tools]
#define FS_BUFF_SIZE (1 * 1024 )
#define FS_FIFO_SIZE (1 * 1024)

#define FS_VERSION "FS.VERSION:  [20131031]"
bool is_out_updated = false;
bool is_fs_work_enable = false;
int fs_slient_level = 0;//level:0->mem_fifo    (1 err 2 warn 3suc)->uart  4 debug
struct lidbg_fifo_device *fs_msg_fifo;
//zone end

void lidbg_fileserver_main(int argc, char **argv)
{
    int cmd = 0;
    int cmd_para = 0;
    int thread_count = 0;
    if(argc < 3)
    {
        FS_ERR("echo \"c file 1 1 1\" > /dev/mlidbg0\n");
        return;
    }

    thread_count = simple_strtoul(argv[0], 0, 0);
    cmd = simple_strtoul(argv[1], 0, 0);
    cmd_para = simple_strtoul(argv[2], 0, 0);

    if(thread_count)
        FS_ERR("thead_test:remove\n");

    switch (cmd)
    {
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        FS_WARN("machine_id:%d\n", get_machine_id());
        break;
    case 5:
        fs_save_list_to_file();
        break;
    case 6:
        break;
    case 10:
        FS_WARN("<fs_show_filename_list>\n");
        fs_show_filename_list();
        break;
    case 11:
        FS_ALWAYS("<fs_msg_fifo_to_file>\n");
        fs_msg_fifo_to_file(NULL, NULL);
        break;
    default:
        FS_ERR("<check you cmd:%d>\n", cmd);
        break;
    }
}


//zone below [fileserver]
void copy_all_conf_file(void)
{
    char buff[50] = {0};
    fs_copy_file(get_lidbg_file_path(buff, "build_time.conf"), LIDBG_LOG_DIR"build_time.txt");
    fs_copy_file(get_lidbg_file_path(buff, "core.conf"), PATH_CORE_CONF);
    fs_copy_file(get_lidbg_file_path(buff, "drivers.conf"), PATH_DRIVERS_CONF) ;
    fs_copy_file(get_lidbg_file_path(buff, "state.conf"), PATH_STATE_CONF);
    fs_copy_file(get_lidbg_file_path(buff, "cmd.conf"), PATH_CMD_CONF);
    fs_copy_file(get_lidbg_file_path(buff, "machine_info.conf"), PATH_MACHINE_INFO_FILE);
}
void check_conf_file(void)
{
    int size[10];
    char buff[50] = {0};

    size[0] = fs_is_file_updated(get_lidbg_file_path(buff, "build_time.conf"), PATH_PRE_CONF_INFO_FILE);
    size[1] = fs_get_file_size(PATH_DRIVERS_CONF);
    size[2] = fs_get_file_size(PATH_CORE_CONF);
    size[3] = fs_get_file_size(PATH_STATE_CONF);
    size[4] = fs_get_file_size(PATH_PRE_CONF_INFO_FILE);
    size[5] = fs_get_file_size(PATH_MACHINE_INFO_FILE);

    fs_mem_log("<check_conf_file:%d,%d,%d,%d,%d,%d>\n", size[0], size[1], size[2], size[3], size[4], size[5]);
    FS_ALWAYS("<check_conf_file:%d,%d,%d,%d,%d,%d>\n", size[0], size[1], size[2], size[3], size[4], size[5]);

    if(size[0] || size[1] < 1 || size[2] < 1 || size[4] < 1 || size[5] < 1)
    {
        FS_ALWAYS( "<overwrite:push,update?>\n");
        fs_mem_log( "<overwrite:push,update?>\n");
        is_out_updated = true;
        copy_all_conf_file();
        lidbg_shell_cmd("rm -rf /data/kmsg.txt");
        lidbg_shell_cmd("rm -rf "LIDBG_KMSG_FILE_PATH);
        lidbg_shell_cmd("rm -rf "LIDBG_LOG_DIR"lidbg_mem_log.txt");
        lidbg_shell_cmd("rm -rf "PATH_FS_FIFO_FILE);
        lidbg_shell_cmd("rm -rf "LIDBG_TRACE_MSG_PATH);
    }

}
void fs_msg_fifo_to_file(char *key, char *value)
{
    fs_mem_log("%s\n", __func__);
    lidbg_fifo_get(fs_msg_fifo, PATH_FS_FIFO_FILE, 0);
}
void lidbg_fileserver_main_prepare(void)
{
    char buff[50] = {0};

    fs_msg_fifo = lidbg_fifo_alloc("fileserver", FS_FIFO_SIZE, FS_BUFF_SIZE);

    FS_WARN("<%s>\n", FS_VERSION);

    lidbg_shell_cmd("mkdir "LIDBG_LOG_DIR);
    lidbg_shell_cmd("mkdir "LIDBG_OSD_DIR);

    check_conf_file();

    set_machine_id();
    FS_ALWAYS("machine_id:%d\n", get_machine_id());

    fs_fill_list(PATH_CORE_CONF, FS_CMD_FILE_CONFIGMODE, &lidbg_core_list);
    fs_fill_list(PATH_DRIVERS_CONF, FS_CMD_FILE_CONFIGMODE, &lidbg_drivers_list);
    fs_fill_list(PATH_MACHINE_INFO_FILE, FS_CMD_FILE_CONFIGMODE, &lidbg_machine_info_list);

    fs_copy_file(get_lidbg_file_path(buff, "build_time.conf"), LIDBG_MEM_DIR"build_time.txt");
    if(fs_slient_level != 4)
        fs_get_intvalue(&lidbg_core_list, "fs_slient_level", &fs_slient_level, NULL);
    fs_get_intvalue(&lidbg_core_list, "fs_save_msg_fifo", NULL, fs_msg_fifo_to_file);

}
void lidbg_fileserver_main_init(void)
{
}
//zone end

static int __init lidbg_fileserver_init(void)
{
    FS_ALWAYS("<==IN==>\n\n");

    lidbg_fileserver_main_prepare();

    lidbg_fs_keyvalue_init();
    lidbg_fs_log_init();
    lidbg_fs_conf_init();
    lidbg_fs_cmn_init();

    lidbg_fileserver_main_init();//note,put it in the end.
    msleep(100);
    LIDBG_MODULE_LOG;

    FS_ALWAYS("<==OUT==>\n");
    return 0;
}

static void __exit lidbg_fileserver_exit(void)
{
}


EXPORT_SYMBOL(lidbg_fileserver_main);
EXPORT_SYMBOL(is_out_updated);
EXPORT_SYMBOL(is_fs_work_enable);
EXPORT_SYMBOL(fs_slient_level);

module_init(lidbg_fileserver_init);
module_exit(lidbg_fileserver_exit);

MODULE_AUTHOR("futengfei");
MODULE_DESCRIPTION("fileserver.entry");
MODULE_LICENSE("GPL");

