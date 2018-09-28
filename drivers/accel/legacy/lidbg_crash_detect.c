//#include <math.h>

#define DETECT_THRESHOLD	(1000)			//a = 10.00
#define MCONVERT_PARA		981 / 1024		//g = 9.81
static int x_data_bak = 0;
static int y_data_bak = 0;
static int z_data_bak = 0;
static int cnt_exceed_threshold = -1;
static DECLARE_COMPLETION (completion_for_notifier);

#define NOTIFIER_MAJOR_GSENSOR_STATUS_CHANGE	(130)
#define NOTIFIER_MINOR_EXCEED_THRESHOLD 		(10)
/*
double atan_self(double x)   
{   
//atan(x)=x-x^3/3+x^5/5-x^7/7+.....(-1<x<1)   
//return:[-pi/2,pi/2]   
	double pi = 3.14;
    double mult,sum,xx;   
	int i = 0;
	
    sum=0;   
    if(x==1){   
        return pi/4;   
    }   
    if(x==-1){   
        return -pi/4;   
    }   
  
    (x>1||x<-1)?(mult=1/x):(mult=x);   
    xx=mult*mult;   
       
    for(i=1;i<200;i+=2){   
        sum+=mult*((i+1)%4==0?-1:1)/i;   
        mult*=xx;   
    }   
    if(x>1||x<-1){   
        return pi/2-sum;   
    }   
    else{   
        return sum;   
    }   
} 
*/

static void get_gsensor_data(int x, int y, int z)
{
	char temp_cmd[200],temp_cmd2[200];
	double rad2 = 0;
	sprintf(temp_cmd,"1,%d,%d,%d,%s\n",
		-x * MCONVERT_PARA,
		-y * MCONVERT_PARA,
		-z * MCONVERT_PARA,get_current_time());
	sprintf(temp_cmd2,"2,%d,%d,%d,%s\n",
			abs(x - x_data_bak)* MCONVERT_PARA,
			abs(y - y_data_bak)* MCONVERT_PARA,
			abs(z - z_data_bak)* MCONVERT_PARA,get_current_time());

	//rad2 = atan_self((double)(x/int_sqrt(y*y+z*z)));
	//lidbg("======rad2: %f======\n",rad2);
	
	fs_file_write2("/sdcard/gsensorData.txt", temp_cmd);
	fs_file_write2("/sdcard/gsensorData2.txt", temp_cmd2);

	if(cnt_exceed_threshold == -1)
	{
		cnt_exceed_threshold = 0;
		x_data_bak = x;
		y_data_bak = y;
		z_data_bak = z;
	}
	else
	{
		if ((abs(x - x_data_bak)* MCONVERT_PARA > DETECT_THRESHOLD) ||
			(abs(y - y_data_bak)* MCONVERT_PARA > DETECT_THRESHOLD) ||
			(abs(z - z_data_bak)* MCONVERT_PARA > DETECT_THRESHOLD))
		{
			cnt_exceed_threshold++;
			fs_mem_log("gsensor previous data, x = %d , y = %d, z = %d\n", -x * MCONVERT_PARA, -y * MCONVERT_PARA, -z * MCONVERT_PARA);
			fs_mem_log("gsensor current  data, x = %d , y = %d, z = %d\n", -x_data_bak * MCONVERT_PARA, -y_data_bak * MCONVERT_PARA, -z_data_bak * MCONVERT_PARA);
			fs_mem_log("gsensor delta    data, x = %d , y = %d, z = %d, cnt = %d\n\n",
						abs(x - x_data_bak)* MCONVERT_PARA,
						abs(y - y_data_bak)* MCONVERT_PARA,
						abs(z - z_data_bak)* MCONVERT_PARA,
						cnt_exceed_threshold);

			complete(&completion_for_notifier);
		}
		x_data_bak = x;
		y_data_bak = y;
		z_data_bak = z;
	}
	return;
}
/*
static void crash_detect_init(void)
{
	CREATE_KTHREAD(thread_notifier_func, NULL);
}
*/
