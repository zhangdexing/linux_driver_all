

#define GET_INODE_FROM_FILEP(filp) ((filp)->f_path.dentry->d_inode)

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


int lidbg_readwrite_file(const char *filename, char *rbuf, const char *wbuf, size_t length)
{
    int ret = 0;
    struct file *filp = (struct file *) - ENOENT;
    mm_segment_t oldfs;
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    do
    {
        int mode = (wbuf) ? O_RDWR : O_RDONLY;
        filp = filp_open(filename, mode, S_IRUSR);
        if (IS_ERR(filp) || !filp->f_op)
        {
            ret = -ENOENT;
            break;
        }
        if (!filp->f_op->write || !filp->f_op->read)
        {
            filp_close(filp, NULL);
            ret = -ENOENT;
            break;
        }
        if (length == 0)
        {
            struct inode    *inode;
            inode = GET_INODE_FROM_FILEP(filp);
            if (!inode)
            {
                lidbg("kernel_readwrite_file: Error 2\n");
                ret = -ENOENT;
                break;
            }
            ret = i_size_read(inode->i_mapping->host);
            break;
        }
        if (wbuf)
        {
            ret = filp->f_op->write(filp, wbuf, length, &filp->f_pos);
            if (ret < 0)
            {
                lidbg("kernel_readwrite_file: Error 3\n");
                break;
            }
        }
        else
        {
            ret = filp->f_op->read(filp, rbuf, length, &filp->f_pos);
            if (ret < 0)
            {
                lidbg("kernel_readwrite_file: Error 4\n");
                break;
            }
        }
    }
    while (0);

    if (!IS_ERR(filp))
        filp_close(filp, NULL);
    set_fs(oldfs);
    return ret;
}
