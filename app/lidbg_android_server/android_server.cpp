
#include "lidbg_servicer.h"

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <binder/IInterface.h>
#include <media/AudioSystem.h>
#include <media/IAudioFlinger.h>
#include <media/IAudioPolicyService.h>
#include <math.h>
#include <system/audio.h>

#define TAG "android_server:"
using namespace android;

bool dbg = false;
int loop_count = 0;
bool playing_old = false;
bool playing = false;
int phoneCallState_old = AUDIO_MODE_IN_COMMUNICATION;
int phoneCallState = AUDIO_MODE_INVALID;
static sp<IAudioPolicyService> gAudioPolicyService = 0;


bool sendDriverState(char *type, int value)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s %d", type, value);
    lidbg(TAG"write.[%s]\n", cmd);
    LIDBG_WRITE("/dev/fly_sound0", cmd);
    return true;
}
sp<IBinder> getService(char *name)
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    if(dbg)
        lidbg( TAG"getService.in:%s\n", name);

    do
    {
        binder = sm->getService(String16(name));
        if (binder != 0)
            break;
        lidbg(TAG" waiting...[%s]\n", name);
        usleep(1000000);
    }
    while (true);
    //if(binder == 0)
    //    lidbg(TAG "getService.fail1[%s]\n",name);
    // else
    if(dbg)
        lidbg( TAG"getService.succes1[%s]\n", name);
    return binder;
}

void GetAudioPolicyService()
{
    sp<IBinder> binder = getService("media.audio_policy");
    gAudioPolicyService = interface_cast<IAudioPolicyService>(binder);
}
void handleAudioPolicyServiceEvent()
{
    if(gAudioPolicyService != 0)
    {
        sp<IAudioPolicyService> &aps = gAudioPolicyService;
        playing = aps->isStreamActive((audio_stream_type_t)3, 0) |
                  aps->isStreamActive((audio_stream_type_t)0, 0) |
                  aps->isStreamActive((audio_stream_type_t)2, 0) |
                  aps->isStreamActive((audio_stream_type_t)1, 0) |
                  aps->isStreamActive((audio_stream_type_t)5, 0);
        phoneCallState = aps->getPhoneState();
        if(dbg)
            lidbg(TAG"playing=%d,getPhoneState=%d\n", playing, phoneCallState);
    }
    else
    {
        lidbg( TAG"gAudioPolicyService == 0\n");
        return;
    }

    if(playing != playing_old)
    {
        playing_old = playing;
        sendDriverState("sound", playing);
    }
    if(phoneCallState != phoneCallState_old)
    {
        phoneCallState_old = phoneCallState;
        sendDriverState("phoneCallState", phoneCallState);
    }

}


void androidServiceInit()
{
    GetAudioPolicyService();
}
void androidServiceHandle()
{
    handleAudioPolicyServiceEvent();
}
static void *thread_check_boot_complete(void *data)
{
    while(1)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("sys.boot_completed", value, "0");
        if (value[0] == '1')
        {
            lidbg( TAG" send message  :boot_completed = %c,delay 2S \n", value[0]);
            sleep(2);
            LIDBG_WRITE("/dev/lidbg_interface", "BOOT_COMPLETED");
            break;
        }
        sleep(1);
    }
    return ((void *) 0);
}
int main(int argc, char **argv)
{
    pthread_t ntid;
    argc = argc;
    argv = argv;
    lidbg( TAG"lidbg_android_server:main.sleep(20)\n");

    sleep(20);

    androidServiceInit();
    pthread_create(&ntid, NULL, thread_check_boot_complete, NULL);
    while(1)
    {
        androidServiceHandle();
        loop_count++;
        if(loop_count > 200)//get service again per 20s.
        {
            char value[PROPERTY_VALUE_MAX];
            androidServiceInit();
            property_get("persist.lidbg.sound.dbg", value, "0");
            if (value[0] == '1')
                dbg = 1;
            else
                dbg = 0;
            loop_count = 0;
        }

        usleep(100000);//100ms
    }

}
