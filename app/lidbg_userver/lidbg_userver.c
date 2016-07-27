
#include "lidbg_servicer.h"
#include "lidbg_insmod.h"

#define LIDBG_UEVENT_MSG_LEN  (512)
#define LIDBG_UEVENT_NODE_NAME "lidbg_uevent"

struct parse_action_table
{
    char *action;
    void (*progress_action)(char *action_para);
};

struct uevent
{
    char *action;
    char *devpath;
    char *subsystem;
    char *devname;
    char *lidbg_action;
    char *lidbg_parameter;
};

static char *uevent_ignore = "cpu,power_supply";



//zone start [add a new item]
void progress_action_shell(char *action_para)
{

    if (!strncmp(action_para, "insmod", sizeof("insmod") - 1))
    {
        lidbg( "find cmd:%s\n", action_para);
        module_insmod((action_para + sizeof("insmod")));
    }
    else
        system(action_para);
}
struct parse_action_table lidbg_parse_action[] =
{
    { "shell", progress_action_shell },
};
//zone end

//@func: replace 'des't to 'replace' between drr to drr+len.
static void memreplace(char *drr, char dest, char replace, int len)
{
    int loop = 0;
    if(drr)
    {
        while((loop < len))
        {
            if((*(drr + loop) == dest))
                *(drr + loop) = replace;
            loop++;
        }
    }
}

static void lidbg_uevent_process( struct uevent *uevent)
{
    unsigned int loop;
    for(loop = 0; loop < SIZE_OF_ARRAY(lidbg_parse_action); loop++)
    {
        if (uevent->lidbg_action && uevent->lidbg_parameter && !strcmp(lidbg_parse_action[loop].action, uevent->lidbg_action))
        {
            lidbg_parse_action[loop].progress_action(uevent->lidbg_parameter);
            return ;
        }
    }
    lidbg( "err action, check: { '%s', '%s'}%d\n", uevent->lidbg_action, uevent->lidbg_parameter, (uevent->lidbg_parameter != NULL));
}
static void system_uevent_transfer(char *msg)
{
    LIDBG_UEVENT(msg);
}

static void parse_uevent(char *msg, struct uevent *uevent)
{
    uevent->action = "null";
    uevent->devpath = "null";
    uevent->subsystem = "null";
    uevent->devname = "null";
    uevent->lidbg_action = "null";
    uevent->lidbg_parameter = NULL;

    while (*msg)
    {
        if (!strncmp(msg, "ACTION=", 7))
        {
            msg += 7;
            uevent->action = msg;
        }
        else if (!strncmp(msg, "DEVPATH=", 8))
        {
            msg += 8;
            uevent->devpath = msg;
        }
        else if (!strncmp(msg, "SUBSYSTEM=", 10))
        {
            msg += 10;
            uevent->subsystem = msg;
        }
        else if (!strncmp(msg, "DEVNAME=", 8))
        {
            msg += 8;
            uevent->devname = msg;
        }
        else if (!strncmp(msg, "LIDBG_ACTION=", 13))
        {
            msg += 13;
            uevent->lidbg_action = msg;
        }
        else if (!strncmp(msg, "LIDBG_PARAMETER=", 16))
        {
            msg += 16;
            uevent->lidbg_parameter = msg;
        }
        while (*msg++)
            ;
    }
}

static bool lidbg_uevent_callback(int fd)
{
    static char msg[LIDBG_UEVENT_MSG_LEN + 1];
    struct uevent uevent;
    char *p = NULL;
    int n;

    if (fd < 0 )
        return -1;

    n = uevent_kernel_multicast_recv(fd, msg, LIDBG_UEVENT_MSG_LEN);
    if (n > LIDBG_UEVENT_MSG_LEN )
    {
        lidbg( "ERR-overflow-%d,%s\n", n, msg);
        return -1;
    }
    msg[n] = '\0';

    parse_uevent(msg, &uevent);

    if (uevent.devname && !strcmp(uevent.devname, LIDBG_UEVENT_NODE_NAME))
        lidbg_uevent_process(&uevent);
    else if (uevent.subsystem && strstr(uevent_ignore, uevent.subsystem) == NULL)
    {
        memreplace(msg, '\0', ' ', n - 1 );
        system_uevent_transfer(msg);
    }

    return 0;
}

static void lidbg_uevent_poll(bool (*uevent_callback)(int fd))
{
    struct pollfd ufd;
    int fd, nr;

    lidbg( "SUC,FUTENGFEI UEVENT INIT\n");

    fd = uevent_open_socket(64 * 1024, true);
    if (fd >= 0)
    {
        lidbg( "suc,uevent_open_socket\n");
#if defined(SOC_imx6q) || defined(SOC_mt35x)
        system("echo 1 > /dev/userver_ok.txt");
        system("chmod 777 /dev/userver_ok.txt");
#else
        system("echo 1 > /dev/log/userver_ok.txt");
        system("chmod 777 /dev/log/userver_ok.txt");
#endif
        fcntl(fd, F_SETFL, O_NONBLOCK);
        ufd.events = POLLIN;
        ufd.fd = fd;
        while(1)
        {
            ufd.revents = 0;
            nr = poll(&ufd, 1, -1);
            if (nr <= 0)
                continue;
            if (ufd.revents == POLLIN && uevent_callback)
                uevent_callback(ufd.fd);
        }
    }
    else
        lidbg( "err,uevent_open_socket\n");

}


static void *thread_uevent(void *data)
{

    pthread_t lidbg_uevent_tid;
    data = data;
    lidbg("lidbg_userver: uevent thread start\n"); 
    lidbg_uevent_poll(lidbg_uevent_callback);

    return ((void *) 0);

}

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
    pthread_t ntid;
    int ret;
    argc = argc;
    argv = argv;
    DUMP_BUILD_TIME_FILE;

    if(0)
       ret = pthread_create(&ntid,NULL,thread_uevent,NULL);

    #define SHELL_ERRS_FILE "/dev/dbg_msg"

    if(1)
    {
	 int fd,read_len;
	 char str[256];
         char shellstring[256];
         if(access("/dev/lidbg_uevent", X_OK) != 0)
         {
		system("chmod 777 /dev/lidbg_uevent");
		sleep(1);
	 }
	 fd = open("/dev/lidbg_uevent", O_RDWR);
	 if((fd == 0)||(fd == (int)0xfffffffe)|| (fd == (int)0xffffffff))
	 {
		lidbg("open /dev/lidbg_uevent err\n");
	 }
#if defined(SOC_imx6q) || defined(SOC_mt35x)
        system("echo 1 > /dev/userver_ok.txt");
        system("chmod 777 /dev/userver_ok.txt");
#else
        system("echo 1 > /dev/log/userver_ok.txt");
        system("chmod 777 /dev/log/userver_ok.txt");
#endif

#if 0
	while(1)
	 {
		memset(str,'\0',256);
		read_len = read(fd, str, 256);
		if(read_len >=0)
		{
		   lidbg("do:%s\n",str);
		   snprintf(shellstring, 256, "%s 2>> "SHELL_ERRS_FILE, str );
	           system(shellstring);
		}
	 }
#else
{
	struct epoll_event  events[1];
	int  nevents;
        int  epoll_fd = epoll_create(1);
 	epoll_register( epoll_fd, fd );
	while(1)
	{

		nevents = epoll_wait( epoll_fd, events, 1, -1 );
		if (nevents < 0)
		{
		    if (errno != EINTR)
		    {
			lidbg("epoll_wait() unexpected error: %s", strerror(errno));
		    }
		    continue;
		}
                if ((events[0].events & EPOLLIN) != 0)
		{
			memset(str,'\0',256);
			read_len = read(fd, str, 256);
			if(read_len >=0)
			{
//			   lidbg("do+:%s\n",str);
			   snprintf(shellstring, 256, "%s 2>> "SHELL_ERRS_FILE, str );
			   system(shellstring);
//			   lidbg("do-:%s\n",str);
			}
		}
	}
}
#endif
     }
    return 0;
}

