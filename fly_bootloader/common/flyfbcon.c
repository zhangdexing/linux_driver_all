#include "soc.h"
#include "fly_private.h"
#include "ascii24.h"
#include "fly_ascii16.h"
#ifdef BOOTLOADER_TYPE_UBOOT
#include <common.h>
#endif

char flyrecord[flyblk_a][flyblk_b];
#define   script_y   450
#define   script_x   20
#define   scriptgap 20
int   scriptcnt;
static int printf_list = 29;
static int printf_length = 40;

typedef struct
{
    u16		x_position;
    u16		y_position;
    u16		width;
    u16		height;
    u16		back_color;		//èæ¯è²ï\u0152RGB565å\u0152ï\u0152
    u16		reserved;		//ä¿ç2B
    u32		data_len;		//æ°æ®é¿åº\u0160
    u8		*pdata;			//æ°æ®å?
} sLogo, *psLogo;


#define      asc16       ascii16
#define      asc24       ascii24

#define GET_COLOR_RGB555(r,g,b)  ((((r)&0x1F)<<11)|(((g)&0x3F)<<6)|((b)&0x1F))//555
#define GET_COLOR_RGB565(r,g,b)  ((((r)&0x1F)<<11)|(((g)&0x3F)<<5)|((b)&0x1F))//565
#define GET_COLOR_RGB888(r,g,b)  ((((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF))//888

char *text[20];
int text_row = 0;


#if  1
void display_logo_on_screen(sLogo *plogoparameter)
{
    unsigned i = 0;
    unsigned total_x;
    unsigned total_y;
    unsigned bytes_per_bpp;
    unsigned image_base_hdpi;
    unsigned image_base_wdpi;
    sLogo *logo_show = plogoparameter;
    char *pImageBuffer = logo_show->pdata;

    total_x = logo_show->x_position;
    total_y = logo_show->y_position;
    image_base_hdpi = logo_show->height;
    image_base_wdpi = logo_show->width;
    bytes_per_bpp = (FBCON_BPP / 8);//config->bpp

    dprintf(INFO, "******************************************************************************************\n");
    dprintf(INFO, "**flyaudio logo ===>>>> total_x:%d total_y:%d  image_base_hdpi:%d image_base_wdpi:%d**\n",
            total_x, total_y, image_base_hdpi, image_base_wdpi);
    dprintf(INFO, "******************************************************************************************\n");

    //set logo back color
    fly_setBcol(BG_COL);
    //fly_fbcon_clear();

#ifdef  DISPLAY_TYPE_MIPI
    if (bytes_per_bpp == 3)
    {
        if (CLEAN_SCREEN_WRITE)
        {
            // memset (config->base, 0x00ff00, config->width * config->height * bytes_per_bpp);
            //	memset (config->base, 0xff0008,1024*600*3);
            //	debug_messages("flyaudio");
        }

        if(FLY_SCREEN_SIZE_1024)
        {
            for (i = 0; i < image_base_hdpi ; i++)
            {
                memcpy ((fb_base_get() + (((1024 - image_base_wdpi) / 2 +  ((i + (600 - image_base_hdpi) / 2) * 1024)) * bytes_per_bpp)),
                        pImageBuffer + (i * image_base_wdpi * bytes_per_bpp) ,
                        image_base_wdpi * bytes_per_bpp);
            }
        }
        else
        {
            /*
            			for (i = 0; i < 480 ; i++)
            			{
            				memcpy ((config->base + ((0 + (i * (config->width))) * bytes_per_bpp)),
            				pImageBuffer + (i * 800 * bytes_per_bpp) ,
            				800 * bytes_per_bpp);
            			}
            */
            for (i = 0; i < image_base_hdpi ; i++)
            {
                memcpy ((fb_base_get() + (((800 - image_base_wdpi) / 2 +  ((i + (480 - image_base_hdpi) / 2) * 800)) * bytes_per_bpp)),
                        pImageBuffer + (i * image_base_wdpi * bytes_per_bpp) ,
                        image_base_wdpi * bytes_per_bpp);
            }
        }
    }
	else if(bytes_per_bpp == 4){
		if(FLY_SCREEN_SIZE_1024){
			for(i = 0; i < image_base_hdpi; i++){
				memcpy((fb_base_get() + (((1024 - image_base_wdpi) / 2 +((i + (600 - image_base_hdpi) / 2) * 1024)) * bytes_per_bpp)),
					pImageBuffer + (i * image_base_wdpi * bytes_per_bpp),
					image_base_wdpi * bytes_per_bpp);
			}
		}
	}
#else
    if (bytes_per_bpp == 2)// 2
    {
        if (CLEAN_SCREEN_WRITE)
        {
            memset (fb_base_get(), 0xf1, FBCON_WIDTH * FBCON_HEIGHT * bytes_per_bpp);
        }
        if(FLY_SCREEN_SIZE_1024)
        {
            for (i = 0; i < image_base_hdpi ; i++)
            {
                memcpy ((fb_base_get() + (((1024 - image_base_wdpi) / 2 +  ((i + (600 - image_base_hdpi) / 2) * 1024)) * bytes_per_bpp)),
                        pImageBuffer + (i * image_base_wdpi * bytes_per_bpp) ,
                        image_base_wdpi * bytes_per_bpp);
            }
#ifdef BOOTLOADER_IMX6Q
	flush_memery
#endif
        }
        else
        {
            for (i = 0; i < 480; i++)
            {
                memcpy (fb_base_get() + ((0 + (i * FBCON_WIDTH)) * bytes_per_bpp),
                        pImageBuffer + (i * 800 * bytes_per_bpp),
                        800 * bytes_per_bpp);
            }
        }
    }
#endif
#ifdef BOOTLOADER_MT3561
	flush_memery
#endif
#ifdef BOOTLOADER_TYPE_LK
    arch_clean_invalidate_cache_range((unsigned long)fb_base_get(), FBCON_WIDTH * FBCON_HEIGHT * 3);
#endif

    //fbcon_flush();
    /*
    	if(is_cmd_mode_enabled())
    		mipi_dsi_cmd_mode_trigger();
    */
}

/* TODO: take stride into account */

void fly_fbcon_clear(void)
{
    unsigned count = 0;

    count = FBCON_WIDTH * FBCON_HEIGHT;
    dprintf(INFO, "Fbcon clear, width[%d] height[%d] base[0x%x]\n", FBCON_WIDTH, FBCON_HEIGHT, fb_base_get());
    memset(fb_base_get(), 0x00, count * (FBCON_BPP / 8));
#ifdef BOOTLOADER_IMX6Q
	flush_memery
#endif
#ifdef BOOTLOADER_MT3561
	flush_memery
#endif
}

void fly_setBcol(unsigned long int backcolor)
{
    /*
    	int de;
    	int i = 0;
    	int j = 0;
    	int wight = 1024;
    	int hight = 600;
    		unsigned char R;
    		unsigned char G;
    		unsigned char B;

    		R = backcolor>>16&0x0000ff;
    		G = backcolor>>8&0x0000ff;
    		B = backcolor&0x0000ff;


    	for(i=0;i<hight;i++)
    	for(j=0;j<wight*3;)
    	{
    		memset (config->base  +  i * config->width+j , R,1);
    		memset (config->base  +  i * config->width+j+1, G, 1);
    		memset (config->base  +  i * config->width+j+2, B , 1);
    		j+=3;
    	}
    */
#if (LOGO_FORMAT == RGB888)
    unsigned char *tem = malloc(1024 * 3);
    int i = 0, m = 0;

    unsigned char R;
    unsigned char G;
    unsigned char B;

    R = backcolor >> 16 & 0xff;
    G = backcolor >> 8 & 0xff;
    B = backcolor & 0xff;

    for(i = 0; i < 1024; i++)
    {
        tem[m++] = B;
        tem[m++] = G;
        tem[m++] = R;
    }
    for (i = 0; i < 600; i++)
    {
        memcpy (fb_base_get() + ((0 + (i * FBCON_WIDTH)) * 3),
                tem,
                1024 * 3);
    }
#ifdef BOOTLOADER_TYPE_LK
	arch_clean_invalidate_cache_range((unsigned long)fb_base_get(), FBCON_WIDTH * FBCON_HEIGHT * 3);
#endif
    free(tem);
#elif(LOGO_FORMAT == ARGB8888)
    unsigned char *tem = malloc(1024 * 4);
    int i = 0, m = 0;
    unsigned char A;
    unsigned char R;
    unsigned char G;
    unsigned char B;
    A = 0xff;
    R = backcolor >> 16 & 0xff;
    G = backcolor >> 8 & 0xff;
    B = backcolor & 0xff;

    for(i = 0; i < 1024; i++)
    {

        tem[m++] = B;
        tem[m++] = G;
        tem[m++] = R;
        tem[m++] = A;
    }
    for (i = 0; i < 600; i++)
    {
        memcpy (fb_base_get() + ((0 + (i * FBCON_WIDTH)) * 4),
                tem,
                1024 * 4);
    }

    free(tem);
#else
    u16 *ptr;
    int i, j;
    unsigned char R = backcolor >> 16 & 0xff;
    unsigned char G = backcolor >> 8 & 0xff;
    unsigned char B = backcolor & 0xff;

    ptr = gd->fb_base;
    for(i = 0; i < 1024; i++)
        for(j = 0; j < 600; j++)
            *ptr++ = GET_COLOR_RGB565(R, G, B);
#ifdef BOOTLOADER_IMX6Q
	flush_memery
#endif
#endif
#ifdef BOOTLOADER_MT3561
	flush_memery
#endif
}


void FlySetLogoBcol(unsigned short  backcolor)
{

    unsigned char *tem = malloc(1024 * 3);
    int i = 0, m = 0;

    unsigned char R;
    unsigned char G;
    unsigned char B;
    //RGB 565 CHANGES RGB888
    R	=	(backcolor >> 8) & 0x00F8;
    G	=	(backcolor >> 3) & 0x00FC;
    B	=	(backcolor << 3) & 0x00F8;
    //STORE RGB888 DATA TO tem
    for(i = 0; i < 1024; i++)
    {
        tem[m++] = B;
        tem[m++] = G;
        tem[m++] = R;
    }
    //set RGB888 DATA TO SREEN

    for (i = 0; i < 600; i++)
    {
        memcpy (fb_base_get() + ((0 + (i * FBCON_WIDTH)) * 3), tem, 1024 * 3);
#ifdef BOOTLOADER_TYPE_LK
        arch_clean_invalidate_cache_range((unsigned long)fb_base_get() + ((0 + (i * FBCON_WIDTH)) * 3), FBCON_WIDTH * 3);
#endif

    }

    free(tem);

}

void fly_putpext(int x, int y, unsigned long  color)
{
    unsigned char *tem = malloc(20 * 4);
    int i = 0, m = 0;
    unsigned char R;
    unsigned char G;
    unsigned char B;
    //RGB 565 CHANGES RGB888
    /*
    		R	=	(color>>8)&0x00F8;
    		G	=	(color>>3)&0x00FC;
    		B	=	(color<<3)&0x00F8;
    */
    R	=	(color >> 16) & 0x00FF;
    G	=	(color >> 8) & 0x00FF;
    B	=	color & 0x00FF;
    //STORE RGB888 DATA TO tem

#if (LOGO_FORMAT == RGB888)
    for(i = 0, m = 0; i < 1; i++)
    {
        tem[m++] = B;
        tem[m++] = G;
        tem[m++] = R;
    }
    //set RGB888 DATA TO SREEN
    memcpy (fb_base_get() + ((x + (y * FBCON_WIDTH)) * 3), tem, 1 * 3);
#ifdef BOOTLOADER_TYPE_LK
    arch_clean_invalidate_cache_range((unsigned long)fb_base_get() + ((x + (y * FBCON_WIDTH)) * 3), 1 * 3);
#endif
    free(tem);
#elif(LOGO_FORMAT == ARGB8888)
    for(i = 0, m = 0; i < 1; i++)
    {
        tem[m++] = B;
        tem[m++] = G;
        tem[m++] = R;
	 tem[m++] = 0xff;
    }
    //set RGB888 DATA TO SREEN
    memcpy (fb_base_get() + ((x + (y * FBCON_WIDTH)) * 4), tem, 1 * 4);
    free(tem);
#else
    for(i = 0, m = 0; i < 1; i++)
    {
        tem[m++] = GET_COLOR_RGB565(R, G, B);
        tem[m++] = GET_COLOR_RGB565(R, G, B) >> 8;
    }
    memcpy (fb_base_get() + ((x + (y * FBCON_WIDTH)) * 2), tem, 1 * 2);
#ifdef BOOTLOADER_TYPE_UBOOT
	flush_cache(fb_base_get() + ((x + (y * FBCON_WIDTH)) * 2),1 * 2);
#endif
    free(tem);
#endif
}



static void fly_ascii_font(int x, int y, const char *ascii , unsigned long fontcol)
{
    int i, j;
    int bits;
    for(i = 0; i < 16; i++)
    {
        bits = ascii[i];
        for(j = 0; j < 8; j++, bits <<= 1)
            if(bits & 0x80)
            {
                fly_putpext((x + j), (y + i), fontcol);
                fly_putpext((x + j + 1), (y + i), fontcol);
            }
    }
}

static void fly_ascii_font24(int x, int y, const char *ascii , unsigned long fontcol)
{
    int i, j, k;
    //dprintf(INF"ascii = 0x%c\n",*ascii);
    for(i = 0; i < 24; i++)
        for(j = 0; j < 2; j++)
        {
            //dprintf(INFO,"ascii = 0x%d\n",*ascii);
            for(k = 0; k < 8; k++)
            {
                if(ascii[i * 2 + j] & (0x80 >> k))
                {
                    //fly_putpext(x+j*8+k,y+i, fontcol);
                    fly_putpext(x + j * 8 + k, y + i, fontcol);
                }

            }
        }
}


void fly_text16(int x, int y, const char *s, unsigned long fontcol)
{
    char *p = s;
    int offset;
    char *pen;
    y -= 16;
    while((*p != 0) && (*p != '\0'))
    {
        offset = (*p - 0x20) * 16;
        pen = asc16 + offset;
        fly_ascii_font(x, y, pen, fontcol);
        x += 8;
        p++;
    }
}


void fly_text24(int x, int y, const char *s, unsigned long fontcol)
{
    char *p = s;
    int offset;
    char *pen;
    y -= 24;
    while((*p != 0) && (*p != '\0'))
    {
        //dprintf(INFO,"s = %c \n",*p);
        offset = (*p - 0x20) * 48 ;
        pen = asc24 + offset;
        fly_ascii_font24(x, y, pen, fontcol);
        x += 13;
        p++;
    }
}

void fly_text_lk(int x, int y, const char *s, unsigned long fontcol)
{
    fly_text24(x, y, s, fontcol);
    //fly_text16(x,y, s, fontcol);
}

int  flyprint(char *pmat)
{
    int i, ngap = 0;
    int k = 0;
    int tradesign = 0, reprintsign = 0;
    char dbuff[flyblk_b];

    if(scriptcnt > printf_list)
    {
        for(k = 0; k < printf_list; k++)
            strcpy(flyrecord[k], flyrecord[k + 1]);
        memset(flyrecord[k], 0, sizeof(flyrecord[k]));
        scriptcnt = k;
        k = 0;
    }
    i = scriptcnt;
    int l = strlen(flyrecord[i]);
    while(*pmat != NULL)
    {
        if(*pmat == '\n')
        {
            dbuff[k] = '\0';
            tradesign = 1;
            pmat++;
            if(*pmat !=  NULL) reprintsign = 1;
            goto    print;
        }

        if((k + l) > printf_length)
        {
            dbuff[k] = '\0';
            tradesign = 1;
            reprintsign = 1;
            goto    print;
        }

        dbuff[k] = *pmat;
        pmat++;
        k++;
    }
print:
    strcat(flyrecord[i], dbuff);
    fly_fbcon_clear();

    for(; i >= 0; i--)
    {
        fly_text_lk(script_x, script_y - ngap * scriptgap, flyrecord[i], 0xffff00);
        ngap++;
    }

    mdelay(100);
    if(tradesign )  scriptcnt++;
    if(reprintsign) flyprint(pmat);
    return 0;
}

void fly_print(char *pmt, ...)
{
    char pbuf[256];
    int err;

    va_list ap;
    va_start(ap, pmt);
    err = vsnprintf(pbuf, sizeof(pbuf), pmt, ap);
    va_end(ap);

    flyprint(pbuf);
}

void fly_version(int x, int y, const char *s, unsigned long fontcol, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, s);
    vsnprintf(buf, 256, s, ap);
    va_end(ap);
    char *p = buf;
    fly_text_lk(x, y, p, fontcol);
}


void drawHline(int x0, int y0, int wide, unsigned long color)
{
    for(wide += x0; x0 < wide; x0++)
        fly_putpext(x0, y0, color);
}

void drawVline(int x0, int y0, int hide, unsigned long color)

{
    for(hide += y0; y0 < hide; y0++)
        fly_putpext(x0, y0, color);
}



void drawRect(int x0, int y0, int wide, int hide, unsigned long linecolor, unsigned long backcolor)
{
    int i, j;

    drawHline(x0, y0, wide, linecolor);

    drawHline(x0, (y0 + hide - 1), wide, linecolor);

    drawVline(x0, y0, hide, linecolor);

    drawVline((x0 + wide), y0, hide, linecolor);

    for(i = 0; i < hide; i++)
        for(j = 0; j < wide; j++)
            fly_putpext(x0 + j, y0 + i, backcolor);

}

#endif


