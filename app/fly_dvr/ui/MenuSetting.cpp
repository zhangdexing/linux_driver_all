#include "Flydvr_Common.h"
#include "MenuSetting.h"
#include <stdio.h>
#include <stdlib.h>

static MenuInfo gDefaultMenuInfo;
static MenuInfo gCurMenuInfo = {0};

#define PROP_HEAD	"persist.flydvr."

#define	NUMBER_SETTING		(100)
#define	FOURCC(a, b, c, d)	((int)a + ((int)b << 8) + ((int)c << 16) + ((int)d << 24))
#define	MAGIC_SETTING		FOURCC('a', 'i', 't', 'm')
#define	VERSION_SETTING		0x0100

typedef struct _setting_atom {
    char*		szSKey;
    int			nSVal;
    int			nMin;
    int			nMax;
    const char*	szSRem;
} SETTING_ATOM;

typedef struct {
	int				idSet;
	int				vrSet;
	int				nuSet;
	SETTING_ATOM	sAtom[NUMBER_SETTING];
} MENU_ATOMS;

static MENU_ATOMS  menu_atoms =
{
    MAGIC_SETTING,
    VERSION_SETTING,
    NUMBER_SETTING,
    {
        // Still
        {/* 000 */"EMSaveDays", 3,    1, EMERGENCY_SAVE_NUM - 1,   "0~Save days of Emergency Files  - 14"},
        {/* 001 */"EMSwitch",1,     0, EMERGENCY_NUM - 1,    "0:Emergency Disable, 1:Emergency Enable"},
        {/* 003 */"VRMode", 0,    0, RECORD_MODE_NUM - 1,   "0:Single VR, 1:Dual VR"},
        {/* 004 */"VRSwitch",0,     0, RECORD_NUM - 1,    "0:Start VR, 1:Stop VR"},
        {/* 005 */"VRFileTime", 3,    1, SINGLEFILE_RECORD_TIME_NUM - 1,   "0~time of VRFile(mins) - 10"},
        /* END OF MARK */
        {NULL,                            0,           0,              0,                            NULL}
    }
};


pMenuInfo MenuSettingConfig(void)
{
    return &gCurMenuInfo;
}


void CurrentMenuClear(void)
{
	memset(&gCurMenuInfo,0,sizeof(MenuInfo));
}

static void MenuSettingInRange(UINT8 *cur, UINT8 low, UINT8 high)
{
	*cur = (*cur>=high)?(high):( (*cur<=low)?(low):(*cur) );
}


void ListAllMenuSetting(MenuInfo *Info)
{
    lidbg("---------------------------------------------------\n");
    lidbg(" uiEmergencySaveDays          = %d \n", Info->uiEmergencySaveDays          );
    lidbg(" uiEmergencySwitch       = %d \n", Info->uiEmergencySwitch       );
    lidbg(" uiRecordMode     = %d \n", Info->uiRecordMode     );
    lidbg(" uiRecordSwitch        = %d \n", Info->uiRecordSwitch        );
    lidbg(" uiSingleFileRecordTime       = %d \n", Info->uiSingleFileRecordTime       );
    lidbg("---------------------------------------------------\n");
}

FLY_BOOL CheckMenuSetting(MenuInfo* CurMenu)
{
	if(!CurMenu)
		return FLY_FALSE;

	//Still
	MenuSettingInRange(&(CurMenu->uiEmergencySaveDays), 			1, 	EMERGENCY_SAVE_NUM-1		);
	MenuSettingInRange(&(CurMenu->uiEmergencySwitch), 		0, 	EMERGENCY_NUM-1			);
	MenuSettingInRange(&(CurMenu->uiRecordMode), 		0, 	RECORD_MODE_NUM-1	);
	MenuSettingInRange(&(CurMenu->uiRecordSwitch), 		0, 	RECORD_NUM-1				);
	MenuSettingInRange(&(CurMenu->uiSingleFileRecordTime), 		1, 	SINGLEFILE_RECORD_TIME_NUM-1		);

	return FLY_TRUE;
}

FLY_BOOL ImportMenuInfo(MenuInfo* pmi)
{
	char	*p;
	int		i;
	
	if(!pmi)
		return FLY_FALSE;

	p = (char*)pmi;

	for (i = 0; i < sizeof(MenuInfo); i++) {
		menu_atoms.sAtom[i].nSVal = *(p + i);
	}

	return FLY_TRUE;
}

FLY_BOOL ExportMenuProp(char *prop_head)
{
	int		i;
	char prop_name[255];
	char num_str[255];
	for (i = 0; i < sizeof(MenuInfo); i++) {
		sprintf(prop_name, "%s%s",prop_head,menu_atoms.sAtom[i].szSKey);
		sprintf(num_str, "%d",menu_atoms.sAtom[i].nSVal);
		property_set(prop_name, num_str);
	}
	
	return FLY_TRUE;
}

/* Read Text File Setting to MENU_ATOMS and MenuInfo */
FLY_BOOL ParseMenuSetting(char *prop_head, MenuInfo* pmi)
{
	char	*p;
	int		i;
	char prop_name[255];
	char num_str[255];
	char default_str[255];
	p = (char*)pmi;
	
	for (i = 0; i < sizeof(MenuInfo); i++) {
		sprintf(prop_name, "%s%s",prop_head,menu_atoms.sAtom[i].szSKey);
		sprintf(default_str, "%d",menu_atoms.sAtom[i].nSVal);
		//property_set(prop_name, num_str);
		property_get(prop_name, num_str, default_str);
		lidbg("%s:%s(def:%s)\n",prop_name,num_str,default_str);
		menu_atoms.sAtom[i].nSVal = atoi(num_str);
		*(p + i) = menu_atoms.sAtom[i].nSVal;
	}
	return FLY_TRUE;
}