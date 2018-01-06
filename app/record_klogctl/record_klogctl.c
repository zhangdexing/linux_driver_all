
#include "lidbg_servicer.h"

int BUFSIZE;
int MAXINUM = (1024 * 1024 * 100);
int poll_time = 5;
bool save_in_time_mode = 0;
char *PATH =  "/data/reckmsg";
int openfd, seekfd;
int readsize = 0, savesize = 0;
char *buf = NULL;
char timebuf[32];
char newfile[128];
char file_seek[128];
void filesize_ctrl();
#define TAG "record_klogctl:"
int total_size;
sem_t sem;


void update_seek()
{
    int seek = lseek(openfd, 0, SEEK_CUR);
    lseek(seekfd, 0, SEEK_SET);
    write(seekfd, &seek, sizeof(seek));
	if(save_in_time_mode == 0)
    	lidbg(TAG"update_seek.[%d]\n", seek);
}

void filesize_ctrl()
{
    struct stat statbuff;
    if(stat(newfile, &statbuff) < 0)
    {
        lidbg(TAG"get file size err\n");
        lidbg(TAG" %s Error?%s\n", newfile, strerror (errno));
    }

    if(statbuff.st_size > MAXINUM)
    {
        if(lseek(openfd, 0, SEEK_CUR) >= MAXINUM)
        {
            lseek(openfd, 0, SEEK_SET);
            lidbg(TAG" do lseek\n");
        }
    }
}
void do_log_get(int force_save)
{
	sem_wait(&sem);
    readsize = klogctl(4, buf + savesize, total_size);
    //lidbg(TAG"read %d,%d\n",readsize,total_size);
    savesize += readsize;
    if ((readsize > 0) && ( (savesize >= BUFSIZE - total_size) || (force_save == 1)))
    {
        //lidbg(TAG"write log to file\n");
        write(openfd, buf, savesize);
        readsize = savesize = 0;
        memset(buf, '\0', BUFSIZE);
        filesize_ctrl();
        update_seek();
    }
	sem_post(&sem);
}
void sigfunc(int sig)
{
    lidbg(TAG"sigfunc write log to file+\n");

    if(sig == SIGUSR1)
    {
        lidbg(TAG"sigfunc SIGUSR1\n");
        do_log_get(1);
    }
    else if (sig == SIGUSR2)
    {
        lidbg(TAG"sigfunc change in time mode\n");
        poll_time = 1;
        save_in_time_mode = 1;
    }
    else
    {
        lidbg(TAG"sigfunc exit\n");
        free(buf);
        close(openfd);
        exit(1);
    }
    lidbg(TAG"sigfunc write log to file-\n");
}

void write_log()
{
    int seek = 0;
    total_size = klogctl(10, 0, 0);
    BUFSIZE = total_size * 4;
    buf = (char *)malloc(BUFSIZE);
    memset(buf, '\0', BUFSIZE);

    lseek(seekfd, 0, SEEK_SET);
    read(seekfd, &seek, sizeof(seek));
    lseek(openfd, seek, SEEK_SET);

    lidbg(TAG"old seek.[%d],BUFSIZE[%d]\n", seek, BUFSIZE);

    write(openfd, timebuf, strlen(timebuf));

    while(1)
    {
        do_log_get(save_in_time_mode);
        sleep(poll_time);
    }
}

int main(int argc , char **argv)
{
    time_t ctime;
    struct tm *tm;


    umask(0);//????????
    int fd = mkdir(PATH, 777);
    if((fd < 0) && (errno != EEXIST))
    {
        lidbg(TAG"mkdir file err");
    }
    system("dmesg > /data/lidbg/kmsg.boot");

    ctime = time(NULL);
    tm = localtime(&ctime);
    sprintf(timebuf, "\n====#### %d-%02d-%02d_%02d-%02d-%02d\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec); //?????????

    sprintf(newfile, "%s/kmsg.txt", PATH);
    sprintf(file_seek, "%s/seek", PATH);

    openfd = open(newfile, O_RDWR | O_CREAT, 777);
    seekfd = open(file_seek, O_RDWR | O_CREAT, 777);

    {
        char temp[128];
        sprintf(temp, "chmod 777 %s", PATH);
        system(temp);
        sprintf(temp, "chmod 777 %s/*", PATH);
        system(temp);
    }

    signal(SIGUSR1, sigfunc);
    signal(SIGUSR2, sigfunc);

	if (sem_init(&sem, 0, 1) == -1) 
	{
        lidbg(TAG"Unable to sem_init (%s)", strerror(errno));
        exit(1);
    }
    write_log();

	sem_destroy (&sem);
    free(buf);
    close(openfd);
    close(seekfd);
    return 0;
}
