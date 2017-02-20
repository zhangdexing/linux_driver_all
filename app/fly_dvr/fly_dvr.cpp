
#include "Flydvr_Common.h"
#include "Flydvr_General.h"
#include "Flydvr_Message.h"
#include "Flydvr_Menu.h"
#include "MenuSetting.h"

int main(int argc, char *argv[])
{
	uiCheckDefaultMenuExist();
	Flydvr_Initialize();
	Flydvr_SendMessage(FLYM_UI_NOTIFICATION, 0, 0);
	while(1)
    {
        UINT32 uiMsgId, uiParam1, uiParam2;
		UINT16 usCount;
        while (1) {
			//sleep(1);//tmp for debug
			usleep(1000);
		    if (Flydvr_GetMessage( &uiMsgId, &uiParam1, &uiParam2) == FLY_FALSE) {
  			    continue;
		    }
       		break;
        }
		lidbg("=======MSG:%d,%d,%d=======\n",uiMsgId,uiParam1,uiParam2);//tmp for debug
		
		
        //AHC_OS_AcquireSem(ahc_System_Ready, 0);

		uiStateMachine(uiMsgId, uiParam1, uiParam2);

        //AHC_OS_ReleaseSem(ahc_System_Ready);
    }
	lidbg("=======EXIT=======\n");//tmp for debug
	return 0;
}