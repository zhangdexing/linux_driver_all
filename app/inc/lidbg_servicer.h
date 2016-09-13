#ifndef _LIGDBG_SERVER_APP__
#define _LIGDBG_SERVER_APP__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils/Log.h>
#include <cutils/uevent.h>
#include <cutils/properties.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <math.h>
#include <cutils/log.h>
//#undef printf
#define lidbg  LIDBG_PRINT
#define DUMP_BUILD_TIME    do{ lidbg( "Build Time: %s, %s, %s \n", __FUNCTION__, __DATE__, __TIME__);}while(0)
#define DUMP_POS     do{lidbg( "come to %s,%s,line %d\n", __FILE__,__FUNCTION__,__LINE__);}while(0)



#define DUMP_BUILD_TIME_FILE do{\
	char string[256];\
	sprintf(string, "echo %s build time:%s,%s >> /dev/log/lidbg_log.txt", __FILE__,__DATE__,__TIME__);\
	system(string);\
}while(0)

static inline bool is_file_exist(const char *file)
{
    if(access(file, F_OK) == 0)
        return 1;
    else
        return 0;

}

static inline int get_file_size(char *file)
{
    if(is_file_exist(file))
    {
        int fd;
        struct stat st;
        fd = open(file, O_RDONLY);
        if (fd < 0)
            return 0;

        if (fstat(fd, &st) < 0)
            return 0;

        return st.st_size;

    }
    else
    {
        return 0;
    }

}


#define LIDBG_WRITE(node,buf) do{\
	int fd;\
	 fd = open(node, O_RDWR);\
	 if(fd>=0){\
	 write(fd, &buf, sizeof(buf));\
	 close(fd);\
	 }\
}while(0)

#define LIDBG_CALL(cmd,buf,ret_bytes) do{\
	int fd;\
	 fd = open("/dev/mlidbg0", O_RDWR);\
	 if((fd == 0)||(fd == (int)0xfffffffe)|| (fd == (int)0xffffffff))break;\
	 write(fd, &cmd, sizeof(cmd));\
	 if((buf != NULL)&&(ret_bytes))\
	 {\
	 	read(fd, buf, ret_bytes);\
	 }\
	close(fd);\
}while(0)

#if 1

#define LOG_BYTES   (512)

static inline void LIDBG_PRINT(const char *fmt, ...)
{
    int fd;
    char buf[LOG_BYTES];
    char buf1[LOG_BYTES];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, LOG_BYTES, fmt, ap);
    va_end(ap);
    sprintf(buf1, "hal_msg:%s", buf);
    buf1[LOG_BYTES - 1] = '\0';
    if(is_file_exist("/dev/lidbg_msg"))
        fd = open("/dev/lidbg_msg", O_RDWR);
    else
        fd = open("/dev/dbg_msg", O_RDWR);
    if((fd == 0) || (fd == (int)0xfffffffe) || (fd == (int)0xffffffff))
        return;
    write(fd, buf1, /*sizeof(msg)*/strlen(buf1)/*LOG_BYTES*/);
    close(fd);
    return;
}

#endif

#define LIDBG_UEVENT(msg) do{\
	 int fd;\
	 fd = open("/dev/lidbg_uevent", O_RDWR);\
	 if((fd == 0)||(fd == (int)0xfffffffe)|| (fd == (int)0xffffffff))break;\
	 write(fd, msg, strlen(msg));\
	 close(fd);\
}while(0)


//java
/*
import java.io.FileOutputStream;

private void LIDBG_PRINT(String msg)
{
	final String LOG_E = "LIDBG_PRINT";
	msg = "JAVA:" + msg;
	byte b[] = msg.getBytes();
	FileOutputStream stateOutputMsg;
	try {
	stateOutputMsg = new FileOutputStream("/dev/lidbg_msg", true);
	stateOutputMsg.write(b);
	} catch (Exception e ) {
		Log.e(LOG_E, "Failed to set the fastboot state");
	}
}
LIDBG_PRINT("hello world\n");


*/

#define SIZE_OF_ARRAY(array_name) (sizeof(array_name)/sizeof(array_name[0]))

struct lidbg_dev_smem
{
    unsigned long smemaddr;
    unsigned long smemsize;
    unsigned long valid_offset;
};


#endif
