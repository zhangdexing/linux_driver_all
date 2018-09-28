/*************************************************************************
	> File Name: test.c
	> Author: lizhu
	> Mail: 1489306911@qq.com
	> Created Time: 2015?11?28? ??? 11?05?05?
 ************************************************************************/


#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include <errno.h> 

typedef struct _FLYDVR_QUEUE_MESSAGE 
{
	int ulMsgID; 
	int ulParam1;
	int ulParam2;
} FLYDVR_QUEUE_MESSAGE;

typedef enum _EventID {
	EVENT_DRV_NONE,
    	EVENT_DRV_VOLD_SD_REMOVED,
	EVENT_DRV_MAX,
}DRV_MESSAGE; 

typedef enum _FLYDVR_MESSAGE {
	FLYM_USER = 0x0,		
	FLYM_UI_NOTIFICATION,
	FLYM_DRV,
	FLYM_MAX
}FLY_MESSAGE; 

FLYDVR_QUEUE_MESSAGE msg;

int main(){
	char fd_str[255];
	msg.ulMsgID = FLYM_DRV; //fixed to 0x02
	msg.ulParam1 = EVENT_DRV_VOLD_SD_REMOVED;
	msg.ulParam2 = 0x00; //arg (useless)

	property_get("lidbg.dvrmsg.pipeFd", fd_str, "0");
	printf("write to pipe[%s]\n",fd_str);
	if(write(atoi(fd_str),&msg,sizeof(FLYDVR_QUEUE_MESSAGE)) < 0)
	{
		printf("%s: write failed,%s", __func__, strerror(errno));
	}
    return 0;
}



