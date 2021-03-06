
#include "lidbg.h"
//zone below [fs.cmn.tools]
//zone end

//zone below [fs.cmn.driver]
int fs_file_write2(char *filename, char *wbuff)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int file_len = 1;

    filep = filp_open(filename,  O_CREAT | O_RDWR, 0666);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"err:filp_open,%s\n\n\n\n", filename);
        return -1;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    vfs_llseek(filep, 0, SEEK_END);
    if(wbuff)
        vfs_write(filep, wbuff, strlen(wbuff), &filep->f_pos);
    set_fs(old_fs);
    filp_close(filep, 0);
    return file_len;
}

//do not use this interface to write a real file,use fs_file_write2() instead.
int fs_file_write(char *filename, bool creat, void *wbuff, loff_t offset, int len)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int file_len = 1;

    filep = filp_open(filename, creat ? O_CREAT | O_WRONLY : O_WRONLY, 0);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"err:filp_open,%s\n\n\n\n", filename);
        return -1;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    vfs_llseek(filep, offset, SEEK_SET);

    if(wbuff)
        vfs_write(filep, wbuff, len, &filep->f_pos);
	    
    set_fs(old_fs);
    filp_close(filep, 0);
    return file_len;
}
int fs_file_read(const char *filename, char *rbuff, loff_t offset, int readlen)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int read_len = 1;

    filep = filp_open(filename,  O_RDONLY, 0);
    if(IS_ERR(filep))
        return -1;
    old_fs = get_fs();
    set_fs(get_ds());

    vfs_llseek(filep, offset, SEEK_SET);
    
    read_len = vfs_read(filep, rbuff, readlen, &filep->f_pos);

    set_fs(old_fs);
    filp_close(filep, 0);
    return read_len;
}

bool clear_file(char *filename)
{
    struct file *filep;
    filep = filp_open(filename, O_CREAT | O_RDWR | O_TRUNC , 0777);
    if(IS_ERR(filep))
    {
		printk("clear_file false\n");
        return false;
    	}
	else
        filp_close(filep, 0);
	printk("clear_file true\n");
    return true;
}
bool get_file_mftime(const char *filename, struct rtc_time *ptm)
{
    struct file *filep;
    struct inode *inode = NULL;
    struct timespec mtime;
    filep = filp_open(filename, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        lidbg("[futengfei]err.open:<%s>\n", filename);
        return false;
    }
    inode = filep->f_dentry->d_inode;
    mtime = inode->i_mtime;
    rtc_time_to_tm(mtime.tv_sec, ptm);
    filp_close(filep, 0);
    return true;
}
bool is_tm_updated(struct rtc_time *pretm, struct rtc_time *newtm)
{
    if(pretm->tm_sec == newtm->tm_sec && pretm->tm_min == newtm->tm_min && pretm->tm_hour == newtm->tm_hour &&
            pretm->tm_mday == newtm->tm_mday && pretm->tm_mon == newtm->tm_mon && pretm->tm_year == newtm->tm_year )
        return false;
    else
        return true;
}
bool is_file_tm_updated(const char *filename, struct rtc_time *pretm)
{
    struct rtc_time tm;
    if(get_file_mftime(filename, &tm) && is_tm_updated(pretm, &tm))
    {
        *pretm = tm; //get_file_mftime(filename, pretm);//give the new tm to pretm;
        return true;
    }
    else
        return false;
}

bool is_file_exist(char *file)
{
    struct file *filep;
    filep = filp_open(file, O_RDONLY , 0);
    if(IS_ERR(filep))
        return false;
    else
    {
        filp_close(filep, 0);
        return true;
    }
}
int fs_get_file_size(char *file)
{
    struct file *filep;
    struct inode *inodefrom = NULL;
    unsigned int file_len;
    filep = filp_open(file, O_RDONLY , 0);
    if(IS_ERR(filep))
        return -1;
    else
    {
        inodefrom = filep->f_dentry->d_inode;
        file_len = inodefrom->i_size;
        filp_close(filep, 0);
        return file_len;
    }
}
void mem_encode(char *addr, int length, int step_size)
{
    int loop = 0;
    char temp;
    while(loop < length)
    {
        temp = *(addr + loop) + step_size;
        if(temp > 0 && temp < 127)
            *(addr + loop) = temp;
        loop++;
    }
}

bool copy_file(char *from, char *to, bool encode)
{
    char *string = NULL;
    unsigned int file_len;
    struct file *pfilefrom;
    struct file *pfileto;
    struct inode *inodefrom = NULL;
    mm_segment_t old_fs;
	printk("copy file from = %s, to = %s\n", from, to);

    if(!fs_is_file_exist(from))
    {
        FS_ALWAYS("<file_miss:%s>\n", from);
        return false;
    }

    pfilefrom = filp_open(from, O_RDONLY , 0);
    pfileto = filp_open(to, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if(IS_ERR(pfileto))
    {
		{
        		char cmd[128] = {0};
      		sprintf(cmd, "rm -rf %s", to);
        		lidbg_shell_cmd(cmd);
		}
        msleep(500);
        pfileto = filp_open(to, O_CREAT | O_RDWR | O_TRUNC, 0777);
        if(IS_ERR(pfileto))
        {
            FS_ALWAYS("fail to open <%s>\n", to);
            filp_close(pfilefrom, 0);
            return false;
        }
    }
    old_fs = get_fs();
    set_fs(get_ds());
    inodefrom = pfilefrom->f_dentry->d_inode;
    file_len = inodefrom->i_size;

    string = (unsigned char *)vmalloc(file_len);
    if(string == NULL)
    {
        FS_ALWAYS(" <vmalloc.%d>\n",file_len);
        return false;
    }

    vfs_llseek(pfilefrom, 0, 0);
	FS_ALWAYS("pfilefrom->f_pos = %s, &pfilefrom->f_pos = %x\n", pfilefrom->f_pos, &pfilefrom->f_pos);
	vfs_read(pfilefrom, string, file_len, &pfilefrom->f_pos);
    set_fs(old_fs);
    filp_close(pfilefrom, 0);
    if(encode)
        mem_encode(string, file_len, 1);

    old_fs = get_fs();
    set_fs(get_ds());
    vfs_llseek(pfileto, 0, 0);
    vfs_write(pfileto, string, file_len, &pfileto->f_pos);
    	
    set_fs(old_fs);
    filp_close(pfileto, 0);

    vfree(string);
    return true;
}
bool get_file_tmstring(char *filename, char *tmstring)
{
    struct rtc_time tm;
    if(filename && tmstring && get_file_mftime(filename, &tm) )
    {
        sprintf(tmstring, "%d-%02d-%02d %02d:%02d:%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour + 8, tm.tm_min, tm.tm_sec);
        return true;
    }
    return false;
}
bool fs_is_file_updated(char *filename, char *infofile)
{
    char pres[64], news[64];
    memset(pres, '\0', sizeof(pres));
    memset(news, '\0', sizeof(news));
	
    get_file_tmstring(filename, news);
	printk("filename = %s, infofile = %s news = %s\n", filename, infofile, news);

    if(fs_get_file_size(infofile) > 0)
    {
		printk("fs_get_file_size(infofile) > 0\n");
        if ( (fs_file_read(infofile, pres, 0 , sizeof(pres)) > 0) && (!strcmp(news, pres)))
        {
        	printk("fs_file_read false \n");
            return false;
        }
    }
    fs_clear_file(infofile);
    bfs_file_amend(infofile, news, 0);
    return true;
}
//zone end

//zone below [fs.cmn.interface]
bool fs_copy_file(char *from, char *to)
{
    return copy_file(from, to, false);
}
bool fs_copy_file_encode(char *from, char *to)
{
    return copy_file(from, to, true);
}
bool fs_is_file_exist(char *file)
{
    return is_file_exist(file);
}
bool fs_clear_file(char *filename)
{
    return clear_file(filename);
}
//zone end



void lidbg_fs_cmn_init(void)
{
}


EXPORT_SYMBOL(fs_file_read);
EXPORT_SYMBOL(fs_file_write);
EXPORT_SYMBOL(fs_file_write2);
EXPORT_SYMBOL(fs_copy_file);
EXPORT_SYMBOL(fs_copy_file_encode);
EXPORT_SYMBOL(fs_is_file_exist);
EXPORT_SYMBOL(fs_is_file_updated);
EXPORT_SYMBOL(fs_clear_file);
EXPORT_SYMBOL(fs_get_file_size);

