
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
#if defined(VENDOR_MTK)
bool navi_policy_en = false;
#define DEFAULT_MAX_VOLUME (100)
#else
bool navi_policy_en = true;
#define DEFAULT_MAX_VOLUME (15)
#endif

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
int music_level = DEFAULT_MAX_VOLUME * 50 / 100;
int music_max_level = DEFAULT_MAX_VOLUME ;
int delay_off_volume_policy_cnt = 0;
int delay_off_volume_policy = 2000;//ms
int delay_step_on_music = 500;//ms

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
bool step_on_stream_volume(audio_stream_type_t mstream, int start_index , int end_index, int total_time)
{
    int i, error = ERROR_VALUE, old_index = 0;
    int cnt = abs(end_index - start_index), cur_index, step_delay;
    step_delay = (total_time / cnt);
    lidbg(TAG"step_on_stream_volume:[%d/%d/%d/%d/%d]\n", mstream, start_index, end_index, step_delay, cnt);
    if(cnt != 0)
    {
        for (i = 0; i < cnt; i++)
        {
            if(end_index > start_index)
                cur_index = start_index + i;
            else
                cur_index = start_index - i;
            usleep( step_delay * 1000);

            if(cur_index != old_index)
            {
                old_index = cur_index;
                set_stream_volume(mstream, cur_index);
            }
        }
    }
    set_stream_volume(mstream, end_index);
    return (error == NO_ERROR);
}

void check_ring_stream(void)
{
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    ring = //aps->isStreamActive(AUDIO_STREAM_RING, 0) |
        aps->isStreamActive(AUDIO_STREAM_NOTIFICATION, 0) ;

    if(ring != ring_old)
    {
        ring_old = ring;
        if(0)
        {
            char cmd[16];
            sprintf(cmd, "sound %d", (ring ? 3 : 4));
            LIDBG_WRITE("/dev/fly_sound0", cmd);
        }
        lidbg(TAG"ring.[%d],music_level:[%d/%d]\n", ring, music_level, DEFAULT_MAX_VOLUME);
        if(ring)
        {
            //step_on_stream_volume(AUDIO_STREAM_MUSIC, DEFAULT_MAX_VOLUME, music_level, 200);
            set_stream_volume(AUDIO_STREAM_MUSIC, music_level);
        }
        else
        {
            if(dbg_volume)
                lidbg(TAG"start delay_off_volume_policy_cnt\n");
            delay_off_volume_policy_cnt = 1;
        }
    }

    if(delay_off_volume_policy_cnt > 0)
        delay_off_volume_policy_cnt++;
    if(delay_off_volume_policy_cnt * 100 > delay_off_volume_policy)
    {
        if(ring)
        {
            if(dbg_volume)
                lidbg(TAG"restart delay_off_volume_policy_cnt\n");
            delay_off_volume_policy_cnt = 1;
        }
        else
        {
            if(dbg_volume)
                lidbg(TAG"stop delay_off_volume_policy_cnt\n");
            delay_off_volume_policy_cnt = 0;
            {
                //set_all_stream_volume(DEFAULT_MAX_VOLUME);
                step_on_stream_volume(AUDIO_STREAM_MUSIC, music_level, music_max_level, delay_step_on_music);
            }
        }
    }

}
static void *thread_check_ring_stream(void *data)
{
    lidbg( TAG"thread_check_ring_stream:in\n");
    data = data;
    while(1)
    {
        if(gAudioPolicyService != 0 && gAudioFlingerService != 0)
        {
            if(navi_policy_en)
                check_ring_stream();
        }
        else
        {
            lidbg( TAG"thread_check_ring_stream:gAudioPolicyService == 0 gAudioFlingerService==0\n");
            GetAudioPolicyService(true);
            GetAudioFlingerService(true);
            sleep(3);
        }
        usleep(100000);//100ms
    }
    return ((void *) 0);
}

static char old_debug_cmd_value[PROPERTY_VALUE_MAX];
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
    lidbg(TAG"start:navi_policy_en=%d  DEFAULT_MAX_VOLUME=%d\n", navi_policy_en, DEFAULT_MAX_VOLUME);

    GetAudioPolicyService(true);
    GetAudioFlingerService(true);
    if(gAudioPolicyService != 0 && navi_policy_en)
        set_all_stream_volume(DEFAULT_MAX_VOLUME);
    pthread_create(&ntid, NULL, thread_check_ring_stream, NULL);
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
            if(10 < AUDIO_STREAM_CNT)//for MT3561 AUDIO_STREAM_GIS
                playing = aps->isStreamActive((audio_stream_type_t)10, 0) | playing;

            if(dbg_music)
                lidbg(TAG"playing=%d\n", playing);
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
            sprintf(cmd, "sound %d", (playing ? 1 : 2));
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
            if(strncmp(old_debug_cmd_value, value, PROPERTY_VALUE_MAX) != 0) //not equ
            {
                int para[5] = {0}, i = 0;
                char *token;

                strcpy(old_debug_cmd_value, value);
                lidbg(TAG"parse cmd:[%s]\n", value);
                token = strtok( value, " ");
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
                case 7 :
                    dbg_volume = true;
                    music_max_level = (DEFAULT_MAX_VOLUME * para[1] / 100);
                    lidbg(TAG"music_max_level:[%d/%d/%d]\n", music_max_level, para[1], DEFAULT_MAX_VOLUME);
                    break;
                case 8 :
                    delay_off_volume_policy = para[1] ;
                    lidbg(TAG"delay_off_volume_policy:[%dms]\n", delay_off_volume_policy);
                    break;
                case 9 :
                    delay_step_on_music = para[1] ;
                    lidbg(TAG"delay_step_on_music:[%dms]\n", delay_step_on_music);
                    break;

                default :
                    break;
                }
            }
        }
        usleep(100000);//100ms
    }

}

