
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

#define GSENSOR_DEV_PATH						"/dev/mc3xxx"
#define GSENSOR_ISDEBUG_PROP_NAME	"persist.gsensor.isDebug"
#define GSENSOR_DEBUG_FILE_PATH			"/sdcard/gsensor_angle.txt"

struct acceleration_info {
	bool isACCON;
	bool isIRQCrash;
	int x;
	int y;
	int z;
};

static int x_p_threshold = GSENSOR_X_P_THRESHOLD;
static int x_n_threshold = GSENSOR_X_N_THRESHOLD;
static int y_p_threshold = GSENSOR_Y_P_THRESHOLD;
static int y_n_threshold = GSENSOR_Y_N_THRESHOLD;
static int z_p_threshold = GSENSOR_Z_P_THRESHOLD;
static int z_n_threshold = GSENSOR_Z_N_THRESHOLD;
static int d_p_threshold = GSENSOR_DEGREE_P_THRESHOLD;
static int d_n_threshold = GSENSOR_DEGREE_N_THRESHOLD;

int main(int argc, char **argv)
{
	int isDebug = 0;
	char isDebug_String[PROPERTY_VALUE_MAX];
	int fd = 0,fd_txt = 0;
  	int ret;
	struct acceleration_info accel;
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
	/*Open Gsensor Device*/
	fd = open(GSENSOR_DEV_PATH, O_RDWR);
	if (fd < 0) 
	{
	    lidbg("open mc3xxx fail\n");
		sleep(1);
	    goto open_dev;
	}
	lidbg("open mc3xxx ok\n");
	chmod(GSENSOR_DEV_PATH,0777);
	sleep(1);

	/*Debug Prop*/
	property_get(GSENSOR_ISDEBUG_PROP_NAME, isDebug_String, "0");
	isDebug = atoi(isDebug_String);
	lidbg("========[%s]-> %d =======\n",GSENSOR_ISDEBUG_PROP_NAME,isDebug);
	
	if(isDebug)
	{
		fd_txt= open(GSENSOR_DEBUG_FILE_PATH,  O_RDWR|O_CREAT|O_TRUNC, 0777);
		if (fd < 0) 
		{
		    lidbg("open gsensor_angle.txt fail\n");
			isDebug = 0;
		}
	}

	/*reverse install*/
	while(1)
	{
		usleep(100*1000);
		read(fd, &accel, sizeof(struct acceleration_info));
		if(	accel.z != 0) break;
	}
	if(	accel.z < -700)
	{
		lidbg("%s:====Z REVERSE INSTALL(z:%d)====\n",__func__,accel.z);
		z_p_threshold = -GSENSOR_Z_N_THRESHOLD;
		z_n_threshold = -GSENSOR_Z_P_THRESHOLD;
	}
	else lidbg("%s:====Z NORMAL INSTALL(z:%d)====\n",__func__,accel.z);
	
	while(1)
	{
		usleep(100*1000);
		read(fd, &accel, sizeof(struct acceleration_info));

		/*Suspend Notify: Gsensor IRQ*/
		if(accel.isACCON == false)
		{
			if(accel.isIRQCrash == true)
			{
				lidbg("%s:====Suspend Gsensor IRQ recv====\n",__func__);
				system("am broadcast -a com.flyaudio.lidbg.gsensor --ei action 0");
				sleep(1);
			}
			continue;
		}
		else
		{
			/*Calculating the Z axis angle*/
			rad = atan(accel.x/sqrt(accel.y*accel.y + accel.z*accel.z));
			degree = (rad/3.14)*180;
			//lidbg("GsensorData:%d,%d,%d====%f",accel.x,accel.y,accel.z,(rad/3.14)*180);

			if(isDebug)
			{
				tt=time(NULL);
			    strftime(tmpbuf,80,"%Y-%m-%d,%H:%M:%S\n",localtime(&tt));
			}

			/*useless data*/
			if(accel.x > 3000 && accel.y > 3000 && accel.z > 4000)
			{
				sprintf(logLine , "Error Data:%d,%d,%d\n",accel.x,accel.y,accel.z);
				lidbg(logLine);
				if(isDebug)
					write(fd_txt,logLine, strlen(logLine));
				continue;
			}

			/*Three axis threshold: two times each crash warning*/
			/*Flat Ground No Move HYUNDAI:100,280,900*/
			if(accel.x > x_p_threshold || accel.x < x_n_threshold)
			{
				lidbg("X axis Crash Warning____%d:%d,%d,%d,%f____\n",x_Cnt,accel.x,accel.y,accel.z,degree);
				if(++x_Cnt >= 2) //filter
				{
					sprintf(logLine , "X axis Crash Warning.%s\n",tmpbuf);
					lidbg(logLine);
					if(isDebug)
						write(fd_txt,logLine, strlen(logLine));
					ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
					system("am broadcast -a com.flyaudio.lidbg.gsensor --ei action 0");
					sleep(1);
					x_Cnt = 0;
				}
			}
			else if(accel.y > y_p_threshold || accel.y < y_n_threshold)
			{
				lidbg("Y axis Crash Warning____%d:%d,%d,%d,%f____\n",y_Cnt,accel.x,accel.y,accel.z,degree);
				if(++y_Cnt >= 2) //filter
				{
					sprintf(logLine , "Y axis Crash Warning.%s\n",tmpbuf);
					lidbg(logLine);
					if(isDebug)
						write(fd_txt,logLine, strlen(logLine));
					ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
					system("am broadcast -a com.flyaudio.lidbg.gsensor --ei action 0");
					sleep(1);
					y_Cnt = 0;
				}
			}
			else if(accel.z > z_p_threshold || accel.z < z_n_threshold)
			{
				lidbg("Z axis Crash Warning____%d:%d,%d,%d,%f____\n",z_Cnt,accel.x,accel.y,accel.z,degree);
				if(++z_Cnt >= 2) //filter
				{
					sprintf(logLine , "Z axis Crash Warning.%s\n",tmpbuf);
					lidbg(logLine);
					if(isDebug)
						write(fd_txt,logLine, strlen(logLine));
					ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
					system("am broadcast -a com.flyaudio.lidbg.gsensor --ei action 0");
					sleep(1);
					z_Cnt = 0;
				}
			}
			else if( degree > d_p_threshold || degree < d_n_threshold)
			{
				lidbg("Roll Over Warning____%d:%d,%d,%d,%f____\n",rollOverCnt,accel.x,accel.y,accel.z,degree);
				if(++rollOverCnt >= 2) //filter
				{
					sprintf(logLine , "Roll Over Warning.%s\n",tmpbuf);
					lidbg(logLine);
					if(isDebug)
						write(fd_txt,logLine, strlen(logLine));	
					ioctl(fd,GSENSOR_NOTIFY_CHAIN, 0);
					system("am broadcast -a com.flyaudio.lidbg.gsensor --ei action 0");
					sleep(1);
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
	}

	if(fd >= 0) close(fd);
	if(fd_txt >= 0) close(fd_txt);
	lidbg("=======Gsensor_Det_Main DONE=========");
	return 0;
}

