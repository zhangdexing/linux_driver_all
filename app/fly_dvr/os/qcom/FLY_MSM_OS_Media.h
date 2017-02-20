typedef enum _FLYOS_DEVTYPE {
    FLYOS_DEVTYPE_NOT_REMOVEABLE = 0,
    FLYOS_DEVTYPE_REMOVEABLE,
    FLYOS_DEVTYPE_MAX_ID
} FLYOS_DEVTYPE;

typedef struct FlyOS_tagDevParam_T
{
    unsigned char  devname[12];
    unsigned char  devtype;
    unsigned char  partition_count;
    unsigned int dev_totalspace;//KB
    unsigned int partition_size[50];
} FlyOS_tagDevParam_t, *pFlyOS_tagDevParam_t;

typedef struct FlyOS_tagDevinfo_T
{
    FlyOS_tagDevParam_t dev[8];
    int     devcount; 
    int     disk_num;
} FlyOS_tagDevinfo_t, *pFlyOS_tagDevinfo_t;

int FLY_MSM_OS_GetStorageMediaGeometry(const char* partition_head,FlyOS_tagDevinfo_t* info);
int FLY_MSM_OS_GetPathFreeSpace(char* path);
int FLY_MSM_OS_GetVRFileInfo(char* Dir,char* minRecName, unsigned int* filecnt);
int FLY_MSM_OS_DelDaysFile(char* Dir,int days);
int FLY_MSM_OS_SetFirstDelProtectFile(char* protectVRFileName);
int FLY_MSM_OS_SetSecondDelProtectFile(char* protectVRFileName);
bool FLY_MSM_OS_DelLostDir(char* mediaPath);