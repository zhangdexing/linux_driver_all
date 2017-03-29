typedef enum _FLYDVR_MEDIA_ID {
    FLYDVR_MEDIA_MMC0 = 0,
    FLYDVR_MEDIA_MMC1,
    FLYDVR_MEDIA_UDISK,
    FLYDVR_MEDIA_MAX_ID
} FLYDVR_MEDIA_ID;

typedef enum{
    SDMMC_OUT = 0,
    SDMMC_IN
}SDMMC_STATE;

#define MMC1_REVERSE_SIZE		    		1024

#if 0
#define MMC0_VOLUME_NAME	"mmcblk0"
#define MMC1_VOLUME_NAME	"mmcblk1"
#define UDISK_VOLUME_NAME	"sda"

#define MMC0_STORAGE_PATH 	"/storage/sdcard0"
#define MMC1_STORAGE_PATH 	"/storage/sdcard1"
#define UDISK_STORAGE_PATH 	"/storage/udisk"

#define MMC0_VR_PATH 	"/storage/sdcard0/camera_rec"
#define MMC1_VR_PATH 	"/storage/sdcard1/camera_rec"
#else
#ifdef PLATFORM_msm8996
#define MMC0_VOLUME_NAME	"sda"
#define MMC1_VOLUME_NAME	"mmcblk0"
#define UDISK_VOLUME_NAME	"sdf"

#define MMC0_STORAGE_PATH 	"/storage/emulated/0/"
#define MMC1_STORAGE_PATH 	"/storage/sdcard1"
#define UDISK_STORAGE_PATH 	"/storage/udisk"

#define MMC0_VR_PATH 	"/storage/emulated/0/camera_rec"
#define MMC1_VR_PATH 	"/storage/sdcard1/camera_rec"
#define MMC0_ONLINE_VR_PATH 	"/storage/emulated/0/preview_cache"
#else
#define MMC0_VOLUME_NAME	"mmcblk0"
#define MMC1_VOLUME_NAME	"mmcblk1"
#define UDISK_VOLUME_NAME	"sdb"

#define MMC0_STORAGE_PATH 	"/storage/emulated/0/"
#define MMC1_STORAGE_PATH 	"/storage/sdcard1"
#define UDISK_STORAGE_PATH 	"/storage/udisk"

#define MMC0_VR_PATH 	"/storage/sdcard0/camera_rec"
#define MMC1_VR_PATH 	"/storage/sdcard1/camera_rec"
#define MMC0_ONLINE_VR_PATH 	"/dev/preview_cache"
#endif


#endif

#define PRINT_STORAGE_FILESPACE_USAGE		system("du -ah "MMC1_STORAGE_PATH" >> "LOG_PATH)

FLY_BOOL Flydvr_GetStorageMediaGeometry(UINT8 byMediaID, UINT8* devtype, UINT32* partition_count,UINT32* totalspace);
FLY_BOOL Flydvr_GetPathFreeSpace(UINT8 byMediaID, UINT32* freeSpace);
FLY_BOOL Flydvr_GetVRFileInfo(UINT8 byMediaID ,INT8* minVRFileName,UINT32* filecnt);
FLY_BOOL Flydvr_DelDaysFile(UINT8 byMediaID ,UINT32 days);
SDMMC_STATE Flydvr_SDMMC_GetMountState(void);
void Flydvr_SDMMC_SetMountState(SDMMC_STATE val);
FLY_BOOL Flydvr_Get_IsSDMMCFull(void);
void Flydvr_Set_IsSDMMCFull(FLY_BOOL val);
FLY_BOOL Flydvr_CheckVRPath(UINT8 byMediaID);
FLY_BOOL Flydvr_MkVRPath(UINT8 byMediaID);
FLY_BOOL Flydvr_CheckOnlineVRPath();
FLY_BOOL Flydvr_MkOnlineVRPath();
FLY_BOOL Flydvr_SetFirstDelProtectFile(INT8* protectVRFileName);
FLY_BOOL Flydvr_SetSecondDelProtectFile(INT8* protectVRFileName);
FLY_BOOL Flydvr_DelLostDir(UINT8 byMediaID);