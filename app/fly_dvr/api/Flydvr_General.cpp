#include "Flydvr_Common.h"
#include "Flydvr_General.h"
#include "Flydvr_Parameter.h"
#include "Flydvr_Version.h"
#include "Flydvr_Message.h"
#include "Flydvr_Menu.h"
#include "Flydvr_OS.h"
#include "Flydvr_USB.h"
#include "Flydvr_Media.h"
#include "Flydvr_ISPIF.h"
#include "MenuSetting.h"



typedef void (*callback_t)(void);

#define FLYDVR_MSG_QUEUE_SIZE		    	0x80
#define FLYDVR_MSG_CMD_SIZE		    		0x3

static UINT8 m_bFlydvrGeneralInit;

FLY_OS_PIPE m_msg_pipe;
static FLYDVR_QUEUE_MESSAGE            m_MessageQueue[FLYDVR_MSG_QUEUE_SIZE];
static UINT32                    m_MessageQueueIndex_W;
static UINT32                    m_MessageQueueIndex_R;

FLY_OS_PIPE m_msg_lp_pipe;
static FLYDVR_QUEUE_MESSAGE            m_MessageQueueLP[FLYDVR_MSG_QUEUE_SIZE];
static UINT32                    m_MessageQueueLPIndex_W;
static UINT32                    m_MessageQueueLPIndex_R;

pthread_t thread_cb_front_cam_connect_id;
pthread_t thread_cb_front_cam_disconnect_id;
pthread_t thread_cb_rear_cam_connect_id;
pthread_t thread_cb_rear_cam_disconnect_id;
pthread_t thread_cb_media_connect_id;
pthread_t thread_cb_media_disconnect_id;
pthread_t thread_cb_media_full_id;
pthread_t thread_cb_gsensor_crash_id;
pthread_t thread_cb_media_abnormal_full_restore_id;
pthread_t thread_driver_daemon_id;
pthread_t thread_media_daemon_id;

static pthread_cond_t thread_media_daemon_cond;  
static pthread_mutex_t thread_media_daemon_mutex;  

static int flycam_fd = -1;

FLY_BOOL		isFrontCamFound;
FLY_BOOL		isFrontCamRemove;
FLY_BOOL		isRearCamFound;
FLY_BOOL		isRearCamRemove;
FLY_BOOL		isMMC1Conn;
FLY_BOOL		isMMC1DisConn;
FLY_BOOL		isMMC1Full;
FLY_BOOL		isGsensorCrash;
FLY_BOOL		isMediaAbnormalFullRestore;

static FLY_BOOL Flydvr_InitGeneralFunc(void)
{
    memset(m_MessageQueue, 0, sizeof(m_MessageQueue));
    m_MessageQueueIndex_W = 0;
    m_MessageQueueIndex_R = 0;

    return FLY_TRUE;
}

FLY_BOOL Flydvr_InitFlydvrMessage(void)
{
	return Flydvr_OS_CreatePipe(&m_msg_pipe, FLYDVR_MSG_CMD_SIZE);
}

static FLY_BOOL Flydvr_InitGeneralFunc_LP(void)
{
    memset(m_MessageQueueLP, 0, sizeof(m_MessageQueueLP));
    m_MessageQueueLPIndex_W = 0;
    m_MessageQueueLPIndex_R = 0;

    return FLY_TRUE;
}

FLY_BOOL Flydvr_InitFlydvrMessage_LP(void)
{
	return Flydvr_OS_CreatePipe(&m_msg_lp_pipe, FLYDVR_MSG_CMD_SIZE);
}

//------------------------------------------------------------------------------
//  Function    : AHC_SendAHLMessage
//  Description : This function sends an AHL message.
//  Notes       : Message Queue can store (AHC_MSG_QUEUE_SIZE - 1) messages
//------------------------------------------------------------------------------
/**
 @brief This function sends an AHL message.
 Parameters:
 @param[in] ulMsgID : Message ID
 @param[in] ulParam1 : The first parameter. sent with the operation.
 @param[in] ulParam2 : The second parameter sent with the operation.
 @retval TRUE or FALSE. // TRUE: Success, FALSE: Fail
*/
FLY_BOOL Flydvr_SendMessage(UINT32 ulMsgID, UINT32 ulParam1, UINT32 ulParam2)
{
    UINT8 ret;

    static FLY_BOOL bShowDBG = FLY_TRUE;

/*
    if(m_bSendAHLMessage == AHC_FALSE) {
        return AHC_TRUE;
    }

    if(AHC_OS_AcquireSem(m_AHLMessageSemID, 0) != OS_NO_ERR) {
        printc("AHC_SendAHLMessage OSSemPost: Fail!! \r\n");
        return AHC_FALSE;
    }

    if((glAhcParameter.AhlMsgUnblock != glAhcParameter.AhlMsgBlock) && (ulMsgID == glAhcParameter.AhlMsgBlock)) {
        //Message ID blocked !
        AHC_OS_ReleaseSem(m_AHLMessageSemID);
        return AHC_TRUE;
    }
*/
    if(m_MessageQueueIndex_R == (m_MessageQueueIndex_W+ 1)%FLYDVR_MSG_QUEUE_SIZE) {
        //Message Queue Full !
        if(bShowDBG) {
            #if (1)
            UINT8 i;
            #endif

            bShowDBG = FLY_FALSE;
            lidbg("XXX : Message Queue Full ...Fail!! XXX\r\n");

            #if (1)
            lidbg("Dump Message Queue from Index_R to Index_W\r\n");
            for(i=m_MessageQueueIndex_R; i<FLYDVR_MSG_QUEUE_SIZE; i++) {
                lidbg("%3d:: %4d     %4d     %4d\r\n", i, m_MessageQueue[i].ulMsgID, m_MessageQueue[i].ulParam1,m_MessageQueue[i].ulParam2);
            }
            lidbg("------------------------------\r\n");
            for(i=0; i<m_MessageQueueIndex_W; i++) {
                lidbg("%3d:: %4d     %4d     %4dX\r\n", i, m_MessageQueue[i].ulMsgID, m_MessageQueue[i].ulParam1,m_MessageQueue[i].ulParam2);
            }
            #endif
        }
        //AHC_OS_ReleaseSem(m_AHLMessageSemID);
        return FLY_FALSE;
    }

    m_MessageQueue[m_MessageQueueIndex_W].ulMsgID = ulMsgID;
    m_MessageQueue[m_MessageQueueIndex_W].ulParam1 = ulParam1;
    m_MessageQueue[m_MessageQueueIndex_W].ulParam2 = ulParam2;

    //ret = AHC_OS_PutMessage(AHC_MSG_QId, (void *)(&m_MessageQueue[m_MessageQueueIndex_W]));
	ret = Flydvr_OS_PutMessage(&m_msg_pipe, &m_MessageQueue[m_MessageQueueIndex_W]);

    if(ret != 0) {
        lidbg("AHC_OS_PutMessage: ret=%d  Fail!!\r\n", ret);
        //AHC_OS_ReleaseSem(m_AHLMessageSemID);
        return FLY_FALSE;
    }

    bShowDBG = FLY_TRUE;

    m_MessageQueueIndex_W = (m_MessageQueueIndex_W + 1)%FLYDVR_MSG_QUEUE_SIZE;

    //AHC_OS_ReleaseSem(m_AHLMessageSemID);
    return FLY_TRUE;
}


FLY_BOOL Flydvr_GetMessage(UINT32* ulMsgID, UINT32* ulParam1, UINT32* ulParam2)
{
	FLYDVR_QUEUE_MESSAGE q_msg;
	if (Flydvr_OS_GetMessage(&m_msg_pipe, &q_msg, 0x10) != 0)
	{
		*ulMsgID = 0;
        *ulParam1 = 0;
        *ulParam2 = 0;
        return FLY_FALSE;
	}
	else
	{
		*ulMsgID = q_msg.ulMsgID;
	    *ulParam1 = q_msg.ulParam1;
	    *ulParam2 = q_msg.ulParam2;
	}
	//sem up
	 m_MessageQueueIndex_R = (m_MessageQueueIndex_R + 1)%FLYDVR_MSG_QUEUE_SIZE;
	//sem down
	 return FLY_TRUE;
}


FLY_BOOL Flydvr_SendMessage_LP(UINT32 ulMsgID, UINT32 ulParam1, UINT32 ulParam2)
{
    UINT8 ret;

    static FLY_BOOL bShowDBG = FLY_TRUE;

    if(m_MessageQueueLPIndex_R == (m_MessageQueueLPIndex_W+ 1)%FLYDVR_MSG_QUEUE_SIZE) {
        //Message Queue Full !
        if(bShowDBG) {
            #if (1)
            UINT8 i;
            #endif

            bShowDBG = FLY_FALSE;
            lidbg("XXX : Message Queue Full ...Fail!! XXX\r\n");

            #if (1)
            lidbg("Dump Message Queue from Index_R to Index_W\r\n");
            for(i=m_MessageQueueLPIndex_R; i<FLYDVR_MSG_QUEUE_SIZE; i++) {
                lidbg("%3d:: %4d     %4d     %4d\r\n", i, m_MessageQueueLP[i].ulMsgID, m_MessageQueueLP[i].ulParam1,m_MessageQueueLP[i].ulParam2);
            }
            lidbg("------------------------------\r\n");
            for(i=0; i<m_MessageQueueLPIndex_W; i++) {
                lidbg("%3d:: %4d     %4d     %4dX\r\n", i, m_MessageQueueLP[i].ulMsgID, m_MessageQueueLP[i].ulParam1,m_MessageQueueLP[i].ulParam2);
            }
            #endif
        }
        //AHC_OS_ReleaseSem(m_AHLMessageSemID);
        return FLY_FALSE;
    }

    m_MessageQueueLP[m_MessageQueueLPIndex_W].ulMsgID = ulMsgID;
    m_MessageQueueLP[m_MessageQueueLPIndex_W].ulParam1 = ulParam1;
    m_MessageQueueLP[m_MessageQueueLPIndex_W].ulParam2 = ulParam2;

    //ret = AHC_OS_PutMessage(AHC_MSG_QId, (void *)(&m_MessageQueue[m_MessageQueueIndex_W]));
	ret = Flydvr_OS_PutMessage(&m_msg_lp_pipe, &m_MessageQueueLP[m_MessageQueueLPIndex_W]);

    if(ret != 0) {
        lidbg("AHC_OS_PutMessage: ret=%d  Fail!!\r\n", ret);
        //AHC_OS_ReleaseSem(m_AHLMessageSemID);
        return FLY_FALSE;
    }

    bShowDBG = FLY_TRUE;

    m_MessageQueueLPIndex_W = (m_MessageQueueLPIndex_W + 1)%FLYDVR_MSG_QUEUE_SIZE;

    //AHC_OS_ReleaseSem(m_AHLMessageSemID);
    return FLY_TRUE;
}


FLY_BOOL Flydvr_GetMessage_LP(UINT32* ulMsgID, UINT32* ulParam1, UINT32* ulParam2)
{
	FLYDVR_QUEUE_MESSAGE q_msg;
	if (Flydvr_OS_GetMessage(&m_msg_lp_pipe, &q_msg, 0x10) != 0)
	{
		*ulMsgID = 0;
        *ulParam1 = 0;
        *ulParam2 = 0;
        return FLY_FALSE;
	}
	else
	{
		*ulMsgID = q_msg.ulMsgID;
	    *ulParam1 = q_msg.ulParam1;
	    *ulParam2 = q_msg.ulParam2;
	}
	//sem up
	 m_MessageQueueLPIndex_R = (m_MessageQueueLPIndex_R + 1)%FLYDVR_MSG_QUEUE_SIZE;
	//sem down
	 return FLY_TRUE;
}


/***********driver daemon***************/

int Flydvr_SendDriverIoctl(const char *who, char magic , char nr, unsigned long  arg)
{
    int ret = -1;

    if(flycam_fd < 0)
    {
        flycam_fd = open(DRIVER_NODE, O_RDWR);
        if(flycam_fd < 0)
        {
            lidbg("[%s].fail2open[%s].%s\n", __FUNCTION__, DRIVER_NODE, strerror(errno));
            return ret;
        }
    }
    ret = ioctl(flycam_fd, _IO(magic, nr), arg);
    lidbg("[%s].%s.suc.[(%c,%d),ret=%d,errno=%s]\n", who, __FUNCTION__  , magic, nr, ret, strerror(errno));
    return ret;
}

void *thread_driver_daemon(void* data)
{
	struct status_info info,tmp_info;
	FLY_BOOL b_old_frontStatus = FLY_FALSE, b_frontStatus = FLY_FALSE;
	FLY_BOOL b_old_rearStatus = FLY_FALSE, b_rearStatus = FLY_FALSE;
	/*driver args init*/
	tmp_info.emergencySaveDays = MenuSettingConfig()->uiEmergencySaveDays;
	tmp_info.emergencySwitch= MenuSettingConfig()->uiEmergencySwitch;
	tmp_info.recordMode= MenuSettingConfig()->uiRecordMode;
	tmp_info.recordSwitch= MenuSettingConfig()->uiRecordSwitch;
	tmp_info.singleFileRecordTime= MenuSettingConfig()->uiSingleFileRecordTime;
	tmp_info.sensitivityLevel= MenuSettingConfig()->uiGsensorSensitivity;
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_IO, (unsigned long) &tmp_info);
	
	while(1)
	{
		int ret = Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_NOTIFY, (unsigned long) &info);
		switch(ret)
	    {	    
			case MSG_USB_NOTIFY:
				lidbg("%s:===MSG_USB_NOTIFY,%d,%d===\n",__func__,b_old_frontStatus,b_frontStatus);
				b_frontStatus = Flydvr_ISP_IF_LIB_CheckFrontCamExist();
				if(b_old_frontStatus == FLY_FALSE && b_frontStatus == FLY_TRUE)
				{
					lidbg("%s:===MSG_USB_NOTIFY: FRONT_CONN===\n",__func__);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_DVR_SONIX);
					isFrontCamFound = FLY_TRUE;
				}
				else if(b_old_frontStatus == FLY_TRUE && b_frontStatus == FLY_FALSE)
				{
					lidbg("%s:===MSG_USB_NOTIFY: FRONT_DISCONN===\n",__func__);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_DVR_DISCONNECT);
					isFrontCamRemove = FLY_TRUE;
				}
				b_old_frontStatus = b_frontStatus;

				b_rearStatus = Flydvr_ISP_IF_LIB_CheckRearCamExist();
				if(b_old_rearStatus == FLY_FALSE && b_rearStatus == FLY_TRUE)
				{
					lidbg("%s:===MSG_USB_NOTIFY: REAR_CONN===\n",__func__);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_REAR_SONIX);
					isRearCamFound = FLY_TRUE;
				}
				else if(b_old_rearStatus == FLY_TRUE && b_rearStatus == FLY_FALSE)
				{
					lidbg("%s:===MSG_USB_NOTIFY: REAR_DISCONN===\n",__func__);
					Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_REAR_DISCONNECT);
					isRearCamRemove = FLY_TRUE;
				}
				b_old_rearStatus = b_rearStatus;
	        break;

			case MSG_RECORD_SWITCH:
				if(info.recordSwitch == RECORD_START)
				{
					lidbg("%s:===MSG_START_RECORD===[%d]\n",__func__,info.recordSwitch);
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_RECORD_START, 0);
				}
				else if(info.recordSwitch == RECORD_STOP)
				{
					lidbg("%s:===MSG_STOP_RECORD===[%d]\n",__func__,info.recordSwitch);
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_RECORD_STOP, 0);
				}
			break;

			case MSG_RECORD_MODE:
				lidbg("%s:===MSG_RECORD_MODE===[%d]\n",__func__,info.recordMode);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_RECORD_MODE, info.recordMode);
			break;

			case MSG_SINGLE_FILE_RECORD_TIME:
				lidbg("%s:===MSG_SINGLE_FILE_RECORD_TIME===[%d]\n",__func__,info.singleFileRecordTime);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_SINGLE_FILE_TIME, info.singleFileRecordTime);
			break;

			case MSG_EM_SAVE_DAYS:
				lidbg("%s:===MSG_EM_SAVE_DAYS===[%d]\n",__func__,info.emergencySaveDays);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_EMERGENCY_SAVEDAYS, info.emergencySaveDays);
			break;

			case MSG_EM_SWITCH:
				lidbg("%s:===MSG_EM_SWITCH===[%d]\n",__func__,info.emergencySwitch);
				if(info.emergencySwitch == EMERGENCY_START)
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_EMERGENCY_SWITCH_ON, 0);
				else if(info.emergencySwitch == EMERGENCY_STOP)
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_EMERGENCY_SWITCH_OFF, 0);
			break;
			
			case MSG_GSENSOR_NOTIFY:
				lidbg("%s:===MSG_GSENSOR_NOTIFY===\n",__func__);
				isGsensorCrash = FLY_TRUE;
			break;

			case MSG_ACCOFF_NOTIFY:
				lidbg("%s:===MSG_ACCOFF_NOTIFY===\n",__func__);
				StateSwitchMode(UI_ACCOFF_STATE);
			break;

			case MSG_ACCON_NOTIFY:
				lidbg("%s:===MSG_ACCON_NOTIFY===\n",__func__);
				StateSwitchMode(UI_ACCON_STATE);
				StateSwitchMode(UI_VIDEO_STATE);
			break;

			case MSG_START_FORMAT_NOTIFY:
				lidbg("%s:===MSG_START_FORMAT_NOTIFY===\n",__func__);
				StateSwitchMode(UI_SD_FORMAT_STATE);
			break;

			case MSG_STOP_FORMAT_NOTIFY:
				lidbg("%s:===MSG_STOP_FORMAT_NOTIFY===\n",__func__);
				StateSwitchMode(UI_VIDEO_STATE);
			break;

			case MSG_START_ONLINE_VR_NOTIFY:
				lidbg("%s:===MSG_START_ONLINE_VR_NOTIFY===\n",__func__);
				//StateSwitchMode(UI_VIDEO_STATE);
			break;

			case MSG_STOP_ONLINE_VR_NOTIFY:
				lidbg("%s:===MSG_STOP_ONLINE_VR_NOTIFY===\n",__func__);
				//StateSwitchMode(UI_VIDEO_STATE);
			break;

			case MSG_GSENSOR_SENSITIVITY:
				lidbg("%s:===MSG_GSENSOR_SENSITIVITY===[%d]\n",__func__,info.sensitivityLevel);
				Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_KEY_GSENSOR_SENS, info.sensitivityLevel);
#if 0				
				sprintf(cmd, "setprop "GSENSOR_SENSITIVITY_PROP_NAME" %d",info.sensitivityLevel);
				system(cmd);
				MenuSettingConfig()->uiGsensorSensitivity= info.sensitivityLevel;
				uiSaveCurrentConfig();
#endif				
			break;

			case MSG_VR_LOCK:
				lidbg("%s:===MSG_VR_LOCK===[%d]\n",__func__,info.isVRLocked);
				if(info.isVRLocked== true)
					Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_USER_LOCK, 0);
			break;

			case MSG_VOLD_SD_REMOVE:
				lidbg("%s:===MSG_VOLD_SD_REMOVE===\n",__func__);
#if 0				
				Flydvr_SendMessage(FLYM_DRV, EVENT_DRV_VOLD_SD_REMOVED, 0);
#else
				pthread_mutex_lock(&thread_media_daemon_mutex);  
				pthread_cond_signal(&thread_media_daemon_cond);  
				pthread_mutex_unlock(&thread_media_daemon_mutex);  
#endif
			break;
	    }
        lidbg("[%s].call driver_abnormal_cb:ret=%d\n", __FUNCTION__, ret );
		sleep(1);
	}
	return (void*)0;
}


/***********media daemon***************/
void* thread_media_daemon(void* data)
{
	struct timeval now;  
  	struct timespec outtime;  
	UINT8 byMediaID = FLYDVR_MEDIA_MMC1;
	UINT8 devtype;
	UINT32 partition_count;
	UINT32 totalspace;
	UINT32 freeSpace;
	FLY_BOOL lastStatus = FLY_FALSE;
	
	pthread_mutex_lock(&thread_media_daemon_mutex);  
	while(1)
	{
		if(Flydvr_GetStorageMediaGeometry(byMediaID, &devtype, &partition_count, &totalspace) == FLY_TRUE)
		{
			Flydvr_GetPathFreeSpace(byMediaID, &freeSpace);	
			if(freeSpace < MMC1_REVERSE_SIZE) //warning
				lidbg("%s: ======[total:%d, left:%d]======\n", __func__,totalspace,freeSpace);			
			if(Flydvr_CheckVRPath(FLYDVR_MEDIA_MMC1)== FLY_FALSE && freeSpace == 0)
			{
				//lidbg("%s: ======VR Path Not Found!Make New One!======\n", __func__);
			}
			else
			{
				//lidbg("%s: ======OK======\n", __func__,totalspace,freeSpace);	
				/*SD connect/disconnect*/
				if(lastStatus == FLY_FALSE)  isMMC1Conn = FLY_TRUE;
				lastStatus = FLY_TRUE;

				/*SD Full (Wait for delete)*/
				if(freeSpace < MMC1_REVERSE_SIZE)
					isMMC1Full = FLY_TRUE;
				
				/*SD Space Restore*/
				if(freeSpace > MMC1_REVERSE_SIZE + 200)
				{
					if(Flydvr_Get_IsSDMMCFull() == FLY_TRUE)
					{
						isMediaAbnormalFullRestore = FLY_TRUE;
						Flydvr_Set_IsSDMMCFull(FLY_FALSE);
					}
				}

				/*SD Space less than 100MB && still continue video record*/
				if(freeSpace < 100 && Flydvr_Get_IsSDMMCFull() == FLY_FALSE)
				{
					adbg("Space Warning: continue VR [total:%d, left:%d]\n", __func__,totalspace,freeSpace);	
				}
				//sync();
			}
		}
		else 
		{
			if(lastStatus == FLY_TRUE) isMMC1DisConn = FLY_TRUE;
			lastStatus = FLY_FALSE;
		}
#if 0
		sleep(5);
#else
		gettimeofday(&now, NULL);  
	    outtime.tv_sec = now.tv_sec + 5;  
	    outtime.tv_nsec = now.tv_usec * 1000;  
	    pthread_cond_timedwait(&thread_media_daemon_cond, &thread_media_daemon_mutex, &outtime);
#endif
	}
	pthread_mutex_unlock(&thread_media_daemon_mutex);  
	return (void*)0;
}


/***********CB Thread***************/
void *thread_cb_front_cam_connect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isFrontCamFound == FLY_TRUE) 
		{
			cb();
			isFrontCamFound = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_front_cam_disconnect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isFrontCamRemove == FLY_TRUE) 
		{
			cb();
			isFrontCamRemove = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_rear_cam_connect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isRearCamFound == FLY_TRUE) 
		{
			cb();
			isRearCamFound = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_rear_cam_disconnect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isRearCamRemove == FLY_TRUE) 
		{
			cb();
			isRearCamRemove = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_media_connect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isMMC1Conn == FLY_TRUE) 
		{
			cb();
			isMMC1Conn = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_media_disconnect(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isMMC1DisConn == FLY_TRUE) 
		{
			cb();
			isMMC1DisConn = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_media_full(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isMMC1Full == FLY_TRUE) 
		{
			cb();
			isMMC1Full = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_gsensor_crash(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isGsensorCrash== FLY_TRUE) 
		{
			cb();
			isGsensorCrash = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void *thread_cb_media_abnormal_full_restore(void* cb_tmp)
{
	callback_t cb;
	cb = callback_t(cb_tmp);
	while(1)
	{
		if(isMediaAbnormalFullRestore== FLY_TRUE) 
		{
			cb();
			isMediaAbnormalFullRestore = FLY_FALSE;
		}
		else usleep(100*1000);
	}
	return (void*)0;
}

void VRFrontCamConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SetFrontCameraConnect(FLY_TRUE);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_DETECT, 0);
	return;
}

void VRFrontCamDisConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SetFrontCameraConnect(FLY_FALSE);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_FRONT_CAM_REMOVED, 0);
	return;
}

void VRRearCamConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SetRearCameraConnect(FLY_TRUE);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_DETECT, 0);
	return;
}

void VRRearCamDisConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SetRearCameraConnect(FLY_FALSE);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_REAR_CAM_REMOVED, 0);
	return;
}

void VRMediaConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SDMMC_SetMountState(SDMMC_IN);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_CONN_SDCARD, NULL);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_SD_PLUG_IN);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_MMC1_DETECT, 0);
	return;
}

void VRMediaDisConnCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SDMMC_SetMountState(SDMMC_OUT);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DISCONN_SDCARD, NULL);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_SD_PLUG_OUT);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_MMC1_REMOVED, 0);
	return;
}

void VRMediaFullCB(void)
{
    lidbg("%s\n",__func__);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VRCB_MEDIA_FULL, 0);
	return;
}

void VRGsensorCrashCB(void)
{
    lidbg("%s\n",__func__);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_GSENSOR_CRASH, 0);
	return;
}

void VRMediaAbnormalFullRestoreCB(void)
{
    adbg("%s\n",__func__);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_NEW_DVR_ASYN_NOTIFY, RET_SD_NOT_FULL);
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VRCB_MEDIA_ABNORMAL_FULL_RESTORE, 0);
	return;
}

FLY_BOOL Flydvr_RegisterCallback(UINT32 id, callback_t cb)
{
	INT32 ret;
	switch(id)
    {
		case FLY_VIDEO_EVENT_FRONT_CAM_CONNECT:
			//lidbg("%s:=======FLY_VIDEO_FRONT_CAM_CONNECT=======\n",__func__);
			ret=pthread_create(&thread_cb_front_cam_connect_id,NULL,thread_cb_front_cam_connect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_connect: Create pthread error!\n");
			}
       		break;
		case FLY_VIDEO_EVENT_FRONT_CAM_DISCONNECT:
			//lidbg("%s:=======FLY_VIDEO_FRONT_CAM_DISCONNECT=======\n",__func__);
			ret=pthread_create(&thread_cb_front_cam_disconnect_id,NULL,thread_cb_front_cam_disconnect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_disconnect: Create pthread error!\n");
			}
			break;
		case FLY_VIDEO_EVENT_REAR_CAM_CONNECT:
			//lidbg("%s:=======FLY_VIDEO_FRONT_CAM_CONNECT=======\n",__func__);
			ret=pthread_create(&thread_cb_rear_cam_connect_id,NULL,thread_cb_rear_cam_connect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_rear_cam_connect: Create pthread error!\n");
			}
       		break;
		case FLY_VIDEO_EVENT_REAR_CAM_DISCONNECT:
			//lidbg("%s:=======FLY_VIDEO_FRONT_CAM_DISCONNECT=======\n",__func__);
			ret=pthread_create(&thread_cb_rear_cam_disconnect_id,NULL,thread_cb_rear_cam_disconnect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_rear_cam_disconnect: Create pthread error!\n");
			}
			break;
		case FLY_VIDEO_EVENT_MEDIA_CONN:
			ret=pthread_create(&thread_cb_media_connect_id,NULL,thread_cb_media_connect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_disconnect: Create pthread error!\n");
			}
			break;
		case FLY_VIDEO_EVENT_MEDIA_DISCONN:
			ret=pthread_create(&thread_cb_media_disconnect_id,NULL,thread_cb_media_disconnect, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_disconnect: Create pthread error!\n");
			}
       		break;
		case FLY_VIDEO_EVENT_MEDIA_FULL:
			ret=pthread_create(&thread_cb_media_full_id,NULL,thread_cb_media_full, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_disconnect: Create pthread error!\n");
			}
       		break;
		case FLY_VIDEO_EVENT_GSENSOR_CRASH:
			ret=pthread_create(&thread_cb_gsensor_crash_id,NULL,thread_cb_gsensor_crash, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_front_cam_disconnect: Create pthread error!\n");
			}
       		break;
		case FLY_VIDEO_EVENT_MEDIA_ABNORMAL_FULL_RESTORE:
			ret=pthread_create(&thread_cb_media_abnormal_full_restore_id,NULL,thread_cb_media_abnormal_full_restore, (void*)cb);
			if(ret !=0){
				lidbg ("thread_cb_media_abnormal_full_restore: Create pthread error!\n");
			}
			break;
    }
	 return FLY_TRUE;
}

FLY_BOOL Flydvr_RegisterDriverDaemon()
{
	INT32 ret;
	ret=pthread_create(&thread_driver_daemon_id,NULL,thread_driver_daemon, (void*)0);
	if(ret !=0){
		lidbg ("thread_driver_daemon: Create pthread error!\n");
	}
	return FLY_TRUE;
}

FLY_BOOL Flydvr_RegisterMediaDaemon()
{
	INT32 ret;
	pthread_cond_init(&thread_media_daemon_cond, NULL); 
	pthread_mutex_init(&thread_media_daemon_mutex, NULL); 
	ret=pthread_create(&thread_media_daemon_id,NULL,thread_media_daemon, (void*)0);
	if(ret !=0){
		lidbg ("thread_media_daemon: Create pthread error!\n");
	}
	return FLY_TRUE;
}

FLY_BOOL Flydvr_Initialize(void)
{
	if ( !m_bFlydvrGeneralInit )
    {
        Flydvr_PrintBuildTime();
        Flydvr_PrintFwVersion();
        Flydvr_PARAM_Initialize();
        Flydvr_InitGeneralFunc();
        Flydvr_InitFlydvrMessage();
		Flydvr_InitGeneralFunc_LP();
        Flydvr_InitFlydvrMessage_LP();
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_FRONT_CAM_CONNECT,  VRFrontCamConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_FRONT_CAM_DISCONNECT,  VRFrontCamDisConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_REAR_CAM_CONNECT,  VRRearCamConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_REAR_CAM_DISCONNECT,  VRRearCamDisConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_MEDIA_CONN,  VRMediaConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_MEDIA_DISCONN,  VRMediaDisConnCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_MEDIA_FULL,  VRMediaFullCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_GSENSOR_CRASH,  VRGsensorCrashCB);
		Flydvr_RegisterCallback(FLY_VIDEO_EVENT_MEDIA_ABNORMAL_FULL_RESTORE,  VRMediaAbnormalFullRestoreCB);
		Flydvr_SendMessage(FLYM_UI_NOTIFICATION, EVENT_VIDEO_VR_INIT, 0);//Before driver
		Flydvr_RegisterDriverDaemon();
		Flydvr_RegisterMediaDaemon();
        m_bFlydvrGeneralInit = FLY_TRUE;
    }
	return FLY_TRUE;
}

 
