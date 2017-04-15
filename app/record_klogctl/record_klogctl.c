
#include "lidbg_servicer.h"

int BUFSIZE;
int MAXINUM = (1024*1024*15);
int poll_time = 5;
bool save_in_time_mode = 0;
char * PATH =  "/data/lidbg/reckmsg/";

int openfd;
int readsize = 0,savesize = 0;
char * buf = NULL;
char newfile[128];
void filesize_ctrl();


int remove_old_file()
{
        DIR *d;
        struct OLDERFILE{
                long mtime;
            	char filename[128];
        }olderfile;
        struct dirent *de;
        char fname[128];
        struct stat filebuf;
        int findnum = 0;
        d = opendir(PATH);
        long mtime = 0;
        if(d == NULL)
        {
                perror("remove_old_file err");
                exit(1);
        }
	lidbg("remove_old_file\n");
        while((de = readdir(d))!=NULL)
        {
		if(strncmp(de->d_name,".",1) == 0)
                        continue;
                sprintf(fname,"%s%s",PATH,de->d_name);
		if(strcmp(fname,newfile) == 0)
			continue;
                int exists = stat(fname,&filebuf);
                if(exists < 0)
                {
                        lidbg("Could not stat %s\n",de->d_name);
                }
                else
                {
                        if(findnum == 0)
                        {
                                olderfile.mtime = filebuf.st_mtime;
                                sprintf(olderfile.filename,"%s%s",PATH,de->d_name);
                                findnum ++;
                        }
                        else
                        {
                                if(filebuf.st_mtime<olderfile.mtime)
                                {
                                        olderfile.mtime = filebuf.st_mtime;
                                        sprintf(olderfile.filename,"%s%s",PATH,de->d_name);
                                }
                                findnum ++;
                        }
                        lidbg("%s:file->st_mtime is%d\n",de->d_name,filebuf.st_mtime);
                }
        }      
        lidbg("now remove %s\n",olderfile.filename);
        remove(olderfile.filename);
        return 0;
}


void write_log()
{
	int total_size = klogctl(10,0,0);
	BUFSIZE = total_size * 4;
        buf = (char *)malloc(BUFSIZE);
	memset(buf,'\0',BUFSIZE);
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
		}
		sleep(poll_time);
	}
}

void filesize_ctrl()
{
	DIR *d;
	struct dirent *de;
	struct stat filebuf;
	int exists;
	int totalsize;
        int filenum = 0;
	char filename[128];
	/********计算文件夹大小***********/
	d = opendir(PATH);
	
	if(d == NULL)
	{
		lidbg("record_klogctl:filesize_ctrl err");
		free(buf);
		close(openfd);
		exit(1);
	}

	totalsize = 0;
	while((de = readdir(d))!=NULL)
	{
		if(strncmp(de->d_name,".",1) == 0)//跳过目录.和..
			continue;
			
		sprintf(filename,"%s%s",PATH,de->d_name);
		exists = stat(filename,&filebuf);
		if(exists < 0)
		{
			lidbg("record_klogctl:Could not stat %s\n",de->d_name);
		}
		else
		totalsize += filebuf.st_size;
		filenum++;

	}
	//lidbg("record_klogctl:totalsize:%d\n",totalsize);
	closedir(d);
	
	if(totalsize > MAXINUM)
	{
		if(filenum == 1)
		{
			if(lseek(openfd, 0, SEEK_CUR) >= MAXINUM)
				lseek(openfd,0,SEEK_SET);
		}
		else
		{
			remove_old_file();
		}
	}
}


void sigfunc(int sig)
{
	lidbg("record_klogctl:sigfunc write log to file+\n");
	write(openfd,buf,savesize);
	readsize = savesize = 0;
	memset(buf,'\0',BUFSIZE);
	filesize_ctrl();

	if(sig == SIGUSR1)
	{
		lidbg("record_klogctl:sigfunc SIGUSR1\n");
	}
	else if (sig == SIGUSR2)
	{
		lidbg("record_klogctl:sigfunc change in time mode\n");
		MAXINUM = (1024*1024*50);
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
	save_in_time_mode = strtoul(argv[1], 0, 0);
	if(save_in_time_mode == 1)
	{
		lidbg("record_klogctl:in time mode\n");
		MAXINUM = (1024*1024*500);
		poll_time = 1;
		PATH = "/sdcard/kmsg/";
	}
	
	umask(0);//屏蔽创建文件权限
	int fd = mkdir(PATH,777);	
	if((fd < 0) && (errno != EEXIST))
	{
		lidbg("record_klogctl:mkdir file erro");
	}
	ctime = time(NULL);
	tm = localtime(&ctime);
	sprintf(newfile,"%s%d-%02d-%02d_%02d-%02d-%02d",PATH,tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);//将时间转换为字符串
	creat(newfile,777);//创建目标文件
	
	openfd = open(newfile,O_RDWR | O_CREAT,0777);
	system("chmod 777 /data/lidbg/reckmsg/* ");
	system("chmod 777 /sdcard/kmsg/* ");

	signal(SIGUSR1,sigfunc);
	signal(SIGUSR2,sigfunc);
	write_log();
	free(buf);
	close(openfd);
	return 0;
}
