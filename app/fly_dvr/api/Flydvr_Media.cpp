
#include "Flydvr_Common.h"
#include "Flydvr_Media.h"
#include "FLY_MSM_OS_Fs.h"
#include "FLY_MSM_OS_Media.h"

SDMMC_STATE m_bSDMMCMountState;

#if (1)

SDMMC_STATE Flydvr_SDMMC_GetMountState(void)
{
    return m_bSDMMCMountState;
}

void Flydvr_SDMMC_SetMountState(SDMMC_STATE val)
{
    m_bSDMMCMountState = val;
}

#endif


FLY_BOOL Flydvr_GetStorageMediaGeometry(UINT8 byMediaID, UINT8* devtype, UINT32* partition_count,UINT32* totalspace)
{
	FlyOS_tagDevinfo_t info;
	INT8 bpVolume[100];
	UINT8 i;
	FLY_MSM_OS_GetStorageMediaGeometry("mmcblk",&info);//mmcblk
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(bpVolume, MMC0_VOLUME_NAME);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(bpVolume, MMC1_VOLUME_NAME);
	        break;
	    case FLYDVR_MEDIA_UDISK://not support temp
	        strcpy(bpVolume, UDISK_VOLUME_NAME);
	        break;
	    default:
	        return FLY_FALSE;
    }

	for(i = 0; i < info.devcount; i++)
	{
		if (strcmp((INT8*)info.dev[i].devname, bpVolume) == 0)
		{
			*devtype = info.dev[i].devtype;
			*partition_count = info.dev[i].partition_count;
			*totalspace = info.dev[i].dev_totalspace/1024;
			//lidbg("%s: ======%s found:[%d,%d,%d]======\n", __func__,info.dev[i].devname,*devtype,*partition_count,*totalspace);
			return FLY_TRUE;
		}
	}
	
	return FLY_FALSE;
}

FLY_BOOL Flydvr_GetPathFreeSpace(UINT8 byMediaID, UINT32* freeSpace)
{
	INT8 storagePath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(storagePath, MMC0_STORAGE_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(storagePath, MMC1_STORAGE_PATH);
	        break;
	    case FLYDVR_MEDIA_UDISK://not support temp
	        strcpy(storagePath, UDISK_STORAGE_PATH);
	        break;
	    default:
	        return FLY_FALSE;
    }
	*freeSpace = FLY_MSM_OS_GetPathFreeSpace(storagePath);
	return FLY_TRUE;
}

#if 0
FLY_BOOL Flydvr_GetTotalFileSize(UINT8 byMediaID, UINT32* fileSize)
{
	INT8 VRPath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(VRPath, MMC0_VR_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(VRPath, MMC1_VR_PATH);
	        break;
	    default:
	        return FLY_FALSE;
    }
	*freeSpace = FLY_MSM_OS_GetPathFreeSpace(storagePath);
	return FLY_TRUE;
}
#endif

FLY_BOOL Flydvr_GetVRFileInfo(UINT8 byMediaID ,INT8* minVRFileName,UINT32* filecnt)
{
	INT8 VRPath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(VRPath, MMC0_VR_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(VRPath, MMC1_VR_PATH);
	        break;
	    default:
	        return FLY_FALSE;
    }
	FLY_MSM_OS_GetVRFileInfo(VRPath, minVRFileName, filecnt);
	return FLY_TRUE;
}

FLY_BOOL Flydvr_SetFirstDelProtectFile(INT8* protectVRFileName)
{
	FLY_MSM_OS_SetFirstDelProtectFile(protectVRFileName);
	return FLY_TRUE;
}

FLY_BOOL Flydvr_SetSecondDelProtectFile(INT8* protectVRFileName)
{
	FLY_MSM_OS_SetSecondDelProtectFile(protectVRFileName);
	return FLY_TRUE;
}

FLY_BOOL Flydvr_DelDaysFile(UINT8 byMediaID ,UINT32 days)
{
	INT8 VRPath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(VRPath, MMC0_VR_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(VRPath, MMC1_VR_PATH);
	        break;
	    default:
	        return FLY_FALSE;
    }
	FLY_MSM_OS_DelDaysFile(VRPath, days);
	return FLY_TRUE;
}

FLY_BOOL Flydvr_DelLostDir(UINT8 byMediaID)
{
	INT8 storagePath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(storagePath, MMC0_STORAGE_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(storagePath, MMC1_STORAGE_PATH);
	        break;
	    default:
	        return FLY_FALSE;
    }
	FLY_MSM_OS_DelLostDir(storagePath);
	return FLY_TRUE;
}

FLY_BOOL Flydvr_CheckVRPath(UINT8 byMediaID)
{
	bool ret;
	INT8 VRPath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(VRPath, MMC0_VR_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(VRPath, MMC1_VR_PATH);
	        break;
	    default:
			return FLY_FALSE;
	}

	ret = FLY_MSM_OS_IsDirExist(VRPath);
	if(ret == true) 
		return FLY_TRUE; 
	return FLY_FALSE;
}

FLY_BOOL Flydvr_MkVRPath(UINT8 byMediaID)
{
	bool ret;
	INT8 VRPath[255];
	switch(byMediaID) {
	    case FLYDVR_MEDIA_MMC0:
			strcpy(VRPath, MMC0_VR_PATH);
	        break;
	    case FLYDVR_MEDIA_MMC1:
	        strcpy(VRPath, MMC1_VR_PATH);
	        break;
	    default:
			return FLY_FALSE;
	}

	ret = FLY_MSM_OS_MkDir(VRPath);
	if(ret == true) 
		return FLY_TRUE; 
	return FLY_FALSE;
}

FLY_BOOL Flydvr_CheckOnlineVRPath()
{
	bool ret;
	INT8 VRPath[255];
	strcpy(VRPath, MMC0_ONLINE_VR_PATH);

	ret = FLY_MSM_OS_IsDirExist(VRPath);
	if(ret == true) 
		return FLY_TRUE; 
	return FLY_FALSE;
}


FLY_BOOL Flydvr_MkOnlineVRPath()
{
	bool ret;
	INT8 VRPath[255];
	strcpy(VRPath, MMC0_ONLINE_VR_PATH);

	ret = FLY_MSM_OS_MkDir(VRPath);
	if(ret == true) 
		return FLY_TRUE; 
	return FLY_FALSE;
}


