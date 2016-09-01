
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

bool dbg_music = false;
bool dbg_volume = false;
bool navi_policy_en = true;
bool playing_old = false;
int loop_count = 0;

static sp<IAudioPolicyService> gAudioPolicyService = 0;

void GetAudioPolicyService(bool dbg)
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    if(dbg)
        lidbg( TAG"GetAudioPolicyService.in\n");

    do
    {
        binder = sm->getService(String16("media.audio_policy"));
        if (binder != 0)
            break;
        lidbg(TAG" waiting...\n");
        usleep(1000000);
    }
    while (true);
    gAudioPolicyService = interface_cast<IAudioPolicyService>(binder);
    if(gAudioPolicyService == 0)
        lidbg(TAG "GetAudioPolicyService.fail1\n");
    else if(dbg)
        lidbg( TAG"GetAudioPolicyService.succes1\n");
}

bool playing = false;
int boot_completed = 0;
static void *thread_check_boot_complete(void *data)
{
    data = data;
    while(1)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("sys.boot_completed", value, "0");
        if (value[0] == '1')
        {
            lidbg( TAG" send message  :boot_completed = %c,delay 2S \n", value[0]);
            sleep(2);
            LIDBG_WRITE("/dev/lidbg_interface", "BOOT_COMPLETED");
            boot_completed = 1;
            break;
        }
        sleep(1);
    }
    return ((void *) 0);
}

#define ERROR_VALUE (-2)
bool ring = false;
bool ring_old = false;
#define DEFAULT_MAX_VOLUME (15)
//refer to MAX_STREAM_VOLUME in AudioService.java (base\services\core\java\com\android\server\audio)
int max_stream_volume[AUDIO_STREAM_CNT] =
{
    DEFAULT_MAX_VOLUME,  // STREAM_VOICE_CALL
    DEFAULT_MAX_VOLUME,  // STREAM_SYSTEM
    DEFAULT_MAX_VOLUME,  // STREAM_RING
    DEFAULT_MAX_VOLUME, // STREAM_MUSIC
    DEFAULT_MAX_VOLUME,  // STREAM_ALARM
    DEFAULT_MAX_VOLUME,  // STREAM_NOTIFICATION
    DEFAULT_MAX_VOLUME, // STREAM_BLUETOOTH_SCO
    DEFAULT_MAX_VOLUME,  // STREAM_SYSTEM_ENFORCED
    DEFAULT_MAX_VOLUME, // STREAM_DTMF
    DEFAULT_MAX_VOLUME  // STREAM_TTS
};
int music_level = DEFAULT_MAX_VOLUME * 70 / 100;
static sp<IAudioFlinger> gAudioFlingerService = 0;
void GetAudioFlingerService(bool dbg)
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    if(dbg)
        lidbg( TAG"GetAudioFlingerService.in\n");

    do
    {
        binder = sm->getService(String16("media.audio_flinger"));
        if (binder != 0)
            break;
        lidbg(TAG" waiting...\n");
        usleep(1000000);
    }
    while (true);
    gAudioFlingerService = interface_cast<IAudioFlinger>(binder);
    if(gAudioFlingerService == 0)
        lidbg(TAG "GetAudioFlingerService.fail1\n");
    else if(dbg)
        lidbg( TAG"GetAudioFlingerService.succes1\n");
}

bool print_stream_volume(void)
{
    int i, error = ERROR_VALUE, index;
    audio_devices_t device;
    status_t status ;
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    lidbg(TAG"getStreamVolumeIndex.in,%d\n", AUDIO_STREAM_CNT);
    for (i = 0; i < AUDIO_STREAM_CNT; i++)
    {
        device = aps->getDevicesForStream((audio_stream_type_t)i) ;
        status = aps->getStreamVolumeIndex((audio_stream_type_t)i, &index, AUDIO_DEVICE_OUT_DEFAULT);
        if(0 != device && NO_ERROR == status )
        {
            lidbg(TAG"getStreamVolumeIndex.success:index-%d/%d\n", i, index);
            error = 0;
        }
        else
        {
            lidbg(TAG"getStreamVolumeIndex.error:device:%d  status-%d/%d\n", device, i, status);
        }
    }
    error = 0;
    return (error != ERROR_VALUE);
}
bool set_stream_volume(audio_stream_type_t mstream, int index)
{
    int error = ERROR_VALUE;
    audio_devices_t device;
    status_t status ;
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    device = aps->getDevicesForStream(mstream) ;
    if(index > max_stream_volume[mstream])
        index = max_stream_volume[mstream];
    status = aps->setStreamVolumeIndex(mstream, index , device);
    if(0 != device && NO_ERROR == status )
    {
        if(dbg_volume)
            lidbg(TAG"set_stream_volume.success.mstream.%d device.%d index.%d,status:%d\n", mstream, device, index, status);
    }
    else
    {
        if(dbg_volume)
            lidbg(TAG"set_stream_volume.error.mstream.%d device.%d index.%d,status:%d\n", mstream, device, index, status);
    }
    return (error == NO_ERROR);
}
bool set_all_stream_volume(int index)
{
    int i, error = ERROR_VALUE;
    if(dbg_volume)
        lidbg(TAG"set_all_stream_volume.in,CNT:%d index:%d\n", AUDIO_STREAM_CNT, index);
    for (i = 0; i < AUDIO_STREAM_CNT; i++)
    {
        set_stream_volume((audio_stream_type_t)i, index);
    }
    error = 0;
    return (error != ERROR_VALUE);
}

void check_ring_stream(void)
{
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    ring = //aps->isStreamActive(AUDIO_STREAM_RING, 0) |
        aps->isStreamActive(AUDIO_STREAM_NOTIFICATION, 0) ;
    if(ring != ring_old)
    {
        ring_old = ring;
        lidbg(TAG"ring.[%d],music_level:[%d/%d]\n", ring, music_level, DEFAULT_MAX_VOLUME);
        if(ring)
        {
            set_stream_volume(AUDIO_STREAM_MUSIC, music_level);
        }
        else
        {
            set_all_stream_volume(DEFAULT_MAX_VOLUME);
        }
    }
}


int main(int argc, char **argv)
{
    pthread_t ntid;
    argc = argc;
    argv = argv;
    lidbg(TAG"main\n");
    sleep(20);

    pthread_create(&ntid, NULL, thread_check_boot_complete, NULL);
    while(boot_completed == 0)
    {
        lidbg(TAG"wait\n");
        sleep(5);
    }
    lidbg(TAG"start\n");

    GetAudioPolicyService(true);
    GetAudioFlingerService(true);
    if(gAudioPolicyService != 0)
        set_all_stream_volume(DEFAULT_MAX_VOLUME);
    while(1)
    {
        if(gAudioPolicyService != 0 && gAudioFlingerService != 0)
        {
            sp<IAudioPolicyService> &aps = gAudioPolicyService;
            playing = aps->isStreamActive((audio_stream_type_t)3, 0) |
                      aps->isStreamActive((audio_stream_type_t)0, 0) |
                      aps->isStreamActive((audio_stream_type_t)2, 0) |
                      aps->isStreamActive((audio_stream_type_t)1, 0) |
                      aps->isStreamActive((audio_stream_type_t)5, 0);
            if(dbg_music)
                lidbg(TAG"playing=%d\n", playing);
            if(navi_policy_en)
                check_ring_stream();
        }
        else
        {
            lidbg( TAG"gAudioPolicyService == 0 gAudioFlingerService==0\n");
            GetAudioPolicyService(true);
            GetAudioFlingerService(true);
        }

        if(playing != playing_old)
        {
            char cmd[16];
            playing_old = playing;
            sprintf(cmd, "sound %d", playing);
            LIDBG_WRITE("/dev/fly_sound0", cmd);
            if(dbg_music)
                lidbg(TAG"write.[%d,%s]\n", playing, cmd);
        }
        loop_count++;
        if(loop_count > 200)
        {
            GetAudioPolicyService(false);
            GetAudioFlingerService(false);
            loop_count = 0;
        }
        if(!(loop_count % 20))//per 2 s
        {
            char value[PROPERTY_VALUE_MAX];
            property_get("persist.lidbg.sound.dbg", value, "n");
            if(value[0] != 'n')
            {
                int para[5] = {0}, i = 0;
                char *token = strtok( value, " ");
                while( token != NULL )
                {
                    para[i] = atoi(token);
                    lidbg(TAG"para:[%d,%d]\n", i , para[i]);
                    token = strtok( NULL, " ");
                    i++;
                }

                switch (para[0])
                {
                case 1 :
                    dbg_music = !dbg_music;
                    lidbg(TAG"dbg_music:[%d]\n", dbg_music);
                    break;
                case 2 :
                    dbg_volume = !dbg_volume;
                    lidbg(TAG"dbg_volume:[%d]\n", dbg_volume);
                    break;
                case 3 :
                    navi_policy_en = (para[1] == 1);
                    lidbg(TAG"navi_policy_en:[%d]\n", navi_policy_en);
                    break;
                case 4 :
                    dbg_volume = true;
                    print_stream_volume();
                    set_stream_volume((audio_stream_type_t)para[1], para[2]);
                    break;
                case 5 :
                    dbg_volume = true;
                    set_all_stream_volume(para[1]);
                    break;
                case 6 :
                    dbg_volume = true;
                    music_level = (DEFAULT_MAX_VOLUME * para[1] / 100);
                    lidbg(TAG"music_level:[%d/%d/%d]\n", music_level, para[1], DEFAULT_MAX_VOLUME);
                    break;
                default :
                    break;
                }
                property_set("persist.lidbg.sound.dbg", "n");
            }
        }
        usleep(100000);//100ms
    }

}

