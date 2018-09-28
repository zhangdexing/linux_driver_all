#define FLY_MISC_COMMAND_PAGE 2

//typedef unsigned int uint32;
typedef  int int32;
typedef unsigned short uint16;
typedef  short int16;
typedef unsigned char uint8;
typedef  char int8;
typedef unsigned char BYTE;

typedef  unsigned int UINT;


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
    uint8 ver[4];
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
    uint8 version[4];
    unsigned int  offset;
} recovery_header_t;

//----------------------------------------------

typedef struct qcn_header
{
    char flags[16];
    UINT data_size;
    uint8 version[4];
    UINT offset;
} qcn_header_t;

typedef struct bp_header
{
    char flags[16];
    UINT data_size;
    uint8 version[4];
    UINT offset;
} bp_header_t;

typedef struct bp_time
{
    char flags[16];
    uint8 version[4];
    UINT subVer;
} bp_time_t;

typedef struct recovery_language
{
    char flags[16];
    UINT language;
} recovery_language_t;

typedef struct lk_header
{
    char flags[16];
    UINT data_size;
    uint8 version[4];
    UINT offset;
} lk_header_t;

typedef struct mbr_header
{
    char flags[16];
    UINT data_size;
    uint8 version[4];
    UINT offset;
} mbr_header_t;

typedef struct osd_header
{
    char flags[16];
    UINT data_size;
    BYTE version[4];
    UINT offset;
} osd_header_t;
typedef struct _FLY_RESTORE_DATA
{
    char flags[16];
    uint8 iblock30FsTyepe;
    int32 bCheckBlock;
    uint8 save[512];

} FLY_RESTORE_DATA;

typedef struct hw_recinfo
{
    int bValid;
    char info[32];
} hw_info_t;

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
    //-----------------------------------------
    hw_info_t hwInfo;
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

