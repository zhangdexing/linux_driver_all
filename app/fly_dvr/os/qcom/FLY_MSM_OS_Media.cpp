
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include <sys/vfs.h>
#include <dirent.h> 
#include "FLY_MSM_OS_Media.h"
#include "FLY_MSM_OS_Fs.h"
#include "Flydvr_Common.h"

#define DEVTYPE_DISK 2
#define DEVTYPE_U    3

char firstProtectFile[255],secondProtectFile[255];

int FLY_MSM_OS_tokenString(char *buf, char *separator, char **token)
{
    char *token_tmp;
    int pos = 0;
    if(!buf || !separator)
    {
        lidbg("buf||separator NULL?\n");
        return pos;
    }
    while((token_tmp = strsep(&buf, separator)) != NULL )
    {
        *token = token_tmp;
        token++;
        pos++;
    }
    return pos;
}

int FLY_MSM_OS_CheckIsDiskOrUsbDisk(unsigned char *devname)
{
    FILE *fp;
    int  ret = -1;
    char buffer[80], * line = NULL;
    ssize_t size;
    size_t len = 0;
    int fal=0;

    memset(buffer, 0, 80);
    char tmpfilename[64];
	char tmpfilename2[64];
    if(tmpnam(tmpfilename2) == NULL)
    {
        sprintf(tmpfilename, "%s", "/dev/tmp/check_dev_type.txt");
    }
	else
	{
		if(access("/dev/tmp", F_OK) != 0)
			mkdir("/dev/tmp",S_IRWXU|S_IRWXG|S_IRWXO);
		sprintf(tmpfilename, "%s%s", "/dev/",tmpfilename2);
	}

	printf("tmpfilename =%s\n",tmpfilename);
	
    fp = fopen(tmpfilename, "w+");
    sprintf(buffer, "/sys/block/%s/removable", devname);
    FLY_MSM_OS_CpFile(buffer, tmpfilename, fal, NULL);
    size = getline(&line, &len, fp);
    fclose(fp);
    ret = atoi(line);
    switch(ret)
    {
        case 0:
            ret = DEVTYPE_DISK;
            break;
        case 1:
            ret = DEVTYPE_U;
            break;
        default:
            ret = -1;
            break;
    }
    if(line)
    {
        free(line);
    }
    remove(tmpfilename);
    return ret;
}

int FLY_MSM_OS_GetStorageMediaGeometry(const char* partition_head,FlyOS_tagDevinfo_t* info)
{

    int           xx=1,i=0,dev_tpye;
    ssize_t       size;
    size_t        len = 0;
    unsigned char k;
    FlyOS_tagDevinfo_t  devinfo;
    char          tmp_devname[12];
    char          *line = NULL;
    char          *pname;
    char          *token;
    char          seps[] = " ";
    FILE          *fp = NULL;
	//char			storage_name[50] = "mmcblk";
	char			storage_name[50] ;

	strcpy(storage_name,partition_head);
	
    bzero(tmp_devname, 12);
    bzero(&devinfo, sizeof(devinfo));
    FLY_MSM_OS_CpFile("/proc/partitions", "/dev/log/partitions.txt", xx, storage_name);
    
    fp = fopen("/dev/log/partitions.txt", "r");
    
    while((size = getline(&line, &len, fp)) != -1)
    {
        pname = strrchr(line, ' ');
        if(pname == NULL)
            continue;
        pname++;
        if(memcmp(pname, tmp_devname, strlen(storage_name) + 1) == 0) //???
        {
            i = 0;
            token = strtok(line, seps);
            while(token != NULL)
            {
                if(i == 2)
                {
                    k=devinfo.dev[devinfo.devcount-1].partition_count;
                    devinfo.dev[devinfo.devcount-1].partition_size[k]=atoi(token);
                }
                i++;
                token = strtok(NULL, seps);
            }
            devinfo.dev[devinfo.devcount-1].partition_count++;
        }
        else//???
        {
            memcpy(devinfo.dev[devinfo.devcount].devname, pname, strlen(storage_name) + 1);
            i = 0;
            token = strtok(line, seps);
            while(token != NULL)
            {
                if(i == 2)
                {
                    devinfo.dev[devinfo.devcount].dev_totalspace = atoi(token); 
                }
                i++;
                token = strtok(NULL, seps);
            }
            devinfo.devcount++;
        }
        memcpy(tmp_devname, pname, strlen(storage_name) + 1);
    }
#if 0
	printf("dve count =%d\n",devinfo.devcount);
	for(i = 0; i < devinfo.devcount; i++){
			printf("dev name=%s, total space = %d, partcount = %d\n",devinfo.dev[i].devname,devinfo.dev[i].dev_totalspace,devinfo.dev[i].partition_count);
			
				dev_tpye = FLY_MSM_OS_CheckIsDiskOrUsbDisk(devinfo.dev[i].devname);
				devinfo.dev[i].devtype = dev_tpye;
				if(devinfo.dev[i].devtype == DEVTYPE_U)
				{
					printf("this is USB drive\n");
				}
				if(devinfo.dev[i].devtype == DEVTYPE_DISK)
				{
					printf("this is  hard drive\n");
				}
			
	}
#endif
	memcpy(info, &devinfo, sizeof(devinfo));
	fclose(fp);
    return 0;
}

int FLY_MSM_OS_GetPathFreeSpace(char* path)
{
	struct statfs diskInfo;  
	statfs(path, &diskInfo);  
	unsigned long long totalBlocks = diskInfo.f_bsize;  
	unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;  
	size_t mbFreedisk = freeDisk>>20; 
	return mbFreedisk;
}

int FLY_MSM_OS_SetFirstDelProtectFile(char* protectVRFileName)
{
	strcpy(firstProtectFile, protectVRFileName);
	return 0;
}

int FLY_MSM_OS_SetSecondDelProtectFile(char* protectVRFileName)
{
	strcpy(secondProtectFile, protectVRFileName);
	return 0;
}

int FLY_MSM_OS_GetVRFileInfo(char* Dir,char* minRecName, unsigned int* filecnt)
{
	DIR *pDir ;
	struct dirent *ent; 
	int ret;
	char *date_time_key[5] = {NULL};
	char *date_key[3] = {NULL};
	char *time_key[3] = {NULL};
	int cur_date[3] = {0,0,0};
	int cur_time[3] = {0,0,0};
	int min_date[3] = {5000,13,50};
	int min_time[3] = {13,100,100};
	char tmpDName[100] = {0};
	struct tm prevTm,curTm;
	time_t prevtimep = 0,curtimep;
	
	
	/*find the earliest rec file and del*/
	*filecnt = 0;
	pDir=opendir(Dir);  
	if(pDir == NULL) return -1;
	while((ent=readdir(pDir))!=NULL)  
	{  
	        if(!(ent->d_type & DT_DIR))  
	        {  
	                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
	                        continue;  
					//if((!strncmp(ent->d_name, "F", 1) && (cam_id == DVR_ID)) ||(!strncmp(ent->d_name, "R", 1) && (cam_id == REARVIEW_ID)) )
					//{
					if(strncmp(ent->d_name, "F", 1) && strncmp(ent->d_name, "R", 1))
							continue;

					if((strcmp(ent->d_name, firstProtectFile) == 0) || (strcmp(ent->d_name, secondProtectFile) == 0))
					{
						//lidbg("can't track itself!!%s,%s\n",firstProtectFile,secondProtectFile);
						continue;
					}
						
						(*filecnt)++;
		                //lidbg("ent->d_name:%s====ent->d_reclen:%d=====\n", ent->d_name,ent->d_reclen); 

						strcpy(tmpDName, ent->d_name + 1);
						FLY_MSM_OS_tokenString(tmpDName, "__", date_time_key);
						//lidbg("date_time_key0:%s====date_time_key1:%s=====", date_time_key[0],date_time_key[2]);	
						FLY_MSM_OS_tokenString(date_time_key[0], "-", date_key);
						//lidbg("date_key:%s====%s===%s==", date_key[0],date_key[1],date_key[2]);	
						FLY_MSM_OS_tokenString(date_time_key[2], ".", time_key);
						//lidbg("time_key:%s====%s===%s==", time_key[0],time_key[1],time_key[2]);	
						
						curTm.tm_year = atoi(date_key[0]) -1900;
						curTm.tm_mon = atoi(date_key[1]) -1;
						curTm.tm_mday = atoi(date_key[2]);
						curTm.tm_hour = atoi(time_key[0]);
						curTm.tm_min = atoi(time_key[1]);
						curTm.tm_sec	 = atoi(time_key[2]);	
						curtimep = mktime(&curTm);
						
						#if 0
						lidbg("prevtimep=======%d========",  prevtimep);
						lidbg("curtimep=======%d========",  curtimep);
						lidbg("difftime=======%d========", difftime(curtimep, prevtimep));
						#endif
						if((curtimep < prevtimep) || (prevtimep == 0))
						{
							prevtimep = curtimep;
							strcpy(minRecName, ent->d_name);
							//lidbg("minRecName---->%s\n",minRecName);
						}
					//}
	        }  
	}
	closedir(pDir);
	return 0;
}


int FLY_MSM_OS_DelDaysFile(char* Dir,int days)
{
	char filepath[200] = {0};
	DIR *pDir ;
	struct dirent *ent; 
	unsigned char filecnt = 0;
	int ret;
	char *date_time_key[5] = {NULL};
	char *date_key[3] = {NULL};
	char *time_key[3] = {NULL};
	int cur_date[3] = {0,0,0};
	int cur_time[3] = {0,0,0};
	int min_date[3] = {5000,13,50};
	int min_time[3] = {13,100,100};
	char tmpDName[100] = {0};
	struct tm prevTm,curTm;
	time_t prevtimep = 0,curtimep,currentTime;
	int diffval;

	lidbg("%s: del_days_file -> [%d]\n",__func__,days);
	time( &currentTime ); 
	
	pDir=opendir(Dir);  
	if(pDir == NULL) return -1;
	while((ent=readdir(pDir))!=NULL)  
	{  
	        if(!(ent->d_type & DT_DIR))  
	        {  
	                if((strcmp(ent->d_name,".") == 0) || (strcmp(ent->d_name,"..") == 0) || (ent->d_reclen != 48) ) 
	                        continue;  
					if(strncmp(ent->d_name, "E", 1))
							continue;
						
						filecnt++; 
						strcpy(tmpDName, ent->d_name + 2);
						FLY_MSM_OS_tokenString(tmpDName, "__", date_time_key);
						FLY_MSM_OS_tokenString(date_time_key[0], "-", date_key);
						FLY_MSM_OS_tokenString(date_time_key[2], ".", time_key);
					
						curTm.tm_year = atoi(date_key[0]) -1900;
						curTm.tm_mon = atoi(date_key[1]) -1;
						curTm.tm_mday = atoi(date_key[2]);
						curTm.tm_hour = atoi(time_key[0]);
						curTm.tm_min = atoi(time_key[1]);
						curTm.tm_sec	 = atoi(time_key[2]);	
						curtimep = mktime(&curTm);

    					diffval =  (currentTime - curtimep) /(24*60*60);
						//lidbg("====== diff days:%d.======\n",diffval);
						if(diffval > days)
						{
							sprintf(filepath , "%s/%s",Dir,ent->d_name);
							lidbg("====== oldest EM file will be del:%s.======\n",filepath);
							remove(filepath);  
						}
	        }  
	}
	closedir(pDir);
	return filecnt;

}

