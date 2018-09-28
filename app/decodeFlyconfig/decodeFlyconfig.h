#ifndef _DECODEFLYCONFIG_H_
#define _DECODEFLYCONFIG_H_


typedef unsigned long ULONG;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef int BOOL;

#define FALSE               0
#define TRUE                1


typedef struct TLANGUAGEINFO
{
	char flags[16];
	UINT language;
} LANGUAGEINFO;

typedef struct TENCYPTHEAD
{
	char flags[16];
	UINT data_size;
	BYTE version[4];
	UINT offset;
}ENCYPTHEAD, *PENCYPTHEAD;

typedef struct THWINFO
{
	BOOL bValid;
	char info[32];
} HWINFO;

typedef struct TREPAIREDINFO
{
    char changer[128];
    char changeFrom[48];
    BOOL bValid;
} REPAIREDINFO;

typedef struct TMD5INFO
{
	BYTE md5[16];
} MD5INFO;

typedef struct TOTAINFO
{
	char flag[16];
	BYTE ver[6];	
} OTAINFO;


typedef struct TVERINFO
{
	char name[16];
	BYTE ver[4];
} VERINFO;

typedef struct TBOOTPARAINFO
{
	char initFlag[36];
	UINT updateStage;
	char carType[16];
	UINT debugStatus;
	LANGUAGEINFO languageInfo;
	BYTE reserve[116];
} BOOTPARAINFO, *PBOOTPARAINFO;

typedef struct TOTAPACKINFO
{
	int bValid;
	char Path[128];
} OTAPACKINFO, *POTAPACKINFO;

typedef struct TFLYPARAMETER
{
    BOOTPARAINFO bootPara;
    VERINFO verInfo[30];
    MD5INFO md5Info[30];
	HWINFO hwInfo;
	HWINFO hwInfoBak;
    REPAIREDINFO repairedInfo;
    OTAPACKINFO otaPack;
} FLYPARAMETER, *PFLYPARAMETER;

extern PFLYPARAMETER pflyV2RecoveryPara;
/***********************************************/
typedef struct flyV2bootloader_header
{
	char index[16];
	UINT flybootloader_addr;
	UINT len;
	int boot_parameter[4];
}FLYV2BOOTLOADER_HEADER,*P_FLYV2BOOTLOADER_HEADER;
extern P_FLYV2BOOTLOADER_HEADER pflyV2Bootloader;
/***********************************************/
typedef struct flyBarCodeInfo{
	UINT barcode_len;
	UINT time_len;
	BYTE barcode_data[50];
	BYTE date[14];
}FLYBARCODEINFO;

typedef struct flyV2Product_barcode
{
	BYTE barcode_flag[10];
	UINT effective;
	UINT each_barcode_size;
	UINT all_data_size;
	UINT data_offset;
	FLYBARCODEINFO bar[10];
}FLYV2PRODUCT_BARCODE;

typedef struct flyV2ProductInfo_header
{
	BYTE productFlag[12];
	UINT all_data_size;
	FLYV2PRODUCT_BARCODE product_barcode;
}FLYV2PRODUCTINFO_HEADER,*P_FLYV2PRODUCTINFO_HEADER;
extern P_FLYV2PRODUCTINFO_HEADER pflyV2ProductInfo;

#endif
