#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <termios.h>
#include <malloc.h>
#include <pthread.h>
#include <poll.h>
#include <dirent.h>
#include <mntent.h>
#include <stdbool.h>
#include <time.h>

#include "decodeFlyconfig.h"

#if defined(ANDROID_PLATFORM)
#include "Zip.h"
#include "DirUtil.h"
#endif

static int is_file_exist(const char *file)
{
    if(access(file, F_OK) == 0)
        return 1;
    else
        return 0;

}

#define LOG_BYTES   (512)

static void printk(const char *fmt, ...)
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
    {
    	printf(buf1);
    	return ;	
    }
    write(fd, buf1, /*sizeof(msg)*/strlen(buf1)/*LOG_BYTES*/);
    close(fd);
    return;
}

#define kmsgLog(fmt,arg...) printk(fmt,##arg)

int my_system(const char * cmd)
{
    FILE * fp;
    int res; char buf[1024];
    if (cmd == NULL)
    {
        kmsgLog("my_system cmd is NULL!\n");
        return -1;
    }
    if ((fp = popen(cmd, "r") ) == NULL)
    {
        perror("popen");
        kmsgLog("popen error: %s/n", strerror(errno)); return -1;
    }
    else
    {
        while(fgets(buf, sizeof(buf), fp))
        {
            kmsgLog("%s", buf);
        }
        if ( (res = pclose(fp)) == -1)
        {
            kmsgLog("close popen file pointer fp error!\n"); return res;
        }
        else if (res == 0)
        {
            return res;
        }
        else
        {
            kmsgLog("popen res is :%d\n", res); return res;
        }
    }
}


void makePath(char *target, const char *str1, const char *str2)
{
	if (strlen(str1) > 0)
	{
		if (strlen(str2) > 0)
		{
			if (str1[strlen(str1)-1] == '/' && str2[0] == '/')
			{
				memcpy(target, str1, strlen(str1) - 1);
				sprintf(target,"%s%s", target, str2);
			}
			else if (str1[strlen(str1)-1] != '/' && str2[0] != '/')
			{
				sprintf(target,"%s/%s", str1, str2);
			}
			else
			{
				sprintf(target,"%s%s", str1, str2);
			}
		}
		else
		{
			sprintf(target, "%s", str1);
		}
	}
	else
	{
		if (strlen(str2) > 0)
		{
			sprintf(target,"%s", str2);
		}
	}

}



int flyV2GetFileIntoBuff(const char* filePath, BYTE **targetBuf, ULONG* len)
{
	int fd;
	struct stat stat;
	BYTE *fileData = NULL;

	fd = open(filePath,O_RDONLY);
	if(fd<0)
	{
		kmsgLog(" can't open -> %s error -> %s\n",filePath, strerror(errno));
		return FALSE;
	}

	if(fstat(fd,&stat))
	{
		kmsgLog("can't stat -> %s\n",filePath);
		return FALSE;
	}

	kmsgLog("%s len -> %ld\n",filePath,stat.st_size);
	fileData = (BYTE *)malloc(stat.st_size);
	if(!fileData)
	{
		kmsgLog("malloc fail!\n");
		close(fd);
		return FALSE;
	}

	if(read(fd,fileData,stat.st_size) < 0)
	{
		kmsgLog("read -> %s fail!\n",filePath);
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







void flyV2FileDecode(UINT encodesize,BYTE *buf)
{
	UINT code1[4],code2[4];
	BYTE *pt;
	UINT m;
	volatile UINT i=0,j;
	UINT BlockNum = (encodesize - 4) / 4;
	UINT leftNum = (encodesize - 4) % 4;

	pt=(BYTE*)buf;
	for(j=0;j<4;j++)
		code1[j]=*(pt+encodesize-4+j);

	while( i < BlockNum)
	{
		for(j=0;j<4;j++)
			code2[j]=*(pt+j);

		for(j=0;j<4;j++)
			*(pt+j)=(code1[j] ^ *(pt+j))+0x10;

		pt += 4;
		i  += 1;
		if( i == BlockNum)
			break;

		m=code2[0]%4;
		switch(m)
		{
		case 0:
			code1[0]=code2[3];
			code1[1]=code2[2];
			code1[2]=code2[1];
			code1[3]=code2[0];
			break;
		case 1:
			code1[0]=code2[1];
			code1[1]=code2[2];
			code1[2]=code2[3];
			code1[3]=code2[0];
			break;
		case 2:
			code1[0]=code2[2];
			code1[1]=code2[3];
			code1[2]=code2[0];
			code1[3]=code2[1];
			break;
		case 3:
			code1[0]=code2[3];
			code1[1]=code2[0];
			code1[2]=code2[1];
			code1[3]=code2[2];
			break;
		default:
			break;
		}
	}

	if(leftNum)
	{
		for(j=0;j<leftNum;j++)
			*(pt+j)=(code2[0] ^ *(pt+j))+0x10;
	}
}



int cmpStr(const char *str1,const char *str2)
{

    if (strlen(str1) == strlen(str2))
    {
        if (memcmp(str1, str2, strlen(str1)) == 0)
        {
            return 0;
        }
    }
    return 1;
}


int verifyPath(char **path)
{
	if (NULL == *path)
	{
		kmsgLog("%s path is NULL\n", __FUNCTION__);
		return FALSE;
	}
	else if ((*path)[0] == '/')
	{
		return TRUE;
	}
	else
	{
		kmsgLog("%s path is %s\n", __FUNCTION__, *path);
		return FALSE;
	}
}


int flyV2CopyFile(const char* destination, unsigned char *data,unsigned int size)
{
	int count,last,x,num;

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	int dest_fd = open(destination, O_CREAT|O_WRONLY|O_TRUNC|O_SYNC, mode);
	if ((dest_fd < 0 ))
	{
		kmsgLog("flyV2Recovery open -> %s fail ! %s\n",destination, strerror(errno));
		return FALSE;
	}

	x=0;
	count = size/size;
	last = size%size;
	while(count>0)
	{
		if(write(dest_fd,data+x*size,size) != size)
		{
			kmsgLog("flyV2Recovery write data fail !\n");
			return FALSE;
		}
		fsync(dest_fd);
		count--;
		x++;
	}
	if(last>0)
	{
		if(write(dest_fd,data+x*BLOCK_SIZE,last) != last)
		{
			kmsgLog("flyV2Recovery write data end fail !\n");
			return FALSE;
		}
		fsync(dest_fd);
	}
	close(dest_fd);

	fsync(dest_fd);

	sync();

	return TRUE;
}

int CopyFile(const char* source,const char* target)
{
	FILE *ps = NULL;
	FILE *pt = NULL;
	unsigned long size,n1,n2,i = 0,j = 0;
	unsigned long copysize,len;
	unsigned char *temp = NULL;
	struct stat buf;


	ps= fopen(source,"rb");
	if(ps == NULL)
	{
		kmsgLog("%s open the %s failed -> %s!!\n", __FUNCTION__,source,strerror(errno));
		return FALSE;
	}
	pt= fopen(target,"ab+");
	if(pt == NULL)
	{
		kmsgLog("%s open the %s failed -> %s!!\n", __FUNCTION__,target,strerror(errno));
		return FALSE;
	}

	if(stat(source,&buf) < 0)
	{
		kmsgLog("%s Can't found '%s'\n", __FUNCTION__,source);
		fclose(ps);
		fclose(pt);
		return FALSE;
	}

	temp = malloc(64);
	fseek(ps,0,SEEK_SET);
	fseek(pt,0,SEEK_SET);

	while(!feof(ps))
	{
		memset(temp,0,64);
		len=fread(temp,1,64,ps);
		fwrite(temp,len,1,pt);
		//num++;
	}

	//temp = malloc(1024);
	//fseek(ps,0,SEEK_SET);
	//fseek(pt,0,SEEK_SET);

	//while(!feof(ps))
	//{
	//	bzero(temp,1024);
	//	len=fread(temp,1,1024,ps);
	//	fwrite(temp,len,1,pt);
	//	//num++;
	//}

	free(temp);
	fclose(ps);
	fclose(pt);

	return TRUE;
}



int flyV2CopyFileInFolder(const char* source,const char* target)
{
	struct stat buf;
	struct dirent* ptr;
	DIR* dir;

	char file_pth[256];
	char file_pth2[256];


	if(stat(source,&buf) < 0)
	{
		kmsgLog("%s failed:Can't found '%s'\n", __FUNCTION__,source);
		return FALSE;
	}

	if(S_ISREG(buf.st_mode))
	{
		if(CopyFile(source,target) == FALSE)
		{
			kmsgLog("%s failed:Can't copy '%s' to '%s'\n", __FUNCTION__,source,target);
			return FALSE;
		}
		return TRUE;
	}

	mkdir(target,0777);
	dir = opendir(source);
	while((ptr = readdir(dir)) != 0)
	{
		if((!memcmp(ptr->d_name,".",1))||(!memcmp(ptr->d_name,"..",2))) continue;

		sprintf(file_pth,"%s/%s",source,ptr->d_name);
		sprintf(file_pth2,"%s/%s",target,ptr->d_name);

		if(FALSE == flyV2CopyFileInFolder(file_pth,file_pth2))
		{
			kmsgLog("%s Can't copy file\n", __FUNCTION__);
			return FALSE;
		}
	}
	closedir(dir);

	return TRUE;
}


int cmdDecode(const char *source, const char *target)
{

	int ret = FALSE;
	BYTE *fileData = NULL;
	ULONG fileLen = 0;
	PENCYPTHEAD ptEncyptHead;

	if (FALSE == verifyPath(&source) || FALSE == verifyPath(&target))
	{
		return FALSE;
	}


	
	ret =  flyV2GetFileIntoBuff(source, &fileData, &fileLen);
	if (FALSE == ret)
	{
		return FALSE;
	}

	flyV2FileDecode(fileLen,fileData);

	ptEncyptHead = (PENCYPTHEAD)fileData;

	kmsgLog("%s update file version -> %d.%d.%d.%d\n", __FUNCTION__,
			ptEncyptHead->version[0],ptEncyptHead->version[1],
			ptEncyptHead->version[2],ptEncyptHead->version[3]);
	kmsgLog("%s file flag %s\n", __FUNCTION__, ptEncyptHead->flags);
	if (cmpStr(ptEncyptHead->flags, "FLYCONFIG") != 0)
	{
		kmsgLog("%s update file verify error!\n", __FUNCTION__);
		free(fileData);
		return FALSE;
	}

	unlink(target);

	if(FALSE == flyV2CopyFile(target, (BYTE *)fileData+sizeof(ENCYPTHEAD), ptEncyptHead->data_size))
	{
		kmsgLog("%s copy file fail !\n", __FUNCTION__);
		free(fileData);
		return FALSE;
	}
	
	
	free(fileData);
	fileData = NULL;
	ptEncyptHead = NULL;
	chmod(target,0777);
	sync();

	return TRUE;
}

#if defined(ANDROID_PLATFORM)

int realUnzip(const char *zipFile, const char *source, const char *target)
{

	int err;
	struct utimbuf timestamp;
	char sysCmd[128];
	ZipArchive zip;

	if (FALSE == verifyPath(&zipFile) || FALSE == verifyPath(&target))
	{
		return FALSE;
	}

	if (NULL == source)
	{
		return FALSE;
	}

	if (cmpStr(source, target) == 0)
	{
		return FALSE;
	}
	
	kmsgLog("%s zipFile -> %s\n", __FUNCTION__, zipFile);


    MemMapping map;
    if (sysMapFile(zipFile, &map) != 0) {
        kmsgLog("failed to map file\n");
        return FALSE;
    }

	kmsgLog("%s open package [%s]\n", __FUNCTION__, zipFile);
	err = mzOpenZipArchive(map.addr, map.length, &zip);
	if (err != 0)
	{
		kmsgLog("%s open zip archive fail !\n", __FUNCTION__);
        sysReleaseMap(&map);
		return FALSE;
	}


	//find filename file
	const ZipEntry* dua_entry = mzFindZipEntry(&zip,source);
	if (dua_entry == NULL)
	{
		kmsgLog("%s can't find -> %s\n", __FUNCTION__,source);
		mzCloseZipArchive(&zip);
        sysReleaseMap(&map);
		return FALSE;
	}



	kmsgLog("%s target -> %s\n", __FUNCTION__, target);
	//memset(sysCmd, 0, sizeof(sysCmd));
	//snprintf(sysCmd, sizeof(sysCmd), "rm -rf %s", target);
	//my_system(sysCmd);
	//sync();

	if (source[strlen(source)-1] != '/')
	{
		int fd = creat(target, 0755);
		if (fd < 0)
		{
			kmsgLog("%s can't mkdir -> %s %s\n", __FUNCTION__,target, strerror(errno));
			mzCloseZipArchive(&zip);
	        sysReleaseMap(&map);
			return FALSE;
		}

		//unzip the file to tmpUpdatePath
		BOOL bUpzipOk = mzExtractZipEntryToFile(&zip, dua_entry, fd);

		close(fd);
		mzCloseZipArchive(&zip);
	    sysReleaseMap(&map);
	    
		if (!bUpzipOk)
		{
			kmsgLog("can't unzip -> %s\n",dua_entry);
			return FALSE;
		}
	}
	else
	{
	

		if(!mzExtractRecursive(&zip, source, target, &timestamp, NULL, NULL, NULL))

		{
			kmsgLog("%s Extract Recursive! fail -> %s !\n", __FUNCTION__, strerror(errno));
			mzCloseZipArchive(&zip);
	        sysReleaseMap(&map);
			return FALSE;
		}

		mzCloseZipArchive(&zip);
	    sysReleaseMap(&map);
	    sync();
    }

	memset(sysCmd, 0, sizeof(sysCmd));
	snprintf(sysCmd, sizeof(sysCmd), "chmod 755 -R %s", target);
	my_system(sysCmd);
	sync();

	sync();
	sync();

	kmsgLog("%s success!\n", __FUNCTION__);
	return TRUE;
	
}
#endif

int cmdUnzip(const char *zipFile, const char *source, const char *target)
{
	
#if defined(ANDROID_PLATFORM)
	return realUnzip(zipFile, source, target);
#else

    int ret1, ret2, ret3, ret4, ret5, ret6;
    char *tmpPath = "/tmp/adf";
    char sysCmd[256]={0};
    
    snprintf(sysCmd, 256, "mkdir -p %s", tmpPath);
    ret1 = my_system(sysCmd);

    snprintf(sysCmd, 256, "unzip -o %s -x %s -d %s > /dev/null", zipFile, source, tmpPath);
    ret2 = my_system(sysCmd);
	
    snprintf(sysCmd, 256, "rm -rf %s", target);
    ret3 = my_system(sysCmd);
    
    snprintf(sysCmd, 256, "cp -rf %s/%s  %s", tmpPath, source, target);
    ret4 = my_system(sysCmd);

    snprintf(sysCmd, 256, "chmod -R 755 %s", target);
    ret5 = my_system(sysCmd);

    
    snprintf(sysCmd, 256, "rm -rf %s", tmpPath);
	ret6 = my_system(sysCmd);
    
    kmsgLog("cmdUnzip success %d %d %d %d %d %d\n", ret1, ret2, ret3, ret4, ret5, ret6);

	return TRUE;
#endif
	
}


static void wait_for_device(const char* fn)//
{
	int tries = 0;
	int ret;
	struct stat buf;
	do
	{
		++tries;
		ret = stat(fn, &buf);
		if (ret)
		{
			sleep(1);
		}
	} while (ret && tries < 10);
	if (ret)
	{
	}
}


int flyV2GetDateFromEMMC(BYTE* outPath,UINT len,const char* device, long offset)//
{

    wait_for_device(device);
    FILE* fd = fopen(device, "rb");
	if (fd == NULL)
	{
        kmsgLog("flyV2Recovery open volume -> %s fail -> %s\n", device, strerror(errno));
		return FALSE;
	}
	fseek(fd, offset, SEEK_SET);

	int count = fread(outPath, len, 1, fd);
	if (count != 1)
	{
        kmsgLog("flyV2Recovery read -> %s fail -> %s\n", device, strerror(errno));
		return FALSE;
    }
	if (fclose(fd) != 0)
	{
        kmsgLog("flyV2Recovery close -> %s fail -> %s\n", device, strerror(errno));
		return FALSE;
	}
    printf("flyV2GetDateFromEMMC lcr12\n");
	return TRUE;
}

char carID[256];
int getCarID()
{
    char *device = "/dev/flyparameter";
    FLYPARAMETER flyV2RecoveryPara;
    memset(&flyV2RecoveryPara,0,sizeof(FLYPARAMETER));
	
	if (FALSE == flyV2GetDateFromEMMC((BYTE *)&flyV2RecoveryPara, sizeof(FLYPARAMETER), device, 0))
	{
		return FALSE;
	}
    kmsgLog("carType %s\n", flyV2RecoveryPara.bootPara.carType);
    memcpy(carID, flyV2RecoveryPara.bootPara.carType, 16);
    kmsgLog("getCarID, end\n");
    return TRUE;
}

ULONG GetTickCount(void)
{
/*
	struct timespec usr_timer;
	clock_gettime(CLOCK_REALTIME,&usr_timer);

	return usr_timer.tv_sec*1000 + usr_timer.tv_nsec / 1000000;
*/

    struct timeval nowTime; 
    gettimeofday(&nowTime, NULL); 
    return nowTime.tv_sec*1000 + nowTime.tv_usec/1000;

}

int main(int argc, char *argv[])
{
	
	int ret = 0;
	char *tmpPath = "/cache/flyconfig.zip";
    char *sourcePath = "/system/vendor/flyaudio/update/flyconfig.cypt";
	char *targetPath="/flyconfig";
	char carModelDir[128];
	char sysCmd[256]={0};
    
    ULONG useTime = GetTickCount();
    
	
	ret = cmdDecode(sourcePath, tmpPath);
	if(TRUE != ret)
	{
		return ret;
	}

	if (2 == argc)
	{
		targetPath = argv[1];
	}
	else if (3 == argc)
	{
		sourcePath = argv[1];
		targetPath = argv[2];
	}

	if (1 == argc)
	{
		my_system("mount -o rw,remount rootfs /");
	}
	
	do
	{
		memset(carModelDir, 0, sizeof(carModelDir));
		memset(carID, 0, sizeof(carID));
		getCarID();
		snprintf(carModelDir, sizeof(carModelDir), "flyconfig/%s/", carID);
	    kmsgLog("carModelDir %s \n", carModelDir);     
		ret = cmdUnzip(tmpPath, carModelDir, targetPath);
		if(TRUE != ret)
		{
			sleep(1);
		}
	}while(TRUE != ret);
		
    snprintf(sysCmd, 256, "rm -rf %s", tmpPath);
    my_system(sysCmd);
	if (1 == argc)
	{
    	my_system("mount -o ro,remount rootfs /");
	}
    kmsgLog("use time %ld\n", GetTickCount() - useTime);    
	return 0;
}


