

extern struct lidbg_fifo_device *fs_msg_fifo;

#define FS_SUC(fmt, args...) do {if(fs_slient_level>=3)printk(KERN_CRIT"[lidbg]   [fs] .succ.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]suceed.%s: " fmt,__func__,##args);} while (0)
#define FS_WARN(fmt, args...) do {if(fs_slient_level>=2)printk(KERN_CRIT"[lidbg]   [fs] .warn.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]warn.%s: " fmt,__func__,##args);} while (0)
#define FS_ERR(fmt, args...) do {if(fs_slient_level>=1)printk(KERN_CRIT"[lidbg]   [fs] .erro.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]err.%s: " fmt,__func__,##args);} while (0)

#define FS_ALWAYS(fmt, args...) do {printk(KERN_CRIT"[lidbg]   [fs] .warn.%s: " fmt,__func__,##args);lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]warn.%s: " fmt,__func__,##args);} while (0)


#define LIDBG_KMSG_FILE_PATH LIDBG_LOG_DIR"lidbg_kmsg.txt"
#define MACHINE_ID_FILE LIDBG_LOG_DIR"MIF.txt"
#define LIDBG_NODE "/dev/mlidbg0"
#define KMSG_NODE "/proc/kmsg"

#define PATH_FS_FIFO_FILE LIDBG_LOG_DIR"fs_msg_fifo.txt"

#define PATH_LIDBG_MEM_LOG_FILE LIDBG_MEM_DIR"lidbg_log.txt"
#define PATH_PRE_CONF_INFO_FILE LIDBG_LOG_DIR"conf_info.txt"
#define PATH_DRIVERS_CONF LIDBG_LOG_DIR"drivers.txt"
#define PATH_CORE_CONF LIDBG_LOG_DIR"core.txt"
#define PATH_CMD_CONF LIDBG_LOG_DIR"cmd.txt"
#define PATH_STATE_CONF LIDBG_LOG_DIR"state.txt"
#define PATH_STATE_MEM LIDBG_MEM_DIR"state.txt"

void fs_msg_fifo_to_file(char *key, char *value);
extern int update_list(const char *filename, struct list_head *client_list);
extern int bfs_file_amend(char *file2amend, char *str_append, int file_limit_M);
extern void save_list_to_file(struct list_head *client_list, char *filename);
extern void set_machine_id(void);
extern void copy_all_conf_file(void);
extern void lidbg_fs_cmn_init(void);
extern void lidbg_fs_conf_init(void);
extern void lidbg_fs_keyvalue_init(void);
extern void lidbg_fs_log_init(void);
extern void update_file_tm(void);
extern bool get_file_mftime(const char *filename, struct rtc_time *ptm);
extern bool get_file_tmstring(char *filename, char *tmstring);
extern bool is_file_tm_updated(const char *filename, struct rtc_time *pretm);
extern bool is_fly_system(void);


