
#include "lidbg_servicer.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/stat.h>
#include <signal.h>
#include <cutils/klog.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <sys/types.h>
#include <getopt.h>

#undef   LOG_TAG
#define  LOG_TAG "uevent_cold_boot:"
#include <cutils/log.h>

int action = 1;////1:add 2:remove

static void usage(char *cmd)
{
    fprintf(stderr, "Usage: %s just use it as below two ways\n"
            "1: ./lidbg_uevent_cold_boot (default to /sys/block)\n"
            "2: ./lidbg_uevent_cold_boot path int (1:add 2:remove)\n",
            cmd);
    lidbg(LOG_TAG "Usage: %s just use it as below two ways\n"
          "1: ./lidbg_uevent_cold_boot (default to /sys/block)\n"
          "2: ./lidbg_uevent_cold_boot path int (1:add 2:remove)\n",
          cmd);
}
static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY | O_CLOEXEC);
    if(fd >= 0)
    {
        if(action == 1)
            write(fd, "add\n", 4);
        else
            write(fd, "remove\n", 7);
        close(fd);
    }

    while((de = readdir(d)))
    {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else
        {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}
static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if(d)
    {
        do_coldboot(d, 0);
        closedir(d);
    }
}
int main(int argc, char **argv)
{
    argc = argc;
    argv = argv;
    int retries = 5;
    const char *path = "/sys/block";

    SLOGE(LOG_TAG "argc=%d\n", argc);
    lidbg(LOG_TAG "argc=%d\n", argc);

    if (argc == 2 && !strcmp(argv[1], "-h"))
    {
        usage("para error");
        exit(EXIT_SUCCESS);
    }

    if(argc == 3)
    {
        lidbg(LOG_TAG "modify para\n");
        path = argv[1];
        action = atoi(argv[2]);
    }

    SLOGE(LOG_TAG "cold boot: argc=%d,path=%s, action = %d\n", argc, path, action);
    lidbg(LOG_TAG "cold boot argc=%d,path=%s, action = %d\n", argc, path, action);

    coldboot(path);

    SLOGE("exit: %s (%s)\n", path, strerror(errno));
    lidbg(LOG_TAG "exit: %s (%s)\n", path, strerror(errno));
    return -1;
}
