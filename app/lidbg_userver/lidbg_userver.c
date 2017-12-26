
#include "lidbg_servicer.h"
#include "lidbg_insmod.h"
#include <sys/time.h>
#define SHELL_ERRS_FILE "/dev/dbg_msg"
#define TAG "dlidbg_userver:"
static int debug = 0;
static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do
    {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    }
    while (ret < 0 && errno == EINTR);
    return ret;
}

int main(int argc, char **argv)
{
    int fd, read_len;
    char str[256];
    char shellstring[256];

    argc = argc;
    argv = argv;
    DUMP_BUILD_TIME_FILE;

    debug = (access("/data/lidbg/debug.txt", R_OK) == 0);
    lidbg(TAG"in:debug.%d\n", debug);

    while(access("/dev/lidbg_uevent", R_OK) != 0)
    {
        system("chmod 777 /dev/lidbg_uevent");
        lidbg(TAG"wait  /dev/lidbg_uevent ...\n");
        usleep(100 * 1000);
    }
    if(debug)
        system("echo dbg  > /dev/lidbg_uevent");

    fd = open("/dev/lidbg_uevent", O_RDONLY );
    if((fd == 0) || (fd == (int)0xfffffffe) || (fd == (int)0xffffffff))
    {
        lidbg(TAG"open /dev/lidbg_uevent err\n");
    }
    lidbg(TAG"open /dev/lidbg_uevent success fd :%d\n", fd );

#if 0
    while(1)
    {
        memset(str, '\0', 256);
        read_len = read(fd, str, 256);
        if(read_len >= 0)
        {
            lidbg("do:%s\n", str);
            snprintf(shellstring, 256, "%s 2>> "SHELL_ERRS_FILE, str );
            system(shellstring);
        }
    }
#else
    {
        int  waste_time_ms;
        struct timeval start;
        struct timeval end;
        struct epoll_event  events[1];
        int  nevents;
        int  epoll_fd = epoll_create(1);
        epoll_register( epoll_fd, fd );
        lidbg(TAG"epoll_wait\n");
        while(1)
        {
            nevents = epoll_wait( epoll_fd, events, 1, -1 );
            if (nevents < 0)
            {
                if (errno != EINTR)
                {
                    lidbg(TAG"epoll_wait() unexpected error: %s", strerror(errno));
                }
                lidbg(TAG"nevents < 0,continue");
                continue;
            }
            if ((events[0].events & EPOLLIN) != 0)
            {
                memset(str, '\0', 256);
                read_len = read(fd, str, 256);
                if(read_len >= 0)
                {
                    snprintf(shellstring, 256, "%s 2>> "SHELL_ERRS_FILE, str );
                    if(debug)
                        lidbg(TAG"shell.[%s]\n", shellstring);
                    gettimeofday(&start, NULL);
                    system(shellstring);
                    gettimeofday(&end, NULL);
                    waste_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
                    if(waste_time_ms >= 1000)
                    {
                        //below cmd :have a check
                        //echo flyaudio:sleep 10 > /dev/lidbg_misc0
                        lidbg(TAG"waste time:[%d ms==>%s]\n", waste_time_ms, str);
                    }
                }
                else
                {
                    lidbg(TAG"read_len<0\n");
                }
            }
            else
            {
                lidbg(TAG"else.((events[0].events & EPOLLIN) != 0)\n");
            }

        }
    }
#endif
    return 0;
}

