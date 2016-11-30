
#include "lidbg_servicer.h"
#include "../../drivers/inc/lidbg_flycam_par.h" /*flycam parameter*/

#include <math.h>

pthread_t thread_checkStatus_id;
int fd = 0,fd_txt = 0;
struct pollfd fds;

//ioctl
#define READ_CAM_PROP(magic , nr) _IOR(magic, nr ,int) 
#define WRITE_CAM_PROP(magic , nr) _IOW(magic, nr ,int)

struct acceleration {
	int x;
	int y;
	int z;
};

void startRec(void)
{
	int ret;
	ret = ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_START_REC), NULL);
	if (ret == RET_NOTVALID)
      	lidbg("@@=========Camera not valid !!=======\n");
	else if (ret == RET_NOTSONIX)
		lidbg("@@=========Camera not sonix !!=======\n");
	else if (ret == RET_IGNORE)
		lidbg("@@=========cmd ignore !!=======\n");
	else
		lidbg("@@========Camera rec done ===%d====\n",ret);
	return 0;
}

void *thread_checkStatus(void *par)
{
	unsigned char cam_status;
	fds.fd     = fd;
   	fds.events = POLLIN;
	int ret;
	while(1)
	{
		fds.revents = 0;
		ret = poll(&fds, 1, -1);
		if (ret <= 0)
		{
			lidbg("time out\n");
			continue;
		}
		if(fds.revents&POLLIN)
		{
			read(fd, &cam_status, 1);
			//printf("[knob0]knob_val=>%x\n",knob_val);
			lidbg("====[cam]cam_status =>0x%x====\n",cam_status);
		}
	}
	return 0;
}
int main(int argc, char **argv)
{
	int i = 0,count = 5;
  	int ret;
	char fw_version[256];
	char testCMD[200];
	char logLine[200];
	lidbg("@@lidbg_ioctl start\n");

open_dev:
	fd = open("/dev/mc3xxx", O_RDWR);
	if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
	{
	    lidbg("@@open mc3xxx fail\n");
	    goto open_dev;
	}

	lidbg("@@open mc3xxx ok\n");

	system("chmod 0777 /dev/mc3xxx");
	sleep(1);

	fd_txt= open("/sdcard/gsensor_angle.txt",  O_RDWR|O_CREAT|O_TRUNC, 0777);
	if((fd == 0xfffffffe) || (fd == 0) || (fd == 0xffffffff))
	{
	    lidbg("@@open gsensor_angle fail\n");
	}

	


#if 0
	ret = pthread_create(&thread_checkStatus_id,NULL,thread_checkStatus,NULL);
	if(ret != 0)
	{
		lidbg( "@@Create pthread error!\n");
		return 1;
	}
#endif
/*
	testCMD[0] = CMD_DUAL_CAM;
	testCMD[1] = 0x0;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
*/

	//testCMD[0] = CMD_SET_RESOLUTION;
	//strcpy(testCMD + 1,"1280x720");

	struct acceleration accel;
	double rad;
	while(1)
	{
		sleep(1);
		read(fd, &accel, sizeof(struct acceleration));
		rad = atan(accel.x/sqrt(accel.y*accel.y + accel.z*accel.z));
		lidbg("GsensorData:%d,%d,%d====%f",accel.x,accel.y,accel.z,(rad/3.14)*180);
		sprintf(logLine , "%d,%d,%d,%f\n",accel.x,accel.y,accel.z,(rad/3.14)*180);
		write(fd_txt,logLine, strlen(logLine));
	}

#if 0
	testCMD[0] = CMD_TIME_SEC;
	testCMD[1] = 0x1;
	testCMD[2] = 0x90;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	lidbg("@@==CMD_TIME_SEC==0:0x%x,1:%d,2:%d,3:%d,4:%d\n",testCMD[0],testCMD[1],testCMD[2],testCMD[3],testCMD[4]);

	testCMD[0] = CMD_FW_VER;
	testCMD[1] = 0x00;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	lidbg("@@==CMD_FW_VER==0:0x%x,1:%d,2:%d,3:%s\n",testCMD[0],testCMD[1],testCMD[2],testCMD + 3);

	testCMD[0] = CMD_TOTALSIZE;
	testCMD[1] = 0x0;
	testCMD[2] = 0x0;
	testCMD[3] = 0x14;
	testCMD[4] = 0x1;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	lidbg("@@==CMD_TOTALSIZE==0:0x%x,1:%d,2:%d,3:%d,4:%d,5:%d,6:%d\n",testCMD[0],testCMD[1],testCMD[2],testCMD[3],testCMD[4],testCMD[5],testCMD[6]);

	testCMD[0] = CMD_PATH;
	strcpy(testCMD + 1,"/storage/sdcard0/era_rec/");
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	lidbg("@@==CMD_PATH==0:0x%x,1:%d,2:%d,3:%s\n",testCMD[0],testCMD[1],testCMD[2],testCMD + 3);

	testCMD[0] = CMD_GET_RES;
	testCMD[1] = 0x00;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	lidbg("@@==CMD_GET_RES==0:0x%x,1:%d,2:%d,3:%s\n",testCMD[0],testCMD[1],testCMD[2],testCMD + 3);

	testCMD[0] = CMD_RECORD;
	testCMD[1] = 0x1;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);

	lidbg("@@==CMD_RECORD==0:0x%x,1:%d,2:%d,3:%d\n",testCMD[0],testCMD[1],testCMD[2],testCMD[3]);
	lidbg("@@==CMD_RECORD==4:0x%x,5:%d,6:%d,7:%d\n",testCMD[4],testCMD[5],testCMD[6],testCMD[7]);

	sleep(10);
	testCMD[0] = CMD_DUAL_CAM;
	testCMD[1] = 0x1;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);
	
	testCMD[0] = CMD_SET_PAR;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);

	sleep(30);
	/*
	if (ret != 0)
      {
      	lidbg("@@NR_PATH ioctl fail===%d===\n",ret);
	}
	lidbg("@@FW Version ===> %s\n" , fw_version);
	*/
	testCMD[0] = CMD_RECORD;
	testCMD[1] = 0x0;
	ret = ioctl(fd,_IO(FLYCAM_REC_MAGIC, NR_CMD), testCMD);

#endif

#if 0
	while(count--)
	{
		ret = ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_PATH), "/storage/sdcard0/camera_rec/");
		if (ret != 0)
        {
        	lidbg("@@NR_PATH ioctl fail===%d===\n",ret);
		}
		ret = ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_RESOLUTION), "1280x720");
		if (ret != 0)
        {
        	lidbg("@@NR_RESOLUTION ioctl fail====%d===\n",ret);
		}
		ret = ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_TIME), 300); 
		if (ret != 0)
        {
        	lidbg("@@NR_TIME ioctl fail====%d===\n",ret);
		}
		
		ret = ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_SET_PAR), NULL); 
		if (ret != 0)
        {
        	lidbg("@@NR_SET_PAR ioctl fail====%d===\n",ret);
		}
		
		/*start recording*/
		lidbg("@@========START========\n");
		startRec();
		sleep(500);
		/*stop recording*/
		if (ioctl(fd,_IO(FLYCAM_REAR_REC_IOC_MAGIC, NR_STOP_REC), NULL) < 0)
	    {
	      	lidbg("@@NR_STOP_REC ioctl fail=======\n");
		}
		lidbg("@@========STOP========\n");
		//check_status();
		//sleep(1);
	}
#endif
	lidbg("@@=======DONE=========");
	return 0;
}

