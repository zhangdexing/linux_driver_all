#include "Flydvr_Common.h"
#include "Flydvr_General.h"
#include "Flydvr_Parameter.h"
#include "Flydvr_ISPIF.h"


#if 0
//------------------------------------------------------------------------------
//  Function    : AHC_GetHwVersion
//  Description : 
//------------------------------------------------------------------------------
void Flydvr_GetHwVersion(BYTE* pbyChipVersion, BYTE* pbyEcoVersion, BYTE* pbyModel)
{	
	int i;
	char value[PROPERTY_VALUE_MAX];
    property_get("ro.camera.sound.forced", value, "0");
    jboolean canDisableShutterSound = (strncmp(value, "0", 2) == 0);
    
	BYTE* pChipVersionId = (BYTE*)AIT_CHIP_VERSION;
	BYTE* pChipEcoId     = (BYTE*)AIT_CHIP_ECO_VERSION;
	BYTE* pModelId       = (BYTE*)AIT_CHIP_MODEL_VERSION;

	*pbyChipVersion  =  *pChipVersionId;
	*pbyEcoVersion   =  *pChipEcoId;

	for (i = 0; i < CHIP_MODEL_STRING_LENGTH; i++) {
		*pbyModel++  = *pModelId++;
	}
}
#endif
//------------------------------------------------------------------------------
//  Function    : AHC_PrintIspLibVersion
//  Description : 
//------------------------------------------------------------------------------
static void Flydvr_PrintIspLibVersion( void )
{
	Flydvr_ISP_IF_LIB_Init();//tmp
				
	adbg("%s: Front:vid = 0x%.4x, pid = 0x%.4x, Manufacturer = %s, VersionMark= %s\n", __func__, 
				Flydvr_ISP_IF_LIB_GetFrontLibVer().Vid, Flydvr_ISP_IF_LIB_GetFrontLibVer().Pid, 
				Flydvr_ISP_IF_LIB_GetFrontLibVer().szMfg, Flydvr_ISP_IF_LIB_GetFrontLibVer().VerMark);
	adbg("%s: Rear:vid = 0x%.4x, pid = 0x%.4x, Manufacturer = %s, VersionMark= %s\n", __func__, 
				Flydvr_ISP_IF_LIB_GetRearLibVer().Vid, Flydvr_ISP_IF_LIB_GetRearLibVer().Pid, 
				Flydvr_ISP_IF_LIB_GetRearLibVer().szMfg, Flydvr_ISP_IF_LIB_GetRearLibVer().VerMark);
}

static void Flydvr_SendDriverIspLibVersion( void )
{			
	//send_driver_msg(flycam_fd,FLYCAM_STATUS_IOC_MAGIC,NR_DVR_FW_VERSION,(unsigned long)codeVer);
	Flydvr_SendDriverIoctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, NR_DVR_FW_VERSION, (unsigned long) Flydvr_ISP_IF_LIB_GetFrontLibVer().VerMark);
}


//------------------------------------------------------------------------------
//  Function    : Flydvr_PrintBuildTime
//  Description : 
//------------------------------------------------------------------------------
void Flydvr_PrintBuildTime(void)
{
	lidbg("\n");
	adbg("------------------------------------------\n");
    adbg("Build Time - %s  %s  \n", __DATE__,__TIME__); 
}

//------------------------------------------------------------------------------
//  Function    : Flydvr_PrintFwVersion
//  Description : 
//------------------------------------------------------------------------------
void Flydvr_PrintFwVersion(void)
{
    UINT16 ulMajorVersion;
    UINT16 ulMediumVersion;
    UINT16 ulMinorVersion;
    UINT16 ulBranchVersion;
    UINT16 ulTestVersion;
    char*  szReleaseName = 0;
	UINT32 uiBoardtype;
	BYTE   byChipVer;
	BYTE   byEcoVer;
    //BYTE   byModelId[CHIP_MODEL_STRING_LENGTH+1] = {0};

	lidbg("------------------------------------------\n");
#if 0
    Flydvr_GetHwVersion(&byChipVer, &byEcoVer, byModelId);
    lidbg("Model:%s  - HW version: %02X ECO %02X\n", byModelId, byChipVer, byEcoVer);

    Flydvr_GetFwVersion(&ulMajorVersion, &ulMediumVersion, &ulMinorVersion, &ulBranchVersion, &ulTestVersion, &szReleaseName);
    lidbg("FW version : %04X.%04X.%04X\n - BRANCH : %04X  TEST : %04X\n", ulMajorVersion, ulMediumVersion, ulMinorVersion, ulBranchVersion, ulTestVersion);
#endif

	Flydvr_PrintIspLibVersion();
	Flydvr_SendDriverIspLibVersion();
    lidbg("Release: %s\n", szReleaseName);

	//Flydvr_GetParam(PARAM_ID_MAINBOARD_TYPE, &uiBoardtype);
	//lidbg("Board Type: %d\n", uiBoardtype);

	lidbg("------------------------------------------\n");
	sleep(5);//prevent bug
	return;
}

