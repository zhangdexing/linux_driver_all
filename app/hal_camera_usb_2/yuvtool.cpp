#include "yuvtool.h"

typedef struct _VSImage 
{
    char *pixels;
    int width;
    int height;
    int stride;
}VSImage;

static void vs_scanline_resample_nearest_Y (char * dest, char * src, int src_width,
    int n, int *accumulator, int increment)
{
    int acc = *accumulator;
    int i;
    int j;
    int x;

    for (i = 0; i < n; i++) {
    j = acc >> 16;
    x = acc & 0xffff;
    dest[i] = (x < 32768 || j + 1 >= src_width) ? src[j] : src[j + 1];

    acc += increment;
    }

    *accumulator = acc;
}


static void vs_scanline_resample_linear_Y (char * dest, char * src, int src_width,
    int n, int *accumulator, int increment)
{
    int acc = *accumulator;
    int i;
    int j;
    int x;

    for (i = 0; i < n; i++) 
    {
        j = acc >> 16;
        x = acc & 0xffff;

        if (j + 1 < src_width)
              dest[i] = (src[j] * (65536 - x) + src[j + 1] * x) >> 16;
        else
              dest[i] = src[j];

        acc += increment;
    }

    *accumulator = acc;
}


static void orc_merge_linear_u8 (char * d1, const char * s1, const char * s2,
    int p1, int p2, int n)
{
    int i;
    char var0;
    char *ptr0;
    char var4;
    const char *ptr4;
    char var5;
    const char *ptr5;
    const short var16 = 128;
    const short var17 = 8;
    const int var24 = p1;
    const int var25 = p2;
    short var32;
    short var33;
    short var34;
    short var35;
    short var36;

    ptr0 = (char *) d1;
    ptr4 = (char *) s1;
    ptr5 = (char *) s2;

    for (i = 0; i < n; i++) 
    {
        var4 = *ptr4;
        ptr4++;
        var5 = *ptr5;
        ptr5++;
        /* 0: mulubw */
        var32 = (char) var4 *(char) var24;
        /* 1: mulubw */
        var33 = (char) var5 *(char) var25;
        /* 2: addw */
        var34 = var32 + var33;
        /* 3: addw */
        var35 = var34 + var16;
        /* 4: shruw */
        var36 = ((short) var35) >> var17;
        /* 5: convwb */
        var0 = var36;
        *ptr0 = var0;
        ptr0++;
    }
}


static void vs_scanline_merge_linear_Y (char * dest, char * src1, char * src2, int n, int x)
{
    int value = x >> 8;

    if (value == 0)
    {
        memcpy (dest, src1, n);
    }
    else 
    {
        orc_merge_linear_u8 (dest, src1, src2, 256 - value, value, n);
    }
}

static void vs_image_scale_nearest_Y (const VSImage * dest, const VSImage * src,  char * tmpbuf)
{
    int acc;
    int y_increment;
    int x_increment;
    int i;
    int j;
    int xacc;
	char * tmpbuf_bk = tmpbuf;

    if (dest->height == 1)
        y_increment = 0;
    else
        y_increment = ((src->height - 1) << 16) / (dest->height - 1);

    if (dest->width == 1)
        x_increment = 0;
    else
        x_increment = ((src->width - 1) << 16) / (dest->width - 1);


    acc = 0;
    for (i = 0; i < dest->height; i++) 
    {
        j = acc >> 16;

        xacc = 0;
        vs_scanline_resample_nearest_Y (dest->pixels + i * dest->stride,
            src->pixels + j * src->stride, src->width, dest->width, &xacc,
            x_increment);

        acc += y_increment;
    }
}

static void vs_image_scale_linear_Y (const VSImage * dest, const VSImage * src, char * tmpbuf)
{
    int acc;
    int y_increment;
    int x_increment;
    char *tmp1;
    char *tmp2;
    int y1;
    int y2;
    int i;
    int j;
    int x;
    int dest_size;
    int xacc;

    if (dest->height == 1)
        y_increment = 0;
    else
        y_increment = ((src->height - 1) << 16) / (dest->height - 1);

    if (dest->width == 1)
        x_increment = 0;
    else
        x_increment = ((src->width - 1) << 16) / (dest->width - 1);

    dest_size = dest->width;

    tmp1 = tmpbuf;
    tmp2 = tmpbuf + dest_size;

    acc = 0;
    xacc = 0;
    y2 = -1;
    vs_scanline_resample_linear_Y (tmp1, src->pixels, src->width, dest->width,     &xacc, x_increment);
    y1 = 0;
    for (i = 0; i < dest->height; i++) 
    {
        j = acc >> 16;
        x = acc & 0xffff;

        if (x == 0) 
        {
              if (j == y1) 
              {
                memcpy (dest->pixels + i * dest->stride, tmp1, dest_size);
              } else if (j == y2) 
              {
                memcpy (dest->pixels + i * dest->stride, tmp2, dest_size);
              }
              else 
              {
                xacc = 0;
                vs_scanline_resample_linear_Y (tmp1, src->pixels + j * src->stride, src->width, dest->width, &xacc, x_increment);
                y1 = j;
                memcpy (dest->pixels + i * dest->stride, tmp1, dest_size);
              }
        }
        else 
        {
              if (j == y1) 
              {
                if (j + 1 != y2) 
                {
                      xacc = 0;
                      vs_scanline_resample_linear_Y (tmp2, src->pixels + (j + 1) * src->stride, src->width, dest->width, &xacc, x_increment);
                      y2 = j + 1;
                }
                   vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride, tmp1, tmp2, dest->width, x);
              } 
              else if (j == y2) 
              {
                if (j + 1 != y1) 
                {
          xacc = 0;
          vs_scanline_resample_linear_Y (tmp1,
              src->pixels + (j + 1) * src->stride, src->width, dest->width,
              &xacc, x_increment);
          y1 = j + 1;
        }
        vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride,
            tmp2, tmp1, dest->width, x);
      } else {
        xacc = 0;
        vs_scanline_resample_linear_Y (tmp1, src->pixels + j * src->stride,
            src->width, dest->width, &xacc, x_increment);
        y1 = j;
        xacc = 0;
        vs_scanline_resample_linear_Y (tmp2,
            src->pixels + (j + 1) * src->stride, src->width, dest->width, &xacc,
            x_increment);
        y2 = (j + 1);
        vs_scanline_merge_linear_Y (dest->pixels + i * dest->stride,
            tmp1, tmp2, dest->width, x);
      }
    }

    acc += y_increment;
  }
}

void resample_yv12(char* dest, int dest_w, int dest_h, char* src, int src_w, int src_h, SCALE_TYPE scale_type)
{
    VSImage dest_y;
    VSImage dest_u;
    VSImage dest_v;
    VSImage src_y;
    VSImage src_u;
    VSImage src_v;

    char *tmp_buf = (char *)malloc(dest_w*4*2);
    if(tmp_buf == NULL)
        return;
        
    dest_y.pixels = dest;
    dest_y.width = dest_w;
    dest_y.height = dest_h;
    dest_y.stride = dest_w;
    
    dest_u.pixels = dest + dest_w*dest_h;
    dest_u.width = dest_w/2;
    dest_u.height = dest_h/2;
    dest_u.stride = dest_w/2;
    
    dest_v.pixels = dest + dest_w*dest_h*5/4;
    dest_v.width = dest_w/2;
    dest_v.height = dest_h/2;
    dest_v.stride = dest_w/2;
    
    src_y.pixels = src;
    src_y.width = src_w;
    src_y.height = src_h;
    src_y.stride = src_w;
    
    src_u.pixels = src + src_w*src_h;
    src_u.width = src_w/2;
    src_u.height = src_h/2;
    src_u.stride = src_w/2;
    
    src_v.pixels = src + src_w*src_h*5/4;
    src_v.width = src_w/2;
    src_v.height = src_h/2;
    src_v.stride = src_w/2;
    
    if(scale_type == SCALE_TYEP_NEAREST)
    {
        vs_image_scale_nearest_Y (&dest_y, &src_y, tmp_buf);
        vs_image_scale_nearest_Y (&dest_u, &src_u, tmp_buf);
        vs_image_scale_nearest_Y (&dest_v, &src_v, tmp_buf);
    }
    else if(scale_type == SCALE_TYPE_BILINEAR)
    {
        vs_image_scale_linear_Y (&dest_y, &src_y, tmp_buf);
        vs_image_scale_linear_Y (&dest_u, &src_u, tmp_buf);
        vs_image_scale_linear_Y (&dest_v, &src_v, tmp_buf);
    }
    free(tmp_buf);
}


	void swapYV12toNV12(char* yv12bytes, char* nv12bytes,int width,int height)
	{
		int nLenY = width * height;
		int nLenU = nLenY >> 2;

		memcpy(nv12bytes,yv12bytes,width * height); 
		for	(int i = 0; i < nLenU; i++) {
			nv12bytes[nLenY + 2 * i + 1] = yv12bytes[nLenY + nLenU + i];//yv12bytes[nLenY + i];
			nv12bytes[nLenY + 2 * i] = yv12bytes[nLenY + i];//yv12bytes[nLenY + nLenU + i];
		}
	}

	void swapYV12to420I(char* yv12bytes, char* yuv420bytes,int width,int height)
	{
		int nLenY = width * height;
		int nLenU = nLenY >> 2;

		memcpy(yuv420bytes,yv12bytes,width * height); 
		for	(int i = 0; i < nLenU; i++) {
			yuv420bytes[nLenY + i] = yv12bytes[nLenY + nLenU + i];
			yuv420bytes[nLenY + nLenU + i] = yv12bytes[nLenY + i];
		}
	}

	
	int YUV422To420(char *pYUV422, char *pYUV420, int lWidth, int lHeight)
	{
		int i = 0, j = 0;
		char *pY = pYUV420;
		char *pV = pYUV420 + lWidth * lHeight;
		char *pU = pV + (lWidth * lHeight) / 4;

		char *pYUVTemp = pYUV422;
		char *pYUVTempNext = pYUV422 + lWidth * 2;

		for (i = 0; i < lHeight; i += 2)
		{
		    for (j = 0; j < lWidth; j += 2)
		    {
		        pY[j] = *pYUVTemp++;
		        pY[j + lWidth] = *pYUVTempNext++;

		        pU[j/2] = *pYUVTemp++;
		        pYUVTempNext++;

		        pY[j+1] = *pYUVTemp++;
		        pY[j+1+lWidth] = *pYUVTempNext++;


		        pV[j/2] = *pYUVTempNext++;
				pYUVTemp++;
		    }

		    pYUVTemp += lWidth*2;
		    pYUVTempNext += lWidth*2;
		    pY += lWidth*2;
		    pU += lWidth/2;
		    pV += lWidth/2;
		}

		return 0;
	}

	void YV12AddPadding(char *inyuv,char *outyuv,int width,int height,int padding)	
	{
		char *inuv,*outuv;
		int size = width * height;
		memcpy(outyuv,inyuv,size); 
		inuv = inyuv + size;
		outuv = outyuv + size;

		for	(int i=0; i < height; i++) {
			for(int j=0; j < width>>1; j++)
			{
				*outuv++ = *inuv++;
			}
			outuv += padding;
		}
	}
	

