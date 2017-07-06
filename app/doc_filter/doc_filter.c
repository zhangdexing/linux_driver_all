
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

#define TAG "doc_filter:"
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
static int  debugMode = 0;
char *gwhitelist[512] = {NULL};
char *gblacklist[512] = {NULL};
bool isDir(const char *s)
{
    struct stat st;
    stat(s, &st);
    return S_ISDIR(st.st_mode);
}
int token_string(char *str, char *delims, char **token)
{
    int pos = 0;
    char *result = NULL;
    result = strtok( str, delims);
    while( result != NULL )
    {
        //DEBUGMSG(TAG "result is \"%s\"\n", result );
        *token = result;
        token++;
        pos++;
        result = strtok( NULL, delims );
    }
    return pos;
}
bool show_list(char *what, char *list[])
{
    int i = 0;
    while(list[i] != NULL)
    {
        DEBUGMSG(TAG "show_list--> [%s][%s]\n", what, list[i] );
        i++;
    }
    return false;
}
int debug_strstr = 0;
int wast_count = 0;
bool is_line_in_taglist(char *line, char *list[], bool debug)
{
    char *prop = NULL;
    int i = 0, loop = 0;
    int  waste_time_ms_while;
    struct timeval thisstart;
    struct timeval thisend;

    while(list[i] != NULL)
    {
        //return false;
        if(debug_strstr)
        {
            gettimeofday(&thisstart, NULL);
            for( loop = 0; loop < 200000000; loop++)
            {
                prop = strstr(line, list[i]);
            }
            gettimeofday(&thisend, NULL);
            waste_time_ms_while = (thisend.tv_sec - thisstart.tv_sec) * 1000 + (thisend.tv_usec - thisstart.tv_usec) / 1000;
            if(waste_time_ms_while > 0)
            {
                wast_count++;
                DEBUGMSG(TAG"filter:waste_time_ms_while_strstr:%d\n", waste_time_ms_while);
                DEBUGMSG(TAG "is_line_in_taglist-->wast_count [%d]\n[%s]\n", wast_count, line);
            }
        }
        else
            prop = strstr(line, list[i]);

        if (prop != NULL)
        {
            if(debug)
            {
                DEBUGMSG(TAG "---------------------------------------------------------\n");
                DEBUGMSG(TAG "is_line_in_taglist-->find [%s]\n[%s]\n", list[i], line);
            }
            return true;
        }
        i++;
    }
    return false;
}
bool del_file(char *file)
{
    char s[128];
    memset(s, '\0', sizeof(s));
    sprintf(s, "rm -rf %s", file);
    system(s);
    DEBUGMSG(TAG"rm old:%s\n", file);
    return true;
}
int filterFile( char *s,  char *blackd, char *whited, char *whitelist[], char *blacklist[],  int start_pos, int max_size)
{
    FILE *src_file = NULL;
    FILE *b_file = NULL;
    FILE *w_file = NULL;
    int fileSize = 0, need_reseek = 0;
    char line[4096];
    int byteread = 0;
    int  waste_time_ms_while;
    struct timeval thisstart;
    struct timeval thisend;

    del_file(blackd);
    del_file(whited);

    src_file = fopen(s, "r");
    if (src_file == NULL)
    {
        DEBUGMSG(TAG "can't open %s: %s\n", s, strerror(errno));
        return -1;
    }
    if(blackd)
    {
        b_file = fopen(blackd, "w");
        if (b_file == NULL)
        {
            fclose(src_file);
            DEBUGMSG(TAG "can't open %s: %s\n", blackd, strerror(errno));
            return -1;
        }
    }
    if(whited)
    {
        w_file = fopen(whited, "w");
        if (w_file == NULL)
        {
            DEBUGMSG(TAG "can't open %s: %s\n", whited, strerror(errno));
        }
    }

    fseek (src_file, 0, SEEK_END);
    fileSize = ftell (src_file);
    fseek (src_file, 0, SEEK_SET);

    if(start_pos < 0 || start_pos >= fileSize)
    {
        DEBUGMSG(TAG "pos oops(start_pos < 0 || start_pos >= fileSize): start_pos:%d fileSize:%d\n", start_pos, fileSize);
        start_pos = 0;
    }

    if(max_size < 0 ||  max_size > fileSize)
    {
        DEBUGMSG(TAG "pos oops: msize:%d fileSize:%d\n", max_size, fileSize);
        max_size = fileSize;
    }

    if(start_pos > 0 && (fileSize - start_pos) < max_size)
    {
        DEBUGMSG(TAG "pos oops(start_pos > 0 && start_pos < max_size): start_pos:%d max_size:%d fileSize:%d\n", start_pos, max_size, fileSize);
        need_reseek = 1;
    }

start:

    fseek (src_file, start_pos, SEEK_SET);

    DEBUGMSG(TAG "start:s:%s blackd:%s whited:%s fileSize:%d\n", s, blackd, whited, fileSize);
    DEBUGMSG(TAG "start:start_pos:%d max_size:%d need_reseek:%d %d/%d\n",  start_pos, max_size, need_reseek, whitelist != NULL, blacklist != NULL);
    gettimeofday(&thisstart, NULL);
    if (feof(src_file))
    {
        DEBUGMSG(TAG "pos oops(start but feof): ftell(src_file):%d fileSize:%d\n", (int)ftell(src_file), fileSize);
        fseek (src_file, 0, SEEK_SET);
    }
    while (fgets(line, sizeof(line), src_file))
    {
        if(blacklist != NULL)
        {
            if(!is_line_in_taglist(line, blacklist, false))
            {
                fputs(line, b_file);
                if(debugMode)
                    DEBUGMSG(TAG "b_file:%s\n", line);
                if(whitelist != NULL)
                {
                    if(is_line_in_taglist(line, whitelist, false))
                    {
                        fputs(line, w_file);
                        if(debugMode)
                            DEBUGMSG(TAG "w_file:%s\n", line);
                    }
                }
            }
        }
        //check feof
        if (feof(src_file))
        {
            if(need_reseek)
            {
                DEBUGMSG(TAG "pos oops(feof(src_file)&&need_reseek): ftell:%d max_size:%d\n", (int)ftell(b_file), max_size);
                fseek (src_file, 0, SEEK_SET);
            }
            else
            {
                DEBUGMSG(TAG "\nfeof(src_file)\n");
                break;
            }
        }

        //DEBUGMSG(TAG "break111:ftell:%d max_size:%d byteread:%d\n", ftell (b_file), max_size, byteread);
        //check maxsize
        if(max_size > 0 )
        {
            byteread += strlen(line);
            if(byteread >= max_size)
            {
                DEBUGMSG(TAG "break:ftell:%d max_size:%d,byteread:%d\n", (int)ftell(b_file), max_size, byteread);
                break;
            }
        }
    }

    gettimeofday(&thisend, NULL);
    waste_time_ms_while = (thisend.tv_sec - thisstart.tv_sec) * 1000 + (thisend.tv_usec - thisstart.tv_usec) / 1000;
    DEBUGMSG(TAG"filter:waste_time_ms_while11111111:%d\n", waste_time_ms_while);

    fclose(src_file);
    fclose(b_file);
    if (w_file != NULL)
        fclose(w_file);
    return TRUE;
}
int32_t getFileData (const char *pFilename, uint8_t **pBfr, uint32_t *pFileSize)
{
    int32_t result = 0;
    FILE *fp = NULL;
    uint32_t nBytesRead = 0;

    if (pFilename == NULL || pFileSize == NULL || pBfr == NULL)
    {
        DEBUGMSG(TAG"(pFilename == NULL || pFileSize == NULL || pBfr == NULL)");
        return -1;
    }
    else
    {
        fp = fopen(pFilename, "r");
        if (fp == NULL)
        {
            DEBUGMSG(TAG" fail open:%s\n", pFilename);
            return -1;
        }
        fseek (fp, 0, SEEK_END);
        *pFileSize = ftell (fp);
        fseek (fp, 0, SEEK_SET);

        *pBfr = (uint8_t *)malloc(*pFileSize);
        if(*pBfr != NULL)
        {
            nBytesRead = (uint32_t) fread (*pBfr, sizeof (uint8_t), (size_t) * pFileSize, fp);
        }
        else
        {
            DEBUGMSG(TAG" malloc(*pFileSize)");
            result = -1;
        }
        if(nBytesRead != *pFileSize)
        {
            DEBUGMSG(TAG"nBytesRead != *pFileSize");
            result = -1;
            if(*pBfr != NULL)
            {
                free(*pBfr);
            }
        }
        fclose (fp);
    }
    return result;
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
             "   -s: source  absolute path\n"
             "   -d: blackdpath\n"
             "   -w: whitedpath\n"
             "   -t: whitelistpath\n"
             "   -y: blacklistpath\n"
             "   -p: start_pos\n"
             "   -l: mmax_size\n"
             "   -m: isDropMode\n"
             "   -c: cut_file\n"
             "   -b: debugMode\n"
             , pname
            );
}
//doc_filter -s /sdcard/kmsg_test.txt -d /sdcard/kmsg_b.txt -w /sdcard/kmsg_w.txt -t /sdcard/kmsg_wl.txt -y /sdcard/kmsg_wb.txt -c /sdcard/kmsg_e.txt -m 1 -p 0 -l 15728640 -b 0
int main(int argc, char **argv)
{
    char *spath = NULL, *blackdpath = NULL, *whitedpath = NULL, *whitelistpath = NULL, *blacklistpath = NULL, *cut_file = NULL;
    int c = 0, ret = 0, tagCount = 0, isDropMode = 0, start_pos = 0, mmax_size = -1;
    uint8_t *pBfr = NULL;
    uint8_t *pBfr2 = NULL;
    uint32_t pFileSize = 0;
    int  waste_time_ms;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);
    system("echo ======doc_start========> /dev/lidbg_msg");

    DEBUGMSG(TAG"==================step1=================== \n");
    while ((c = getopt(argc, argv, "s:d:w:t:y:p:l:m:c:b:h")) != -1)
    {
        //DEBUGMSG(TAG"  %c-->%s\n", c, optarg);
        switch (c)
        {
        case 's':
            spath = (char *)optarg;
            break;
        case 'd':
            blackdpath = (char *)optarg;
            break;
        case 'w':
            whitedpath = (char *)optarg;
            break;
        case 't':
            whitelistpath = (char *)optarg;
            break;
        case 'y':
            blacklistpath = (char *)optarg;
            break;
        case 'p':
            start_pos = atoi(optarg);
            break;
        case 'l':
            mmax_size = atoi(optarg);
            break;
        case 'm':
            isDropMode = atoi(optarg);
            break;
        case 'c':
            cut_file = (char *)optarg;
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

    DEBUGMSG(TAG"spath:%s blackdpath:%s whitedpath:%s whitelistpath:%s \n", spath, blackdpath , whitedpath, whitelistpath);
    DEBUGMSG(TAG"blacklistpath:%s start_pos:%d mmax_size:%d cut_file:%s\n", blacklistpath, start_pos, mmax_size, cut_file);

    if( spath == NULL || blackdpath == NULL || whitelistpath == NULL)
    {
        usage("check para.");
        return FALSE;
    }
    if(whitelistpath)
    {
        if(getFileData(whitelistpath, &pBfr, &pFileSize) < 0)
        {
            DEBUGMSG(TAG"gwhitelist:error \n");
            //goto out;
        }
        else
        {
            tagCount = token_string(pBfr, "\n", &gwhitelist);
            show_list("gwhitelist", gwhitelist);
            DEBUGMSG(TAG"gwhitelist:pFileSize:%d tagCount:%d\n", pFileSize, tagCount);
        }
    }

    if(blacklistpath)
    {
        if(getFileData(blacklistpath, &pBfr2, &pFileSize) < 0)
        {
            goto out;
        }
        else
        {
            tagCount = token_string(pBfr2, "\n", &gblacklist);
            show_list("gblacklist", gblacklist);
            DEBUGMSG(TAG"gblacklist:pFileSize:%d tagCount:%d\n", pFileSize, tagCount);
        }
    }

    if(strstr(spath, "kmsg"))
    {
        int pos = get_pos_from_file(SEEKPATH);
        if(pos > 0)
        {
            start_pos = pos;
            DEBUGMSG(TAG"kmsg:start_pos=%d\n", start_pos);
        }
        else
        {
            DEBUGMSG(TAG"miss:%s\n", SEEKPATH);
        }
    }

    ret = filterFile(spath, blackdpath, whitedpath, gwhitelist, gblacklist, start_pos, mmax_size);
    {
        char s[128];
        memset(s, '\0', sizeof(s));
        sprintf(s, "chmod 777 %s", blackdpath);
        system(s);
        DEBUGMSG(TAG"%s\n", s);
    }

out:
    free(pBfr);
    free(pBfr2);
    DEBUGMSG(TAG"exit:ret->%d\n", ret);
    gettimeofday(&end, NULL);
    waste_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
    DEBUGMSG(TAG"filter:waste_time_ms:%d\n", waste_time_ms);
    if(cut_file)
    {
        //"   -e: is_from_end\n"
        //"   -p: pos\n"
        //"   -l: size\n"
        // "   -b: debugMode\n"
        char s[256];
        int seek = 0;
        memset(s, '\0', sizeof(s));
        DEBUGMSG(TAG"==================step2=================== \n");
        DEBUGMSG(TAG"cut_file\n");
        if(strstr(cut_file, "kmsg"))
        {
            seek = get_pos_from_file(SEEKPATH);
            if(seek < 0)
            {
                DEBUGMSG(TAG"miss:%s\n", SEEKPATH);
                seek = 0;
            }
            if(access(blackdpath, F_OK) == -1)
            {
                DEBUGMSG(TAG"miss:%s\n", blackdpath);
                DEBUGMSG(TAG"use:%s\n", spath);
                blackdpath = spath;
            }
            sprintf(s, "/flysystem/lib/out/doc_split -e 1 -l 15728640 -s %s -d %s -b %d", blackdpath, cut_file, debugMode);// -p %dseek,
        }
        else  if(strstr(cut_file, "logcat"))
        {
            seek = 0;
            if(access(blackdpath, F_OK) == -1)
            {
                DEBUGMSG(TAG"miss:%s\n", blackdpath);
                DEBUGMSG(TAG"use:%s\n", spath);
                blackdpath = spath;
            }
            sprintf(s, "/flysystem/lib/out/doc_split -s %s -d %s -p %d -l 15728640 -b %d", blackdpath, cut_file, seek, debugMode);
        }
        DEBUGMSG(TAG"start:[%s]\n", s);
        system(s);
        gettimeofday(&end, NULL);
        waste_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        DEBUGMSG(TAG"split:waste_time_ms:%d\n", waste_time_ms);
        system("cp -rf /data/lidbg /sdcard/FlyLog/DriBugReport/drivers/");
        system("am broadcast -a cn.flyaudio.uploadlog.driver");
        gettimeofday(&end, NULL);
        waste_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        system("echo ======doc_stop========> /dev/lidbg_msg");
        DEBUGMSG(TAG"broadcast:waste_time_ms:%d\n", waste_time_ms);
    }
    return ret;
}
