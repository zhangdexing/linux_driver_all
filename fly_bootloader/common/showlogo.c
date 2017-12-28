#include "fly_platform.h"
#if defined(BOOTLOADER_IMX6Q)
#include "fsl_fastboot.h"
#else
#include "fastboot.h"
#endif

#include "soc.h"
#include "fly_private.h"

//#include "defaultLogo.h"

#define RGB565_To_ARGB8888(x)   \
	((((x) &   0x1F) << 3) |    \
	(((x) &  0x7E0) << 5) |    \
	(((x) & 0xF800) << 8) |    \
	(0xFF << 24)) // opaque


#define LCD_BASE_ADDR  0x20000000 + 0x10000000 - 0x4000000

#ifndef BOOTLOADER_MT3561
extern unsigned page_mask;
extern unsigned page_size;
#else
unsigned page_mask = 0;
//void display_image_on_screen_fly(char *pImageBuffer);

#endif
//#define unsigned long  u32;
#define 	u32		unsigned int
#define  	u16		unsigned short
#define   u8 		unsigned char

typedef struct flybootloader_header
{
    char index[16];
    unsigned int flybootloader_addr;
    unsigned int len;
    int boot_parameter[4];
} flybootloader_header_t;

typedef struct
{
    u16		x_position;
    u16		y_position;
    u16		width;
    u16		height;
    u16		back_color;		//èæ¯è²ïŒRGB565åŒïŒ
    u16		reserved;		//ä¿ç2B
    u32		data_len;		//æ°æ®é¿åºŠ
    u8		*pdata;			//æ°æ®å?
} sLogo, *psLogo;

void set_screen(u16 *LogoBuf, u16 color, u16 width, u16 height)
{
    u16 i, j;
    u16 *FP = (u16 *)(LogoBuf);
    for(i = 0; i < height; i++)
        for(j = 0; j < width; j++)
            *(FP++) = color;
}

void set_logo(unsigned int *LCDframe, u16 *LogoBuf, u16 xpos, u16 ypos, u16 width, u16 height)
{
    u16 i;
    dprintf(INFO, "xpos %d  ypos %d  width %d height %d\r\n", xpos, ypos, width, height);
    for(i = 0; i < height; i++)
    {
        memcpy((unsigned short *)LCDframe + ((i + ypos) * 800 + xpos) , (unsigned short *)LogoBuf + i * width, width * 2);
    }
}

#if 0
void rgb565to888(u8 *p888, u16 *p565, int pexlnum)
{
    int i = 0, j = 0;
    char tem2[10] = {0};
    for( i = 0, j = 0; i < pexlnum; )
    {
        p888 [j + 2]	=	(p565[i] >> 8) & 0x00F8;
        p888 [j + 1]	=	(p565[i] >> 3) & 0x00FC;
        p888 [j]		=	(p565[i] << 3) & 0x00F8;
        j += 3;
        i++;
    }
    return ;
}

int  show_logo()
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned char *plogo = NULL ;
    sLogo *logo_show = NULL;
    int Size_logo;
    u32 s = 0, s1 = 0;
    unsigned n = 0, i = 0, j = 0, k = 0;
    char tem2[20];
    u8 *logo_buff = NULL;
    u8 *temp_buff = NULL;
    unsigned char *tem = malloc(sizeof(flybootloader_header_t) + 1);
    logo_buff = malloc(1024 * 1024);
    temp_buff = malloc(1024 * 1024);
    u8 *pRGB888 = malloc(1024 * 600 * 3);
    flybootloader_header_t  *hdr = tem;
    int index = INVALID_PTN;
    unsigned long long emmc_ptn = 0;

    dprintf(INFO, "[FLYAUDIO]dcz====>>this is show log...\n");
    if(emmc_get_extra_recovery_msg("logo", sizeof(flybootloader_header_t), tem))
    {
        dprintf(INFO, "FAILED: failed to read date for %s \n", "logo");
        free(logo_buff);
        free(temp_buff);
        free(pRGB888);
        return -1;
    }

    dprintf(INFO, "[FLYAUDIO]dcz====>>hdr->index: %s\n", hdr->index);
    if (memcmp(hdr->index, "FLYBOOTLOADER", strlen("FLYBOOTLOADER")))
    {
        dprintf(INFO, "err_out: Invalid image header found !\n");
        free(logo_buff);
        free(temp_buff);
        free(pRGB888);
        return -1;
    }

    hdr->len += sizeof(flybootloader_header_t);
    n = ROUND_TO_PAGE(hdr->len, page_mask);
    n = n / page_size;
    dprintf(INFO, "[FLYAUDIO]dcz====>>hdr->len: %d\n", hdr->len);

    if(emmc_get_extra_recovery_msg("logo", hdr->len, logo_buff))
    {
        dprintf(INFO, "FAILED(2): failed to read date for %s \n", "logo");
        free(logo_buff);
        free(temp_buff);
        free(pRGB888);
        return -1;
    }

    plogo = logo_buff + sizeof(flybootloader_header_t);
    logo_show = (sLogo *)(plogo + 16);
    Size_logo = logo_show->data_len >> 1;
    u16 *Fb = (u16 *)(plogo + 16 + sizeof(sLogo) - 4);
    k = 0;

    for(i = 0; i < Size_logo;)
    {
        s = Fb[i++];
        s1 = s;
        s1 &= 0x7fff;
        if(s > 0x8000)
        {
            for(j = 0; j < s1; j++)
                ((u16 *)temp_buff)[k++] = Fb[i];
            i++;
        }
        else
        {
            for(j = 0; j < s1; j++)
                ((u16 *)temp_buff)[k++] = Fb[i++];
        }
    }

    //logo data rgb565 change rgb888

    rgb565to888(pRGB888, temp_buff, (logo_show->height) * (logo_show->width));
    logo_show->pdata = pRGB888;
    //input logo data to sreen

    display_logo_on_screen(logo_show);
    free(logo_buff);
    free(temp_buff);
    free(pRGB888);
    return;
}
#else

#define TRUE        1
#define FALSE       0

int getEmmcData(char *Partition, unsigned long len, unsigned char *buf)
{
    unsigned n = 0;
    int index = INVALID_PTN;
    unsigned long long emmc_ptn = 0;

    index = partition_get_index(Partition);
    emmc_ptn = partition_get_offset(index);

    if(emmc_ptn == 0)
    {
        dprintf(INFO, "partition logo doesn't exist\n");
        return FALSE;
    }

    n = ROUND_TO_PAGE(len, page_mask);

    dprintf(INFO, "Num of pages is %u \n", n);

    //if (mmc_read(emmc_ptn , (void *)buf, (n*page_size)))
    if (mmc_read(emmc_ptn , (unsigned int *)buf, n))
    {
        dprintf(INFO, "err_out: Cannot read flylogo  logodata:0x%x\n", n);
        return FALSE;
    }

    return TRUE;
}

int rgb565To888(unsigned char *p888, unsigned char *p565, unsigned long pixel)
{
    int ret = FALSE;

    unsigned long i = 0;
    unsigned long j = 0;
    unsigned long k = 0;
    unsigned short pixelRGB565;

    if (NULL == p888 || NULL == p565)
    {
        dprintf(INFO, "\n err_out: rgb565To888 p888 or p565 is NULL");
        goto err_out;
    }

    if (pixel == 0)
    {
        dprintf(INFO, "\n err_out: rgb565To888 pixel == 0");
        goto err_out;
    }

    for (i = 0; i < pixel; i++)
    {
        pixelRGB565 = p565[k + 1];
        pixelRGB565 = (pixelRGB565 << 8) + p565[k];

        p888[j + 2] = (pixelRGB565 >> 8) & 0x00F8;
        p888[j + 1] = (pixelRGB565 >> 3) & 0x00FC;
        p888[j] = (pixelRGB565 << 3) & 0x00F8;

        j = j + 3;
        k = k + 2;
    }

    dprintf(INFO, "rgb565To888 i->%u j->%u k->%u\n", i, j, k);

    ret = TRUE;

err_out:
    return ret;
}

int RGB565_TO_ARGB8888(unsigned int *p8888, unsigned char *p565, unsigned long Pixel)
{
	int ret = FALSE;
	unsigned long i = 0;
	unsigned long k = 0;
	unsigned int pixelRGB565;
	if (NULL == p8888 || NULL == p565)
	{
		dprintf(INFO, "\n err_out: rgb565To888 p8888 or p565 is NULL");
		goto err_out;
	}
	if (Pixel == 0)
	{
		dprintf(INFO, "\n err_out: rgb565ToArgb8888 pixel == 0");
		goto err_out;
	}
	for (i = 0; i < Pixel; i++)
	{
		pixelRGB565 = (((p565[k+1] << 8) | (p565[k] )) & 0xffff);
		p8888[i] = RGB565_To_ARGB8888(pixelRGB565);
		k = k + 2;
	}
	dprintf(INFO, "rgb565ToArgb8888 i->%u k->%u\n", i, k);
	ret = TRUE;
err_out:
	return ret;
}

//��ѹ���� ����RGB565
int logoDataDecompress(
    unsigned long pixelDataStartIndex,
    unsigned long pixelDataEndIndex,
    unsigned char *pLogoFileData,
    unsigned char *pdataBufRBG565,
    unsigned long lenBufRBG565)
{
    int ret = FALSE;

    unsigned long i, j;
    unsigned long bufRBG565Index = 0;
    unsigned short pixelDataMark;
    unsigned long lenForMark;

    if (NULL == pLogoFileData || NULL == pdataBufRBG565)
    {
        dprintf(INFO, "err_out: pLogoFileData or pdataBufRBG565 is NULL");
        goto err_out;
    }

    if (0 == lenBufRBG565)
    {
        dprintf(INFO, "err_out: lenBufRBG565 == 0\n");
    }

    dprintf(INFO, "logoDataDecompress lenBufRBG565 -> %u\n", lenBufRBG565);

    i = pixelDataStartIndex;


    while (i < pixelDataEndIndex)
    {

        pixelDataMark = pLogoFileData[i + 1];
        pixelDataMark = (pixelDataMark << 8) + pLogoFileData[i];

        //lenForMark = pixelDataMark & 0x07FF;
        lenForMark = pixelDataMark & 0x7FFF;

        //dprintf(INFO,"\n i->%u pixelDataMark->%u bufRBG565Index->%u",i,pixelDataMark,bufRBG565Index);

        if ((bufRBG565Index + lenForMark * 2) <= lenBufRBG565)
        {
            if (pixelDataMark > 0x8000)
            {
                for (j = 0; j < lenForMark; j++)
                {
                    pdataBufRBG565[bufRBG565Index++] = pLogoFileData[i + 2];
                    pdataBufRBG565[bufRBG565Index++] = pLogoFileData[i + 3];
                }
                i = i + 4;
            }
            else
            {
                for (j = 0; j < lenForMark; j++)
                {
                    i = i + 2;
                    pdataBufRBG565[bufRBG565Index++] = pLogoFileData[i];
                    pdataBufRBG565[bufRBG565Index++] = pLogoFileData[i + 1];
                }
                i = i + 2;
            }
        }
        else
        {
            break;
        }
    }

    dprintf(INFO, "i -> %u\n", i);
    dprintf(INFO, "bufRBG565Index -> %u\n", bufRBG565Index);
    ret = TRUE;

err_out:
    return ret;
}

int logo_addr_get(unsigned char **partition_add, int partition_add_len,
                  unsigned char **rgb565_add, int rgb565_add_len,
                  unsigned char **rgb888_add, int rgb888_add_len,
                  unsigned int **argb8888_add, int argb8888_add_len)
{
    unsigned n = 0;

    n = ROUND_TO_PAGE(partition_add_len, page_mask);

    *partition_add = (unsigned char *)malloc(n * 4);
    if (NULL == *partition_add)
    {
        printf("Fail: malloc for partition_add !\n");
        goto err_out;
    }
    memset(*partition_add, 0, partition_add_len);

    *rgb565_add = (unsigned char *)malloc(rgb565_add_len);
    if (NULL == *rgb565_add)
    {
        printf("Fail: malloc for rgb565_add !\n");
        goto err_out;
    }

    *rgb888_add = (unsigned char *)malloc(rgb888_add_len);
    if (NULL == *rgb888_add)
    {
        printf("Fail: malloc for rgb888_add !\n");
        goto err_out;
    }

    *argb8888_add = (unsigned char *)malloc(argb8888_add_len);
    if (NULL == *argb8888_add)
    {
        printf("Fail: malloc for argb8888_add !\n");
        goto err_out;
    }

    return 0;

err_out:
    if (NULL != *partition_add)
    {
        free(*partition_add);
        *partition_add = NULL;
    }

    if (NULL != *rgb565_add)
    {
        free(rgb565_add);
        *rgb565_add = NULL;
    }

    if (NULL != *rgb888_add)
    {
        free(*rgb888_add);
        *rgb888_add = NULL;
    }

    if (NULL != *argb8888_add)
    {
        free(*argb8888_add);
        *argb8888_add = NULL;
    }

    return -1;
}

/*****************edit by flychan*********************/
int  show_logo()
{
    int ret = FALSE;

    int alloc_flag = 0;
    sLogo *pLogoInfo;
    sLogo LogoRGB888Info;

    unsigned long fileLen;
    unsigned long lenTotal;
    unsigned long logoPixel;

    unsigned char *tempBuf = NULL;
    unsigned char *pfileBuf = NULL;
    unsigned char *pDataRGB565 = NULL;
    unsigned char *pDataRGB888 = NULL;
    unsigned char *pDataARGB8888 = NULL;
    unsigned char *pPartitionData = NULL;

    unsigned long pixelDataStartIndex;
    unsigned long pixelDataEndIndex;

    flybootloader_header_t *pflyBootloaderInfo;

    dprintf(INFO, "***** Starting show logo *****\n");

    tempBuf = (unsigned char *)malloc(4096);
    if(NULL == tempBuf)
    {
        dprintf(INFO, "Fail: malloc for tempBuf !\n");
        goto err_out;
    }
    pflyBootloaderInfo = (flybootloader_header_t *)tempBuf;

    if (ptn_read("logo", 0, 4096, tempBuf))
    {
        dprintf(INFO, "err_out: Cannot read flylogo  logodata:0x%x\n");
        goto err_out;
    }

    dprintf(INFO, "pflyBootloaderInfo->index: %s\n", pflyBootloaderInfo->index);
    if (memcmp(pflyBootloaderInfo->index, "FLYBOOTLOADER", strlen("FLYBOOTLOADER")))
    {
        dprintf(INFO, "err_out: Invalid image header found !\n");
        goto err_out;
    }

    pfileBuf = tempBuf + sizeof(flybootloader_header_t);

    if ((0 != strncmp(pfileBuf, "FLYAUDIOLOGOFLY", strlen("FLYAUDIOLOGOFLY")))
            //	||(0 != strncmp((pfileBuf+fileLen-strlen("FLYAUDIO")),"FLYAUDIO",strlen("FLYAUDIO")))
       )
    {
        dprintf(INFO, "\n fail: check logo flag!");
        goto err_out;
    }

    pLogoInfo = (sLogo *)(pfileBuf + sizeof("FLYAUDIOLOGOFLY"));
    dprintf(INFO, "logo info:\n");
    dprintf(INFO, "x_position -> %u\n", pLogoInfo->x_position);
    dprintf(INFO, "y_position -> %u\n", pLogoInfo->y_position);
    dprintf(INFO, "width -> %u\n", pLogoInfo->width);
    dprintf(INFO, "height -> %u\n", pLogoInfo->height);
    dprintf(INFO, "back_color -> %u\n", pLogoInfo->back_color);
    dprintf(INFO, "reserved -> %u\n", pLogoInfo->reserved);
    dprintf(INFO, "data_len -> %u\n", pLogoInfo->data_len);
    dprintf(INFO, "display reverse bit -> %d\n", RecoveryMeg.hwInfo.info[12] - '0');

    logoPixel = pLogoInfo->width * pLogoInfo->height;
    if (0 == logoPixel)
    {
        dprintf(INFO, "fail: logoPixel == 0\n");
        goto err_out;
    }
    dprintf(INFO, "logo pixel -> %u\n", logoPixel);

    fileLen = pflyBootloaderInfo->len;

    lenTotal = pflyBootloaderInfo->len + sizeof(flybootloader_header_t);

    dprintf(INFO, "fileLen -> %ld\n", fileLen);
    dprintf(INFO, "lenTotal: %u\n", lenTotal);

#ifdef LOGO_ADD_ALLOCED
    pPartitionData = LOGO_MALLOCED_ADDR;
    pDataRGB565 = pPartitionData + RGB565_DATA_LEN;
    pDataRGB888 = pDataRGB565 + RGB888_DATA_LEN;
#else
    ret = logo_addr_get(&pPartitionData, lenTotal, &pDataRGB565, logoPixel * 2, &pDataRGB888, logoPixel, &pDataARGB8888, logoPixel*4);
    if(ret)
    {
        dprintf(INFO, "Error::: set logo addr failed !\n");
        goto err_out;
    }
#endif

    if (ptn_read("logo", 0, lenTotal, pPartitionData))
    {
        dprintf(INFO, "FAILED: failed to read date for /logo\n");
        goto err_out;
    }

    pfileBuf = pPartitionData + sizeof(flybootloader_header_t);

    pixelDataStartIndex = sizeof("FLYAUDIOLOGOFLY") + sizeof(sLogo) - sizeof(unsigned char *);
    pixelDataEndIndex = fileLen - strlen("FLYAUDIO");
    dprintf(INFO, "pixelDataStartIndex -> %u\n", pixelDataStartIndex);
    dprintf(INFO, "pixelDataEndIndex -> %u\n", pixelDataEndIndex);

    if (FALSE == logoDataDecompress(pixelDataStartIndex, pixelDataEndIndex, pfileBuf, pDataRGB565, logoPixel * 2))
    {
        dprintf(INFO, "fail to logoDataDecompress\n");
        goto err_out;
    }
    if((RecoveryMeg.hwInfo.info[12] - '0') != 0)
    	logo_reverse(pDataRGB565, &pLogoInfo->width, &pLogoInfo->height, (RecoveryMeg.hwInfo.info[12] - '0'));

#if (LOGO_FORMAT == RGB888)
    if (FALSE == rgb565To888(pDataRGB888, pDataRGB565, logoPixel))
    {
        dprintf(INFO, "fail to rgb565To888\n");
        goto err_out;
    }

#elif(LOGO_FORMAT == ARGB8888)
	if(FALSE == RGB565_TO_ARGB8888((unsigned int*)pDataARGB8888, pDataRGB565, logoPixel))
	{
	dprintf(INFO, "fail to RGB565_TO_ARGB8888\n");
	goto err_out;
	}
#else
#endif
    LogoRGB888Info.x_position = pLogoInfo->x_position;
    LogoRGB888Info.y_position = pLogoInfo->y_position;
    LogoRGB888Info.width = pLogoInfo->width;
    LogoRGB888Info.height = pLogoInfo->height;
    LogoRGB888Info.back_color = pLogoInfo->back_color;
    LogoRGB888Info.reserved = pLogoInfo->reserved;
    LogoRGB888Info.data_len = pLogoInfo->data_len;

#if (LOGO_FORMAT == RGB888)
    LogoRGB888Info.pdata = (unsigned char *)pDataRGB888;
#elif(LOGO_FORMAT == ARGB8888)
	LogoRGB888Info.pdata = (unsigned char *)pDataARGB8888;
#else
    LogoRGB888Info.pdata = (unsigned char *)pDataRGB565;
#endif
    display_logo_on_screen(&LogoRGB888Info);

#ifdef BOOTLOADER_IMX6Q
    memcpy ((unsigned char *)(0x16100000), (unsigned char *)fb_base_get(), 1024*600*2);
#endif

#ifndef LOGO_ADD_ALLOCED
    if (NULL != pDataRGB565)
    {
        free(pDataRGB565);
        pDataRGB565 = NULL;
    }

    if (NULL != pDataRGB888)
    {
        free(pDataRGB888);
        pDataRGB888 = NULL;
    }

    if (NULL != pDataARGB8888)
    {
        free(pDataARGB8888);
        pDataARGB8888 = NULL;
    }

    if (NULL != pPartitionData)
    {
        free(pPartitionData);
        pPartitionData = NULL;
    }

    if (NULL != tempBuf)
    {
        free(tempBuf);
        tempBuf = NULL;
    }
#endif
    ret = TRUE;

err_out:
    //if (NULL != pDataRGB565)
    //{
    //	free(pDataRGB565);
    //	pDataRGB565 = NULL;
    //}

    //if (NULL != pDataRGB888)
    //{
    //	free(pDataRGB888);
    //	pDataRGB888 = NULL;
    //}

    //if (NULL != pPartitionData)
    //{
    //	free(pPartitionData);
    //	pPartitionData = NULL;
    //}

    if (NULL != tempBuf)
    {
        free(tempBuf);
        tempBuf = NULL;
    }

    return ret;
}



#endif

