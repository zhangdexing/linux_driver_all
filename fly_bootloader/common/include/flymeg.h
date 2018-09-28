#ifndef __FLYCOMMON_H__
#define __FLYCOMMON_H__

#define FLY_MISC_COMMAND_PAGE 2

//typedef unsigned int u_int32_;

typedef unsigned char _BYTE_;
typedef  int _int32_;
typedef unsigned short _uint16_;
typedef  short _int16_;
typedef unsigned char _uint8_;
typedef  char _int8_;

typedef  unsigned int _UINT_;


typedef struct paramItem
{
    char flags[16];
    unsigned int val;
} paramItem_t;

typedef struct bootParams
{
    paramItem_t bootParamsLen;
    paramItem_t autoUp;
    paramItem_t upName;
} bootParams_t;

typedef struct verStatus
{
    char name[12];
    _uint8_ ver[4];
    unsigned int  subVer;
} verStatus_t;

typedef struct verParams
{
    verStatus_t start;
    verStatus_t uiconfig;
    verStatus_t lpc;
    verStatus_t halConfig;
    verStatus_t androidOTA;
    verStatus_t panelSteer;
    verStatus_t moduleconfig;
} verParams_t;

typedef struct recovery_header
{
    char flags[16];
    unsigned int  data_size;
    _uint8_ version[4];
    unsigned int  offset;
} recovery_header_t;

//----------------------------------------------

typedef struct qcn_header
{
    char flags[16];
    _UINT_ data_size;
    _uint8_ version[4];
    _UINT_ offset;
} qcn_header_t;

typedef struct bp_header
{
    char flags[16];
    _UINT_ data_size;
    _uint8_ version[4];
    _UINT_ offset;
} bp_header_t;

typedef struct bp_time
{
    char flags[16];
    _uint8_ version[4];
    _UINT_ subVer;
} bp_time_t;

typedef struct recovery_language
{
    char flags[16];
    _UINT_ language;
} recovery_language_t;

typedef struct lk_header
{
    char flags[16];
    _UINT_ data_size;
    _uint8_ version[4];
    _UINT_ offset;
} lk_header_t;

typedef struct mbr_header
{
    char flags[16];
    _UINT_ data_size;
    _uint8_ version[4];
    _UINT_ offset;
} mbr_header_t;


typedef struct _FLY_RESTORE_DATA
{
    char flags[16];
    _uint8_ iblock30FsTyepe;
    _int32_ bCheckBlock;
    _uint8_ save[512];

} FLY_RESTORE_DATA;

typedef struct hw_info
{
    _int32_ bValid;
    char info[32];
} hw_info_t;

typedef struct osd_header
{
    char flags[16];
    _UINT_ data_size;
    _BYTE_ version[4];
    _UINT_ offset;
} osd_header_t;
//-----------------------------------------

typedef struct recovery_meg
{
    bootParams_t bootParam;
    verParams_t verParam;
    recovery_header_t recovery_headers;


    //-----------------------------------------
    lk_header_t lkHeader;
    bp_header_t bpHeader;
    qcn_header_t qcnHeader;
    bp_time_t bpTimes;
    recovery_language_t recoveryLanguage;
    mbr_header_t mbrHeader;
    osd_header_t osdHeader;
    FLY_RESTORE_DATA flyRestoreData;
    hw_info_t hwInfo;
    //-----------------------------------------
} recovery_meg_t;


/**test emmc **/

typedef  struct flywrdata
{
    char flag[16];
    char data[32];
    int size;
} flywrdata_t;


void fly_erase( char *parname);
int emmc_get_extra_recovery_msg(char *ptn_name, unsigned  long len, unsigned char *in);
int emmc_get_extra_recovery_msg2(char *ptn_name, unsigned  long len, unsigned char *in);

//#define WITH_DEBUG_GLOBAL_RAM 1

#endif

