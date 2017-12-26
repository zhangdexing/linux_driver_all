#ifndef _LIGDBG_UEVENT__
#define _LIGDBG_UEVENT__

void lidbg_uevent_shell(char *shell_cmd);
bool lidbg_new_cdev(struct file_operations *cdev_fops, char *nodename);
extern bool is_lidbg_uevent_ready;

#endif

