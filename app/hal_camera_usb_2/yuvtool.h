#ifndef __YUVTOOL_H__
#define __YUVTOOL_H__

#include <stdlib.h>
#include <string.h>

typedef int SCALE_TYPE;
#define SCALE_TYEP_NEAREST 1
#define SCALE_TYPE_BILINEAR 2

void resample_yv12(char* dest, int dest_w, int dest_h, char* src, int src_w, int src_h, SCALE_TYPE scale_type);

void swapYV12toNV12(char* yv12bytes, char* nv12bytes,int width,int height);
int YUV422To420(char *pYUV422, char *pYUV420, int lWidth, int lHeight);
int YUV422To420Mirror(char *pYUV422, char *pYUV420, int lWidth, int lHeight);
void YV12AddPadding(char *inyuv,char *outyuv,int width,int height,int padding);
void swapYV12to420I(char* yv12bytes, char* yuv420bytes,int width,int height);


#endif
