
#include "lidbg_servicer.h"
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define TAG "doc_split:"
#define DEBUGMSG lidbg
//#define DEBUGMSG printf
#define SEEKPATH "/data/reckmsg/seek"

typedef signed char	      int8_t;
typedef unsigned char	    u_int8_t;
typedef short		     int16_t;
typedef unsigned short	   u_int16_t;
typedef int		     int32_t;
typedef unsigned int	   u_int32_t;

#define FALSE 0
#define TRUE 1


int filtersplit( char *s,  char *d, int msize, int mis_from_end, int mpos, int mdebugMode)
{
    FILE *src_file = NULL;
    FILE *dec_file = NULL;
    int fileSize = 0, fileReadPos = 0, readSize = 0, ret = -1;
    int readBytes = 0;
    int need_reseek = 0;

    src_file = fopen(s, "r");
    if (src_file == NULL)
    {
        DEBUGMSG(TAG "can't open %s: %s\n", s, strerror(errno));
        return ret;
    }
    dec_file = fopen(d, "w");
    if (dec_file == NULL)
    {
        fclose(src_file);
        DEBUGMSG(TAG "can't open %s: %s\n", d, strerror(errno));
        return ret;
    }

    fseek (src_file, 0, SEEK_END);
    fileSize = ftell (src_file);

    if(msize > fileSize)
    {
        DEBUGMSG(TAG "pos oops(msize > fileSize): msize:%d fileSize:%d\n", msize, fileSize);
        msize = fileSize;
        mpos = 0;
    }


    if(msize > 0)
    {
        if(mpos < msize)
        {
            DEBUGMSG(TAG "pos oops(start_pos < max_size)1: start_pos:%d max_size:%d fileSize:%d\n", mpos, msize, fileSize);
            need_reseek = 1;
            fileReadPos = fileSize - (msize - mpos);
            DEBUGMSG(TAG "pos oops(start_pos < max_size)2: start_pos:%d need_reseek:%d\n", mpos, need_reseek);
        }
        else
        {
            DEBUGMSG(TAG "pos oops(start_pos >max_size)1: start_pos:%d max_size:%d fileSize:%d\n", mpos, msize, fileSize);
            fileReadPos = msize - mpos; //seek to 0 to filter
        }
    }

    if(mis_from_end)
        fileReadPos = fileSize - msize;

    if(fileReadPos < 0)
    {
        DEBUGMSG(TAG "pos oops(start_pos<0): fileReadPos:%d need_reseek:%d\n", fileReadPos, need_reseek);
        fileReadPos = 0;
    }

    fseek (src_file, fileReadPos, SEEK_SET);
    DEBUGMSG(TAG "msize:%d fileSize:%d mis_from_end:%d mpos:%d mdebugMode:%d  fileReadPos:%d\n", msize, fileSize, mis_from_end, mpos, mdebugMode,  fileReadPos);

    char line[4096];
    bool line_in_taglist = false;
    while (readSize < msize)
    {
        int readsize = 0;
        if(msize - readSize < (int)sizeof(line))
            readsize = msize - readSize ;
        else
            readsize = (int)sizeof(line);
        readBytes = fread(line, 1, readsize, src_file);
        if(readBytes >= 0)
        {
            fwrite(line, 1, readBytes, dec_file);
            readSize += readBytes;
        }
        else
        {
            DEBUGMSG(TAG "\nfread:somthing error,break\n");
            break;
        }

        if (feof(src_file))
        {
            if(need_reseek)
            {
                DEBUGMSG(TAG "pos oops(feof(src_file)&&need_reseek): ftell:%d max_size:%d\n", (int)ftell(dec_file), readSize);
                fseek (src_file, 0, SEEK_SET);
            }
            else
            {
                DEBUGMSG(TAG "\nfeof(src_file)\n");
                break;
            }
        }
    }
    ret = 1;
    fclose(src_file);
    fclose(dec_file);
    return ret;
}
int get_pos_from_file(char *path)
{
    int seekfd, ret = -1;
    seekfd = open(path, O_RDWR, 0777);
    if(seekfd > 0)
    {
        lseek(seekfd, 0, SEEK_SET);
        read(seekfd, &ret, sizeof(ret));
        close(seekfd);
    }
    else
        ret = -1;
    return ret;
}
static void usage(const char *pname)
{
    DEBUGMSG(TAG"%s\n"
             "usage:   -s Filepath -d Filepath \n"
             "   -h: this message\n"
             "   -s: in  absolute path\n"
             "   -d: out  absolute path\n"
             "   -e: is_from_end\n"
             "   -p: pos\n"
             "   -l: size\n"
             "   -b: debugMode\n"
             , pname
            );
}
//time doc_split -s /sdcard/logcat1.txt -d /sdcard/1.txt -e 1 -l 5 -b 1
int main(int argc, char **argv)
{
    char *spath = NULL, *dpath = NULL ;
    int c = 0, ret = 0, size = 0, pos = 0, is_from_end = 0, debugMode = 0;

    DEBUGMSG(TAG"in\n");
    while ((c = getopt(argc, argv, "s:d:e:p:l:b:h")) != -1)
    {
        //DEBUGMSG(TAG"  %c-->%s\n", c, optarg);
        switch (c)
        {
        case 's':
            spath = (char *)optarg;
            break;
        case 'd':
            dpath = (char *)optarg;
            break;
        case 'e':
            is_from_end = atoi(optarg);
            break;
        case 'p':
            pos = atoi(optarg);
            break;
        case 'l':
            size = atoi(optarg);
            break;
        case 'b':
            debugMode = atoi(optarg);
            break;
        case '?':
        case 'h':
            usage("show help");
            return 1;
        }
    }
    DEBUGMSG(TAG"spath:%s dpath:%s is_from_end:%d pos:%d size:%d debugMode:%d\n", spath, dpath, is_from_end, pos, size, debugMode);

    if( spath == NULL || (is_from_end > 0 && pos > 0) )
    {
        usage("check para.");
        return FALSE;
    }
    {
        char s[128];
        memset(s, '\0', sizeof(s));
        sprintf(s, "rm -rf %s", dpath);
        system(s);
        DEBUGMSG(TAG"rm old:%s\n", dpath);
    }
    if(strstr(spath, "kmsg"))
    {
        int tpos = get_pos_from_file(SEEKPATH);
        if(tpos > 0)
        {
            pos = tpos;
            DEBUGMSG(TAG"kmsg:start_pos=%d\n", tpos);
        }
    }

    ret = filtersplit(spath, dpath, size, is_from_end, pos, debugMode);
    {
        char s[128];
        memset(s, '\0', sizeof(s));
        sprintf(s, "chmod 777 %s", dpath);
        system(s);
        DEBUGMSG(TAG"%s\n", s);
    }

    DEBUGMSG(TAG"exit:ret=%d\n", ret);
    return ret;
}
