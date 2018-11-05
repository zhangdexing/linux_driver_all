
#include "lidbg.h"


//zone below [fs.log.tools]
int max_file_len = 1;
int g_iskmsg_ready = 1;
//zone end

//zone below [fs.log.driver]

int bfs_file_amend(char *file2amend, char *str_append, int file_limit_M)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    int  flags, is_file_cleard = 0, file_limit = max_file_len;
    unsigned int file_len;

    if(str_append == NULL)
    {
        lidbg("[futengfei]err.fileappend_mode:<str_append=null>\n");
        return -1;
    }
    flags = O_CREAT | O_RDWR | O_APPEND;

    if(file_limit_M > max_file_len)
        file_limit = file_limit_M;

again:
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    filep = filp_open(file2amend, flags , 0777);
    if(IS_ERR(filep))
    {
        lidbg("[futengfei]err.open:<%s>\n", file2amend);
        return -1;
    }
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    old_fs = get_fs();
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    set_fs(get_ds());
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    inode = filep->f_dentry->d_inode;
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    file_len = inode->i_size;
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    file_len = file_len + 1;
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);

    if(file_len > file_limit * MEM_SIZE_1_MB)
    {
        lidbg("[futengfei]warn.fileappend_mode:< file>8M.goto.again >\n");
        is_file_cleard = 1;
        flags = O_CREAT | O_RDWR | O_APPEND | O_TRUNC;
        set_fs(old_fs);
        filp_close(filep, 0);
        goto again;
    }
	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
    vfs_llseek(filep, 0, SEEK_END);//note:to the end to append;
    if(1 == is_file_cleard)
    {
        char *str_warn = "============have_cleard=============\n\n";
        is_file_cleard = 0;
		printk("2>>>>>>>%d<<<<<<<\n", __LINE__);
		vfs_write(filep, str_warn, strlen(str_warn), &filep->f_pos);
    }
	printk("1>>>>>>>%d %s<<<<<<<\n", __LINE__, str_append);
	vfs_write(filep, str_append, strlen(str_append), &filep->f_pos);


	printk("1>>>>>>>%d<<<<<<<\n", __LINE__);
	set_fs(old_fs);
    filp_close(filep, 0);
    return 1;
}


void file_separator(char *file2separator)
{
    char buf[32];
    lidbg_get_current_time(buf, NULL);
    fs_string2file(0, file2separator, "------%s------\n", buf);
}
//zone end


//zone below [fs.log.interface]
void fs_file_separator(char *file2separator)
{
    file_separator(file2separator);
}
int fs_string2file(int file_limit_M, char *filename, const char *fmt, ... )
{
    va_list args;
    int n;
    char str_append[768];
    va_start ( args, fmt );
    n = vsprintf ( str_append, (const char *)fmt, args );
    va_end ( args );

    return bfs_file_amend(filename, str_append, file_limit_M);
}
int fs_mem_log( const char *fmt, ... )
{
    int len;
    va_list args;
    int n;
    char str_append[512];
    va_start ( args, fmt );
    n = vsprintf ( str_append, (const char *)fmt, args );
    va_end ( args );

    str_append[512 - 1] = '\0';
    len = strlen(str_append);

    bfs_file_amend(PATH_LIDBG_MEM_LOG_FILE, str_append, 0);

    return 1;
}
//zone end


void lidbg_fs_log_init(void)
{
    FS_REGISTER_INT(max_file_len, "fs_max_file_len", 1, NULL);
}

EXPORT_SYMBOL(fs_file_separator);
EXPORT_SYMBOL(fs_string2file);
EXPORT_SYMBOL(fs_mem_log);
EXPORT_SYMBOL(bfs_file_amend);


