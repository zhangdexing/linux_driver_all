
#include "lidbg_servicer.h"
#include <math.h>


#define GSENSOR                                0x95
#define GSENSOR_NOTIFY_CHAIN                     _IO(GSENSOR,  0x11)

#define GSENSOR_X_P_THRESHOLD					  700
#define GSENSOR_X_N_THRESHOLD				-700
#define GSENSOR_Y_P_THRESHOLD					  800
#define GSENSOR_Y_N_THRESHOLD				-200
#define GSENSOR_Z_P_THRESHOLD					1600
#define GSENSOR_Z_N_THRESHOLD					-300
#define GSENSOR_DEGREE_P_THRESHOLD		   40
#define GSENSOR_DEGREE_N_THRESHOLD		 -40

struct acceleration {
	int x;
	int y;
	int z;
};

int main(int argc, char **argv)
{
	int isDebug = 0;
	char isDebug_String[PROPERTY_VALUE_MAX];
	int fd = 0,fd_txt = 0;
  	int ret;
	struct acceleration accel;
	double rad;
	double degree;
	char logLine[200];
	time_t tt;
    char tmpbuf[80];
	char x_Cnt = 0;
	char y_Cnt = 0;
	char z_Cnt = 0;
	char rollOverCnt = 0;
	
	lidbg("Gsensor_Det_Main start\n");

open_dev:
	fd = open("/dev/mc3xxx", O_RDWR);
	if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
	{
	    lidbg("open mc3xxx fail\n");
		sleep(1);
	    goto open_dev;
	}

	lidbg("open mc3xxx ok\n");

	system("chmod 0777 /dev/mc3xxx");
	sleep(1);

	property_get("persist.gsensor.isDebug", isDebug_String, "0");
	isDebug = atoi(isDebug_String);
	lidbg("========[persist.gsensor.isDebug]-> %d =======\n",isDebug);
	
	if(isDebug)
	{
		fd_txt= open("/sdcard/gsensor_angle.txt",  O_RDWR|O_CREAT|O_TRUNC, 0777);
		if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
		{
		    lidbg("open gsensor_angle.txt fail\n");
		}
	}

	while(1)
	{
		usleep(100*1000);
		read(fd, &accel, sizeof(struct acceleration));
		rad = atan(accel.x/sqrt(accel.y*accel.y + accel.z*accel.z));
		degree = (rad/3.14)*180;
		//lidbg("GsensorData:%d,%d,%d====%f",accel.x,accel.y,accel.z,(rad/3.14)*180);

		if(isDebug)
		{
			tt=time(NULL);
		    strftime(tmpbuf,80,"%Y-%m-%d,%H:%M:%S\n",localtime(&tt));
		}

		if(accel.x > 3000 && accel.y > 3000 && accel.z > 4000)
		{
			sprintf(logLine , "Error Data:%d,%d,%d\n",accel.x,accel.y,accel.z);
			lidbg(logLine);
			if(isDebug)
				write(fd_txt,logLine, strlen(logLine));
			continue;
		}
		
		/*Flat Ground No Move HYUNDAI:100,280,900*/
		if(accel.x > GSENSOR_X_P_THRESHOLD || accel.x < GSENSOR_X_N_THRESHOLD)
		{
			lidbg("X axis Crash Warning____%d:%d,%d,%d,%f____\n",x_Cnt,accel.x,accel.y,accel.z,degree);
			if(++x_Cnt >= 2) //filter
			{
				sprintf(logLine , "X axis Crash Warning.%s\n",tmpbuf);
				lidbg(logLine);
				if(isDebug)
					write(fd_txt,logLine, strlen(logLine));
				ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
				x_Cnt = 0;
			}
		}
		else if(accel.y > GSENSOR_Y_P_THRESHOLD || accel.y < GSENSOR_Y_N_THRESHOLD)
		{
			lidbg("Y axis Crash Warning____%d:%d,%d,%d,%f____\n",y_Cnt,accel.x,accel.y,accel.z,degree);
			if(++y_Cnt >= 2) //filter
			{
				sprintf(logLine , "Y axis Crash Warning.%s\n",tmpbuf);
				lidbg(logLine);
				if(isDebug)
					write(fd_txt,logLine, strlen(logLine));
				ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
				y_Cnt = 0;
			}
		}
		else if(accel.z > GSENSOR_Z_P_THRESHOLD || accel.z < GSENSOR_Z_N_THRESHOLD)
		{
			lidbg("Z axis Crash Warning____%d:%d,%d,%d,%f____\n",z_Cnt,accel.x,accel.y,accel.z,degree);
			if(++z_Cnt >= 2) //filter
			{
				sprintf(logLine , "Z axis Crash Warning.%s\n",tmpbuf);
				lidbg(logLine);
				if(isDebug)
					write(fd_txt,logLine, strlen(logLine));
				ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
				z_Cnt = 0;
			}
		}
		else if( degree > GSENSOR_DEGREE_P_THRESHOLD || degree < GSENSOR_DEGREE_N_THRESHOLD)
		{
			lidbg("Roll Over Warning____%d:%d,%d,%d,%f____\n",rollOverCnt,accel.x,accel.y,accel.z,degree);
			if(++rollOverCnt >= 2) //filter
			{
				sprintf(logLine , "Roll Over Warning.%s\n",tmpbuf);
				lidbg(logLine);
				if(isDebug)
					write(fd_txt,logLine, strlen(logLine));	
				ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
				rollOverCnt = 0;
			}
		}
		else 
		{
			x_Cnt = 0;
			y_Cnt = 0;
			z_Cnt = 0;
			rollOverCnt = 0;
		}
		
		/*LogPrint*/
		if(isDebug)
		{
			sprintf(logLine , "%d,%d,%d,%f,%s\n",accel.x,accel.y,accel.z,degree,tmpbuf);
			write(fd_txt,logLine, strlen(logLine));
		}
	}
	
	lidbg("=======Gsensor_Det_Main DONE=========");
	return 0;
}

