typedef enum {
    RECORD_STOP = 0 ,
    RECORD_START = 1,
    RECORD_NUM
} RECORD_SWITCH_SETTING;

typedef enum {
    EMERGENCY_STOP = 0 ,
    EMERGENCY_START = 1,
    EMERGENCY_NUM
} EMERGENCY_SWITCH_SETTING;

typedef enum {
    EMERGENCY_SAVE_1_DAYS = 1 ,
    EMERGENCY_SAVE_2_DAYS = 2 ,
    EMERGENCY_SAVE_3_DAYS= 3 ,
    EMERGENCY_SAVE_4_DAYS = 4 ,
    EMERGENCY_SAVE_5_DAYS = 5 ,
    EMERGENCY_SAVE_6_DAYS = 6 ,
    EMERGENCY_SAVE_7_DAYS = 7 ,
    EMERGENCY_SAVE_8_DAYS = 8 ,
    EMERGENCY_SAVE_9_DAYS = 9 ,
    EMERGENCY_SAVE_10_DAYS = 10 ,
    EMERGENCY_SAVE_11_DAYS = 11 ,
    EMERGENCY_SAVE_12_DAYS = 12 ,
    EMERGENCY_SAVE_13_DAYS = 13 ,
    EMERGENCY_SAVE_14_DAYS= 14 ,
    EMERGENCY_SAVE_NUM
} EMERGENCY_SAVEDAYS_SETTING;

typedef enum {
    SINGLEFILE_RECORD_TIME_1MINS = 1 ,
    SINGLEFILE_RECORD_TIME_2MINS = 2 ,
    SINGLEFILE_RECORD_TIME_3MINS = 3 ,
    SINGLEFILE_RECORD_TIME_4MINS = 4 ,
    SINGLEFILE_RECORD_TIME_5MINS = 5 ,
    SINGLEFILE_RECORD_TIME_6MINS = 6 ,
    SINGLEFILE_RECORD_TIME_7MINS = 7 ,
    SINGLEFILE_RECORD_TIME_8MINS = 8 ,
    SINGLEFILE_RECORD_TIME_9MINS = 9 ,
    SINGLEFILE_RECORD_TIME_10MINS = 10 ,
    SINGLEFILE_RECORD_TIME_NUM
} RECORD_TIME_SETTING;

typedef enum {
    RECORD_SINGLE_MODE = 0 ,
    RECORD_DUAL_MODE = 1,
    RECORD_MODE_NUM
} RECORD_MODE_SETTING;

typedef struct _MenuInfo
{
	/*current used*/
	UINT8 uiEmergencySaveDays;
	UINT8 uiEmergencySwitch;
	UINT8 uiRecordMode;
	UINT8 uiRecordSwitch;
	UINT8 uiSingleFileRecordTime;
} MenuInfo, *pMenuInfo;

pMenuInfo MenuSettingConfig(void);
FLY_BOOL CheckMenuSetting(MenuInfo* CurMenu);
FLY_BOOL ImportMenuInfo(MenuInfo* pmi);
FLY_BOOL ExportMenuProp(char *prop_head);
FLY_BOOL ParseMenuSetting(char *prop_head, MenuInfo* pmi);
void ListAllMenuSetting(MenuInfo *Info);


