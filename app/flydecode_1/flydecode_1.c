
#include "lidbg_servicer.h"
#include "lidbg_insmod.h"
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

#define TAG "flydecode_1:"
#define DEBUGMSG lidbg
//#define DEBUGMSG printf
#define u8 	unsigned char
#define u16 unsigned short
#define u32	unsigned int

#define BYTE u8
#define UINT u32
#define UINT32 u32
#define BOOL bool
#define ULONG u32

#define FALSE 0
#define TRUE 1

typedef struct TENCYPTHEAD
{
    char flags[16];
    UINT data_size;
    BYTE version[4];
    UINT offset;
} ENCYPTHEAD, *PENCYPTHEAD;

int flyV2GetFileIntoBuff(char *filePath, BYTE **targetBuf, ULONG *len)
{
    int fd;
    struct stat stat;
    BYTE *fileData = NULL;

    fd = open(filePath, O_RDONLY);
    if(fd < 0)
    {
        DEBUGMSG(TAG" can't open -> %s error -> %s\n", filePath, strerror(errno));
        return FALSE;
    }

    if(fstat(fd, &stat))
    {
        DEBUGMSG(TAG"can't stat -> %s\n", filePath);
        return FALSE;
    }

    DEBUGMSG(TAG"%s len:%llu\n", filePath, stat.st_size);
    fileData = (BYTE *)malloc(stat.st_size);
    if(!fileData)
    {
        DEBUGMSG(TAG"malloc fail!\n");
        close(fd);
        return FALSE;
    }

    if(read(fd, fileData, stat.st_size) < 0)
    {
        DEBUGMSG(TAG"read -> %s fail!\n", filePath);
        close(fd);
        free(fileData);
        return FALSE;
    }

    *targetBuf = fileData;
    *len = stat.st_size;
    close(fd);
    fileData = NULL;

    return TRUE;
}



void flyV2FileDecode(UINT encodesize, BYTE *buf)
{
    UINT code1[4], code2[4];
    BYTE *pt;
    UINT m;
    volatile UINT i = 0, j;
    UINT BlockNum = (encodesize - 4) / 4;
    UINT leftNum = (encodesize - 4) % 4;

    pt = (BYTE *)buf;
    for(j = 0; j < 4; j++)
        code1[j] = *(pt + encodesize - 4 + j);

    while( i < BlockNum)
    {
        for(j = 0; j < 4; j++)
            code2[j] = *(pt + j);

        for(j = 0; j < 4; j++)
            *(pt + j) = (code1[j] ^ * (pt + j)) + 0x10;

        pt += 4;
        i  += 1;
        if( i == BlockNum)
            break;

        m = code2[0] % 4;
        switch(m)
        {
        case 0:
            code1[0] = code2[3];
            code1[1] = code2[2];
            code1[2] = code2[1];
            code1[3] = code2[0];
            break;
        case 1:
            code1[0] = code2[1];
            code1[1] = code2[2];
            code1[2] = code2[3];
            code1[3] = code2[0];
            break;
        case 2:
            code1[0] = code2[2];
            code1[1] = code2[3];
            code1[2] = code2[0];
            code1[3] = code2[1];
            break;
        case 3:
            code1[0] = code2[3];
            code1[1] = code2[0];
            code1[2] = code2[1];
            code1[3] = code2[2];
            break;
        default:
            break;
        }
    }

    if(leftNum)
    {
        for(j = 0; j < leftNum; j++)
            *(pt + j) = (code2[0] ^ * (pt + j)) + 0x10;
    }
}


int flyV2CopyFile(const char *destination, unsigned char *data, unsigned int size, mode_t mode)
{
    int count, last, x, num;

    if (0)//)ensure_path_mounted(destination) != 0)
    {
        DEBUGMSG(TAG" mount -> %s fail !\n", destination);
        return FALSE;
    }

    //mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    int dest_fd = open(destination, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, mode);
    if ((dest_fd < 0 ))
    {
        DEBUGMSG(TAG" open -> %s fail !\n", destination);
        return FALSE;
    }

    x = 0;
    count = size / 1024;
    last = size % 1024;
    while(count > 0)
    {
        if(write(dest_fd, data + x * 1024, 1024) != 1024)
        {
            DEBUGMSG(TAG" write data fail !\n");
            return FALSE;
        }
        fsync(dest_fd);
        count--;
        x++;
    }
    if(last > 0)
    {
        if(write(dest_fd, data + x * 1024, last) != last)
        {
            DEBUGMSG(TAG" write data end fail !\n");
            return FALSE;
        }
        fsync(dest_fd);
    }
    close(dest_fd);

    fsync(dest_fd);

    sync();
    if (chmod(destination, mode) < 0)
    {
        DEBUGMSG(TAG" fail modify mode:%d\n", mode);
    }
    else
    {
        DEBUGMSG(TAG" success modify mode:%d\n", mode);
    }
    return TRUE;
}

int flyDecodeFile( char *s,  char *d, mode_t mode)
{
    ULONG fileLen ;
    int ret = -1;
    BYTE *fileData = NULL;
    ENCYPTHEAD *p = NULL;

    DEBUGMSG(TAG"  %s[%s->%s]\n", __func__, s, d);
    ret =  flyV2GetFileIntoBuff(s, &fileData, &fileLen);
    if (FALSE == ret)
    {
        DEBUGMSG(TAG"  file fail !\n");
        return -1;
    }

    flyV2FileDecode(fileLen, fileData);
    p = (ENCYPTHEAD *)fileData;
    if(FALSE == flyV2CopyFile(d, (BYTE *)fileData + sizeof(ENCYPTHEAD), p->data_size, mode))
    {
        DEBUGMSG(TAG" copy file fail !\n");
        free(fileData);
        return FALSE;
    }
    return TRUE;
}
int get_chmod_mode_from_string(char *pmode)
{
    int mode = 0;
    const char *s = pmode;
    while (*s)
    {
        if (*s >= '0' && *s <= '7')
        {
            mode = (mode << 3) | (*s - '0');
        }
        else
        {
            DEBUGMSG(TAG" Bad mode\n");
            return 10;
        }
        s++;
    }
    return mode;
}
bool isDir(const char *s)
{
    struct stat st;
    stat(s, &st);
    return S_ISDIR(st.st_mode);
}

int flyDecodeDir(const char *s, const char *d, const char *type, mode_t mode)
{
    struct dirent *entry;
    char spath[512] = {0};
    char dpath[512] = {0};
    int count = 0;
    DEBUGMSG(TAG"  %s\n", __func__);

    DIR *dir = opendir(s);
    if (dir == NULL)
    {
        DEBUGMSG(TAG"Could not open %s\n", s);
    }
    else
    {
        while ((entry = readdir(dir)))
        {
            const char *file_name = entry->d_name;
            snprintf(spath, sizeof(spath), "%s/%s", s, file_name);
            snprintf(dpath, sizeof(dpath), "%s/%s", d, file_name);
            if(isDir(spath))
            {
                DEBUGMSG(TAG"skip dir: %s\n", file_name);
                continue;
            }
            if (type && strstr(file_name, type) == NULL)
            {
                DEBUGMSG(TAG"skip type: %s\n", file_name);
                continue;
            }
            DEBUGMSG(TAG"decode.%d->%s\n", ++count, spath);
            flyDecodeFile(spath, dpath, mode);
        }
    }
    return TRUE;
}
static void usage(const char *pname)
{
    DEBUGMSG("%s\n"
             "usage:   -s Filepath -d Filepath -f 1|0  -c 1|0  -m 755 \n"
             "   -h: this message\n"
             "   -s: source  absolute path\n"
             "   -d: Destination  absolute path\n"
             "   -t: file type -t txt\n"
             "   -m: rwx 777\n"
             "   -c: delete source path\n"
             "   -b: broascast:default 1\n"
             "   -f: is Dir Path\n", pname
            );
}
int main(int argc, char **argv)
{
    char *spath = NULL, *dpath = NULL, *type = NULL;
    int c, ret;
    int isDirPath = -1, mode = -1, isCleanSource = -1, isbroadCast = 1;
    DEBUGMSG(TAG" \n");
    while ((c = getopt(argc, argv, "s:d:f:t:m:c:b:h")) != -1)
    {
        DEBUGMSG(TAG"  %c-->%s\n", c, optarg);
        switch (c)
        {
        case 's':
            spath = (char *)optarg;
            break;
        case 'd':
            dpath = (char *)optarg;
            break;
        case 't':
            type = (char *)optarg;
            break;
        case 'f':
            isDirPath = atoi(optarg);
            break;
        case 'b':
            isbroadCast = atoi(optarg);
            break;
        case 'c':
            isCleanSource = atoi(optarg);
            break;
        case 'm':
            mode = get_chmod_mode_from_string(optarg);
            break;
        case '?':
        case 'h':
            usage("show help");
            return 1;
        }
    }
    DEBUGMSG(TAG"  s:%s d:%s t:%s m:%d f:%d\n", spath, dpath, type, mode, isDirPath);

    if(isDirPath == -1 || mode == -1 || spath == NULL || dpath == NULL)
    {
        usage("check para.");
        return FALSE;
    }
    if(isbroadCast)
        system("am broadcast -a cn.flyaudio.updateappAction --ei action 1");

    if(!isDirPath)
        ret = flyDecodeFile(spath, dpath, mode);
    else
        ret = flyDecodeDir(spath, dpath, type, mode);
    if(isCleanSource == 1)
    {
        DEBUGMSG(TAG"  clean dir start:%s\n", spath);
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "rm -rf %s", spath);
        system(cmd);
        DEBUGMSG(TAG"  clean dir stop:%s\n", cmd);
    }
    if(isbroadCast)
        system("am broadcast -a cn.flyaudio.updateappAction --ei action 0");
    DEBUGMSG(TAG"  success exit\n");
    return ret;
}
