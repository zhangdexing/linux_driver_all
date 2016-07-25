#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include "lidbg_servicer.h"
#define PROCESS "record_klogctl" //进程名字 record_klogctl
#define CMD1 "STORE"   //保存不退出
#define CMD2 "STORE_EXIT"     //保存退出

/*根据进程名字获取进程id*/
int get_pid_by_name(char* processname)
{
    DIR* dir;
    struct dirent *ptr;
    char filepath[100];
    FILE *fp;
    char buf[1024];
    char name[50];
    int pid = -1;
    
    dir = opendir("/proc");

    if(NULL == dir)
    {
      lidbg("sendsignal:open proc failed!\n");
        return -1;  
    }


    while( NULL != (ptr = readdir(dir)) ) //遍历所有目录
    {
        
        if( ( strcmp(ptr->d_name,".") == 0) || ( strcmp(ptr->d_name,"..") == 0 ) || ptr->d_type != DT_DIR ) //如果是隐藏目录或文件直接跳过
        {
            continue;
        }

        sprintf(filepath,"/proc/%s/status",ptr->d_name); // /proc/pid/status 保存进程所有信息

        fp = fopen(filepath,"r"); //只读方式打开
      
        if(NULL == fp)
        {
            continue;
        }

        if( NULL != fgets(buf,1024-1,fp) )
        {
            sscanf(buf,"%*s%s",name);

            if( strcmp(name,processname) == 0)
            {

               pid = atoi(ptr->d_name); //获取进程id

               fclose(fp);
               
               break; 
            }

        }

        fclose(fp);
    }

    closedir(dir);
    
    return pid;
}


int main(int argc,char* argv[])
{
   
   int pid;
   int sig_ret;
   int sig;

   if(argc < 2)
   {
    lidbg("sendsignal:Usage:./sendsignal cmd\n");
    lidbg("sendsignal:cmd can be : STORE or STORE_EXIT \n");
      return -1;
   }
   
   if(strcmp(argv[1],CMD1)==0)
   {
       sig = SIGUSR1;
   }

   else if(strcmp(argv[1],CMD2) == 0)
   {
       sig = SIGUSR2;
   }
   else
   {
     lidbg("sendsignal:unknown cmd!\n");
       return -1;
   }
   

   pid = get_pid_by_name(PROCESS);

   if(-1 == pid)
   {
     lidbg("sendsignal:get process pid failed!\n");
       return -1;
   }
   
 lidbg("sendsignal:pid is %d\n",pid);
  
   
   sig_ret = kill(pid,sig);
   if(-1 == sig_ret)
   {
     lidbg("sendsignal:send signal failed!\n");
   }
   

   return sig_ret;
}
