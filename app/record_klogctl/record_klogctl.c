#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>       
#include <sys/klog.h>
#include "lidbg_servicer.h"

/***********************宏定义******************************************/
#define BUFSIZE 1024*512+1        //读取缓冲区大小
#define MAXSIZE 1024*1024*10       //存储目录容量最大10M
#define ERROR_FILE_MAXSIZE 1024*1024 //记录错误信息的文件最大1M
#define DESTDIR "/data/lidbg/reckmsg"          //读取信息存储路径
#define CONFIGFILE DESTDIR"/.filename"     //记录下次写入的文件以及当前存储文件数目
#define TARGET_STR "error"         //需要保存的信息包含的关键字
#define STROE_EMSG_FILE DESTDIR"/errormsg.txt" //错误信息保存文件
/************************全局变量***********************************************/ 
static int total = 0; 
char filename[256];                //存储文件名
int filenum;                       //文件夹存储的文件数
int new_fd = -1 ;                        //存储文件.txt fd
int emsg_fd = -1;                       //存储错误信息的文件 fd
int direction = 2;                   //文件向前增长或向后覆盖标志  
int iswriten;                      //检测缓冲区所有信息是否写入文件标志
int disposed;                      //异常退出时处理标志
char *buf = NULL;                 //读取缓存区
char *tbuf = NULL;                   //读取辅助缓存，大小为日志缓冲区大小
char *hbuf = NULL;                //记录错误信息的日志缓冲区
int kmsg_buffer_len = 0;
bool debug = 0;
/************************************************************
*函数名称：check_storedir
*函数功能：判读存储目录是否存在，不存在则创建该目录
*输入：    目录路径
*输出：    调试信息
*返回值：  成功返回0  失败返回-1
*作者：    flyaudio-xcg
*日期：    2016-7-20
*************************************************************/
int check_storedir(char* pathname)
{
    DIR* dir;
    int status;

    dir = opendir(pathname);
    if(dir == NULL)
    {
       lidbg("record_klogctl:no such dir and create it\n");
       /*创建目录drmxr-xr-x*/
       status = mkdir(pathname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
       if(-1 == status)
       {
            lidbg("record_klogctl:mkdir failed!\n");
            return -1;
       }
    }
    else
    {
       closedir(dir);
    }

    return 0;
}



/************************************************************
*函数名称：get_totalsize
*函数功能：获取目录大小
*输入：    目录路径
*输出：    调试信息
*返回值：  目录大小
*作者：    flyaudio-xcg
*日期：    2016-7-18
*************************************************************/

int sum(const char *fpath, const struct stat *sb, int typeflag) /*回调函数*/
{ 
     total += sb->st_size;  /*累计文件大小*/
     return 0; 
}  
 
int get_totalsize(const char* path)
{
    total = 0;
    if(access(path, R_OK))
    {
        lidbg("record_klogctl:can't access file %s",path);
        return -1;
    }

    
    if(ftw(path, &sum, 1))
    {
        perror("ftw");
        return -1;
    }
      
    return total;

}

/************************************************************
*函数名称：get_filename_num
*函数功能：获取需要写入的文件名，当前存储目录存储的文件数目
*输入：    存储目标文件名的数组，该数组大小
*输出：    调试信息，下次调试信息存储的文件名和存储文件数目
*返回值：  目标目录当前存储文件数目
*作者：    flyaudio-xcg
*日期：    2016-7-18
*************************************************************/

int get_filename_num(char *filename,int size)
{
    int fd;
    int filenum;
    char buf[256];
   
    if(-1 == access(CONFIGFILE,0))
    {
        lidbg("record_klogctl:%s not exist!\n",CONFIGFILE);
        fd = open(CONFIGFILE,O_RDWR|O_CREAT,0777);

        if(-1 == fd)
        {
            lidbg("record_klogctl:can't create file:%s\n",CONFIGFILE);
            return -1;
        }
        
        lidbg("record_klogctl:create file %s\n",CONFIGFILE);       
 
        write(fd,"1.txt--1\n",strlen("1.txt--1\n"));

        memcpy(filename,"1.txt",strlen("1.txt"));

        filenum = 1;   
    }

    else
    {
        fd = open(CONFIGFILE,O_RDWR);

        if(-1 == fd)
        { 
            lidbg("record_klogctl:open file error!\n");
            return -1;
        }
        memset(buf,0,sizeof(buf));
        read(fd,buf,sizeof(buf));
        buf[strlen(buf)-1]='\0';

        char* token = strtok(buf,"--");

        memcpy(filename,token,strlen(token));

        filenum = atoi(strtok(NULL,"--"));  
    }
    
    close(fd);
    return filenum;
}


/************************************************************
*函数名称：monitor_filesize
*函数功能：查看当前存储目录的大小，如果大于MAXSIZE(10M)则覆盖文件
*输入：    NULL
*输出：    调试信息，需要覆盖的文件名
*返回值：  NULL
*作者：    flyaudio-xcg
*日期：    2016-7-18
*************************************************************/

void monitor_filesize(void)
{
   char _filename[256];  
   struct stat statbuff; 
   int filesize;
    
   stat(STROE_EMSG_FILE, &statbuff);
   filesize = statbuff.st_size; 
   if( filesize >= ERROR_FILE_MAXSIZE )
   {
       lseek(emsg_fd,0,SEEK_SET); //记录错误信息的文件大小超过限定值时，覆盖该文件
   }


   if((get_totalsize(DESTDIR)+BUFSIZE) >= MAXSIZE) //达到最大存储量，开始覆盖
   {
      if(filenum == 1) //只有一个文件情况下
      {
         direction = 0;
         lseek(new_fd,0,SEEK_SET); //将文件指针移到文件头
      }
      else         //如果有多个文件则覆盖时间最久的那个
      {
         get_filename_num(_filename,sizeof(_filename));
         char* s = strtok(_filename,".");

         if(atoi(s)==filenum)     //向前覆盖
         {
             direction = -1;
             memset(_filename,0,sizeof(_filename));
             strcpy(_filename,"1.txt");
             sprintf(filename,"%s/%s",DESTDIR,_filename);
         }
         else      //向后覆盖
         {
             direction = 1;
             sprintf(filename,"%s/%d.txt",DESTDIR,atoi(s)+1);
         }


         close(new_fd);  //关闭之前的文件
        
         new_fd = open(filename,O_RDWR|O_CREAT,0777); //打开将要写入的文件
         if(-1 == new_fd)
         {
             perror("open file");
             exit(-1);
         }
     
         lseek(new_fd,0,SEEK_SET);          //将文件指针移动到文件头部
   
      }
   }

}


/************************************************************
*函数名称：refrech_record
*函数功能：更新CONFIGFILE保存的信息：下次写入的文件--当前文件数目
*输入：    当前调试信息写入的文件名，目标目录存储记录文件数目
*输出：    调试信息，需要覆盖的文件名
*返回值：  成功返回0  失败返回-1
*作者：    flyaudio-xcg
*日期：    2016-7-18
*************************************************************/
int refrech_record(char* filename,int filenum)
{
    int fd = open(CONFIGFILE,O_RDWR);
    char str[50];

    if(-1 == fd)
    {
        return -1;
    }

    memset(str,0,sizeof(str));

    if(direction == 0) //存储目录中只有一个文件情况
    {
        lidbg("record_klogctl:direction == 0\n");

        sprintf(str,"1.txt--%d\n",1);
        write(fd,str,strlen(str));       
    }
    else
    { 
        char* div;
        char* s;
  
        if(NULL != strstr(filename,"/")) 
        {
            s = strtok(filename,"/");
            while((div = strtok(NULL,"/")) != NULL)
            {
                s = div;
            }
        }
             
        s = strtok(s,".");                   
 
        int num = atoi(s);
           
        if(direction == -1 || direction == 1 ) //创建向后覆盖或者创建新文件
        {

            if(num == filenum) num = 1; //当前已经覆盖最后一个文件了，下次需要覆盖第一个文件
            else num = num+1;            //下次覆盖下一个文件

            sprintf(str,"%d.txt--%d\n",num,filenum);    
        }
        else  //这次读写没有产生文件覆盖情况
        {

            if(get_totalsize(DESTDIR) >= MAXSIZE) //当前大小已经超过最大容量，需要覆盖
            {
                if(num == filenum)  //之前没有覆盖过任何文件，或者已经覆盖n轮
                {

                    sprintf(str,"1.txt--%d\n",filenum);
                }
                else 
                {

                    sprintf(str,"%d.txt--%d\n",num+1,filenum);
                }
            }
            else     //下次读写创建新文件存储
            {

                sprintf(str,"%d.txt--%d\n",num+1,filenum+1);
            }
            
        }

        write(fd,str,strlen(str));
    }

    close(fd);

    return 0;

}


/*******************************************************************
*函数名称：handler
*函数功能：异常退出信号捕获处理（CTR+C），将缓存区读取的信息写入记录文件
           更新CONFIGFILE信息，关闭文件
*输入：    NULL
*输出：    调试信息
*返回值：  void
*作者：    flyaudio-xcg
*日期：    2016-7-18
*********************************************************************/
void handler(int sig)
{
    lidbg("record_klogctl:write content from buf to file\n");
    /*将缓冲区的消息写入文件*/
    if(strlen(buf) > 0)
     {
         buf[strlen(buf)-1]='\0';
         write(new_fd,buf,strlen(buf));
         memset(buf,0,BUFSIZE);
     }

     if(strlen(tbuf) > 0)
     {
         write(new_fd,tbuf,strlen(tbuf)); 
         memset(tbuf,0,kmsg_buffer_len+1);
     }     

    /*如果时KILL信号，或者自定义信号2则保存退出*/
    if(!disposed && (sig == SIGUSR2 || sig== SIGKILL || sig == SIGINT) )
    {
        refrech_record(filename,filenum);
        lidbg("record_klogctl:refrech_record\n");
    
        if(-1 != new_fd)
            close(new_fd);

        if(-1 != emsg_fd)
            close(emsg_fd);

         free(buf);
         free(tbuf);
         exit(0);
    }
  
}

/*******************************************************************
*函数名称：store_error_msg
*函数功能：将包含关键字的错误信息存储到文件
*输入：    存储信息的缓冲区buf
*输出：    调试信息
*返回值：  void
*作者：    flyaudio-xcg
*日期：    2016-7-18
*********************************************************************/

void store_error_msg(char *buf)
{
     char* ebuf = NULL;
     char* strtok_buf;
     char  error_tbuf[1024];
    
     memset(error_tbuf,0,sizeof(error_tbuf)); //清空缓冲区

     strtok_buf = strtok(buf,"\n");      //按行分割buf信息
   
     ebuf = strstr(strtok_buf,TARGET_STR);
     if(NULL != ebuf)
     {
         strcat(error_tbuf,strtok_buf);
         strcat(error_tbuf,"\n");    
     }

     while( NULL != (strtok_buf = strtok(NULL,"\n"))) //读取所有行，判断是否包含TARGETSTR，如果有则暂存到error_tbuf,全部读完后写入文件
     {
         ebuf = strstr(strtok_buf,TARGET_STR);
         if(NULL != ebuf)
         {
             strcat(error_tbuf,strtok_buf);  
             strcat(error_tbuf,"\n");         
         }
             
     }

     if(strlen(error_tbuf) > 0)
     {
         error_tbuf[strlen(error_tbuf)-1] = '\0';
         //lidbg("record_klogctl:"error message stored in error_buf : \n%s\n",error_tbuf);
         
         write(emsg_fd,error_tbuf,strlen(error_tbuf));
      
         lidbg("record_klogctl:store error message into file\n");
     }

}

/*******************************************************************
*函数名称：main
*函数功能：读取/proc/kmsg存储的内核输出信息，写入到/data下的DESTDIR，最大
           写入量为MAXSIZE（10M），如果写入量大于10M，进行文件覆盖处理
*输入：    NULL
*输出：    调试信息
*返回值：  成功返回0  失败返回-1 
*作者：    flyaudio-xcg
*日期：    2016-7-18
*********************************************************************/
int main(int argc,char* argv[])
{   

    long long total = 0;
    int ret = 0;
    int count;

    DUMP_BUILD_TIME_FILE;
    lidbg("record_klogctl start\n");

    /*判断存储目录是否存在，如果不存在则创建该目录，出错则退出程序*/
    if(-1 == check_storedir(DESTDIR))
        return -1;

    /*获取需要写入的文件名 和 当前存储的文件数量*/
    if((filenum = get_filename_num(filename,sizeof(filename)))==-1) 
    {
       ret = -1;
       goto end;
    }

    /*获取需要写入的文件路径，读写方式打开，失败则退出程序*/
    char tmp[50];
    sprintf(tmp,"%s/%s",DESTDIR,filename);
    strcpy(filename,tmp);

    new_fd = open(filename,O_RDWR|O_CREAT,0777);  
    
    if(-1 == new_fd)  
    {
        lidbg("record_klogctl:create file %s error and exit!\n",filename);
        goto end;
    }


    //追加方式打开或创建STROE_EMSG_FILE 错误信息存储文件
    emsg_fd = open(STROE_EMSG_FILE,O_RDWR|O_APPEND|O_CREAT,0777);
    
    if(-1 == emsg_fd )
    {
        lidbg("record_klogctl:open file %s error and exit!\n",STROE_EMSG_FILE);
        goto end;
    }

   /*分配内存*/
    buf = (char *)malloc(BUFSIZE);
    if(buf == NULL)
    {
        lidbg("record_klogctl:malloc buf error and exit!\n");
        goto end;
    }
    kmsg_buffer_len = klogctl(10,0,0); //获取内核环缓冲区大小
    tbuf = (char *)malloc(kmsg_buffer_len+1);
    if(tbuf == NULL)
    {
        lidbg("record_klogctl:malloc tbuf error and exit!\n");
        goto end;
    }
    /*注册异常信号处理函数*/
    signal(SIGINT,handler);
    signal(SIGKILL,handler);

    signal(SIGUSR1,handler); /*自定义信号1，接收到该信号则将缓冲区的内容写入文件*/
    signal(SIGUSR2,handler);

    /*syslog调用内核日志消息环缓冲区*/
    memset(buf,0,BUFSIZE);

    system("chmod 777 /data/lidbg/reckmsg/* ");
    while(1)
    {
       //kmsg_buffer_len = 0;
       memset(tbuf,0,kmsg_buffer_len+1);

       //kmsg_buffer_len = klogctl(9,tbuf,sizeof(tbuf)); //获取当前缓冲区可读日志字节数
       
       /*klogctl type：4 读取环形日志缓冲区（syslog）最后kmsg_buffer_len字节，并清除缓冲区的内容，dmesg获取不到信息，/proc/kmsg的信息不受影响*/
       if(kmsg_buffer_len > 0 && (count = klogctl(4,tbuf,kmsg_buffer_len)) > 0 )  //目标文件有信息，读取缓冲区内容存储在tbuf，实际读取字节数为count
       {  
           if(debug) lidbg("record_klogctl:buf=%d,count=%d,kmsg_buffer_len=%d\n",strlen(buf),count,kmsg_buffer_len);

           //memset(hbuf,0,sizeof(hbuf));
           //strncpy(hbuf,tbuf,strlen(tbuf));

           //store_error_msg(hbuf); //记录错误信息
  
           if(count >= BUFSIZE) //读满1M，直接写入文件
           {
              lidbg("record_klogctl:tbuf>%dKB,write tbuf into file\n",BUFSIZE/1024);
               //判断是否直接写入当前文件或者进行覆盖操作
              monitor_filesize(); 
               
               if( strlen(buf) > 0) //如果之前buf缓冲区有内容，先写buf
               {
                   buf[strlen(buf)-1]='\0';
                   write(new_fd,buf,strlen(buf));
                   memset(buf,0,BUFSIZE); //清空buf
               }

                write(new_fd,tbuf,count);//将buf缓存写入存储文件
                memset(tbuf,0,kmsg_buffer_len+1);
                system("chmod 777 /data/lidbg/reckmsg/*");
                iswriten = 1;
           }
           else
           {
               if(strlen(buf)+strlen(tbuf) < (BUFSIZE-1))
               {
                    strcat(buf,tbuf);
                    memset(tbuf,0,kmsg_buffer_len+1);
                    iswriten = 0;
                    if(debug) lidbg("record_klogctl:<1M\t");
               }
               else
               {
                    buf[strlen(buf)-1]='\n';
                    lidbg("record_klogctl:read %dKB and write into file\n",BUFSIZE/1024);  
                    
                    //判断是否直接写入当前文件或者进行覆盖操作
                    monitor_filesize(); 
                 
                    write(new_fd,buf,strlen(buf));//将buf缓存写入存储文件
                    memset(buf,0,BUFSIZE);  //清空缓存

                    write(new_fd,tbuf,strlen(tbuf));//将tbuf缓存写入存储文件
                    memset(tbuf,0,kmsg_buffer_len+1);  //清空缓存
                    system("chmod 777 /data/lidbg/reckmsg/* ");
                    iswriten = 1;
                }
   
           }/*end of if(count >= BUFSIZE) else ...*/

        }/*end of if(kmsg_buffer_len > 0)*/

        sleep(1);//休眠3秒后进行下一次读写
    }/*end of while(1)*/

    /*没读满1M情况，上面执行不会写入文件，在退出之前将缓冲区的内容写入文件*/

    if(!iswriten)   
    {
       lidbg("record_klogctl:write before exit\n");

       if(strlen(buf) > 0)
       {
           buf[strlen(buf)-1]='\0';
           write(new_fd,buf,strlen(buf));
       }

       if(strlen(tbuf) >0)
       {
           write(new_fd,tbuf,strlen(tbuf)); 
       }
    }

    /*更新CONFIGFILE文件内容，写入下次存储的文件名和文件数目*/
    refrech_record(filename,filenum);
 
end:
    if(-1 != new_fd) /*退出前关闭文件*/
        close(new_fd);

    if(-1 != emsg_fd)
       close(emsg_fd);

    disposed = 1;

    free(buf);
    free(tbuf);

    return ret;

}/*end of main*/
