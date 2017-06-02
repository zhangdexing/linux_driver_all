
#include "lidbg_servicer.h"

int BUFSIZE;
int MAXINUM = (1024*1024*200);
int poll_time = 5;
bool save_in_time_mode = 0;
char * PATH =  "/data/reckmsg";
int openfd,seekfd;
int readsize = 0,savesize = 0;
char * buf = NULL;
char timebuf[32];
char newfile[128];
char file_seek[128];
void filesize_ctrl();

void update_seek()
{
	int seek = lseek(openfd, 0, SEEK_CUR);
	lseek(seekfd,0,SEEK_SET);
	write(seekfd,&seek,sizeof(seek));
}

void write_log()
{
	int total_size = klogctl(10,0,0);
	int seek = 0;
	BUFSIZE = total_size * 4;
        buf = (char *)malloc(BUFSIZE);
	memset(buf,'\0',BUFSIZE);

	lseek(seekfd,0,SEEK_SET);
	read(seekfd,&seek,sizeof(seek));
        lseek(openfd,seek,SEEK_SET);

	write(openfd,timebuf,strlen(timebuf));

	while(1)
	{
		readsize = klogctl(4,buf+savesize,total_size);
		//lidbg("record_klogctl:read %d,%d\n",readsize,total_size);
		savesize += readsize;
		if ((readsize > 0) && ( (savesize >= BUFSIZE - total_size) || (save_in_time_mode == 1)))
		{
			//lidbg("record_klogctl:write log to file\n");
			write(openfd,buf,savesize);
			readsize = savesize = 0;
			memset(buf,'\0',BUFSIZE);
			filesize_ctrl();
			update_seek();
		}
		sleep(poll_time);
	}
}



void filesize_ctrl()
{
	struct stat statbuff;  
	if(stat(newfile, &statbuff) < 0)
	{  
		lidbg("record_klogctl:get file size err\n");
	}

	if(statbuff.st_size > MAXINUM)
	{
		if(lseek(openfd, 0, SEEK_CUR) >= MAXINUM)
			lseek(openfd,0,SEEK_SET);
	}
}


void sigfunc(int sig)
{
	lidbg("record_klogctl:sigfunc write log to file+\n");

	if(sig == SIGUSR1)
	{
		lidbg("record_klogctl:sigfunc SIGUSR1\n");
		write(openfd,buf,savesize);
		readsize = savesize = 0;
		memset(buf,'\0',BUFSIZE);
		filesize_ctrl();
		update_seek();
	}
	else if (sig == SIGUSR2)
	{
		lidbg("record_klogctl:sigfunc change in time mode\n");
		MAXINUM = (1024*1024*500);
		poll_time = 1;
		save_in_time_mode = 1;
	}
	else
	{
		lidbg("record_klogctl:sigfunc exit\n");
		free(buf);
		close(openfd);
		exit(1);
	}
	lidbg("record_klogctl:sigfunc write log to file-\n");
}

int main(int argc , char **argv)
{
	time_t ctime;
	struct tm *tm;
	if(argc > 1)
		save_in_time_mode = strtoul(argv[1], 0, 0);
	if(save_in_time_mode == 1)
	{
		lidbg("record_klogctl:in time mode\n");
		MAXINUM = (1024*1024*500);
		poll_time = 1;
		PATH = "/sdcard/kmsg";
	}
	
	umask(0);//屏蔽创建文件权限
	int fd = mkdir(PATH,777);	
	if((fd < 0) && (errno != EEXIST))
	{
		lidbg("record_klogctl:mkdir file err");
	}

	ctime = time(NULL);
	tm = localtime(&ctime);
	sprintf(timebuf,"\n====#### %d-%02d-%02d_%02d-%02d-%02d\n",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);//将时间转换为字符串

	sprintf(newfile,"%s/kmsg.txt",PATH);
	sprintf(file_seek,"%s/seek",PATH);

	openfd = open(newfile,O_RDWR | O_CREAT,0777);
	seekfd = open(file_seek,O_RDWR | O_CREAT,0777);
	sprintf(newfile,"chmod 777 %s",PATH);
	system(newfile);
	sprintf(newfile,"chmod 777 %s/*",PATH);
	system(newfile);
	system("chmod 777 /sdcard/kmsg/* ");

	signal(SIGUSR1,sigfunc);
	signal(SIGUSR2,sigfunc);
	write_log();
	free(buf);
	close(openfd);
	close(seekfd);
	return 0;
}
