
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
int loop_count = 0;
int mypid = -1;
bool playing = false;
int boot_completed = 0;
int phone_call_state = AUDIO_MODE_INVALID;
int phone_call_state_old = AUDIO_MODE_INVALID;


static sp<IAudioPolicyService> gAudioPolicyService = 0;
char *trace_step;
#define TRACE_STEP {trace_step = (char*)__FUNCTION__;}

void recvSignal(int sig)
{
    lidbg(TAG"received signal %d ,restart!!!\n", sig);
    lidbg("trace_step=%s\n", trace_step);
    system("/flysystem/lib/out/lidbg_android_server &");
    exit(1);
}


void GetAudioPolicyService(bool dbg)
{
    TRACE_STEP;
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    if(dbg)
        lidbg( TAG"GetAudioPolicyService.in,mypid:%d\n", mypid);

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
            boot_completed = 1;
            break;
        }
        sleep(1);
    }
    return ((void *) 0);
}

#define ERROR_VALUE (-2)
#define DEFAULT_MUSIC_LEVEL_PERCENT (60)

bool ring = false;
bool ring_old = false;
bool recheck_navi_policy = false;
int first_mute_stream_id = -1;
bool navi_policy_en = false;
int max_volume = -1;
int  sound_on = 0;
int count_num = 0;

int music_level ;
int music_max_level = 15;
int delay_off_volume_policy_cnt = 0;
int delay_off_volume_policy = 1000;//ms
int delay_step_on_music = 500;//ms

static char old_debug_cmd_value[PROPERTY_VALUE_MAX];

//refer to MAX_STREAM_VOLUME in AudioService.java (base\services\core\java\com\android\server\audio)
int max_stream_volume[AUDIO_STREAM_CNT] =
{
};

static sp<IAudioFlinger> gAudioFlingerService = 0;
void GetAudioFlingerService(bool dbg)
{
    TRACE_STEP;
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

int get_stream_volume(bool dbg, audio_stream_type_t stream_type)
{
    TRACE_STEP;
    int index;
    audio_devices_t device;
    status_t status ;
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    if(gAudioPolicyService == 0)
    {
        lidbg(TAG "get_stream_volume:gAudioPolicyService == 0\n");
        return -1;
    }
    device = aps->getDevicesForStream(stream_type) ;
    status = aps->getStreamVolumeIndex(stream_type, &index, AUDIO_DEVICE_OUT_DEFAULT);
    if(0 != device && NO_ERROR == status )
    {
        if(dbg)
            lidbg(TAG"getStreamVolumeIndex.success:index-%d/%d\n", stream_type, index);
    }
    else
    {
        if(dbg)
            lidbg(TAG"getStreamVolumeIndex.error:status-%d/%d  device:%d \n",  stream_type, status, device);
        index = -1 ;
    }
    return index;
}

int get_max_volume(bool dbg)
{
    TRACE_STEP;
    int volume;
    for (int i = 0; i < AUDIO_STREAM_CNT; i++)
    {
        volume = get_stream_volume(dbg, (audio_stream_type_t)i);
        if(volume > max_volume)
            max_volume = volume;
    }
    if(dbg)
        lidbg(TAG"get_max_volume:%d\n", max_volume);
    if(max_volume < 15)
        max_volume = 15;
    return max_volume;
}

void print_stream_volume(void)
{
    TRACE_STEP;
    for (int i = 0; i < AUDIO_STREAM_CNT; i++)
    {
        get_stream_volume(true, (audio_stream_type_t)i);
    }
}
bool set_stream_volume(audio_stream_type_t mstream, int index)
{
    TRACE_STEP;
    int error = ERROR_VALUE;
    audio_devices_t device;
    status_t status ;
    sp<IAudioPolicyService> &aps = gAudioPolicyService;
    device = aps->getDevicesForStream(mstream) ;
    //if(index > max_stream_volume[mstream])
    //    index = max_stream_volume[mstream];
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
    TRACE_STEP;
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
    TRACE_STEP;
    int i, error = ERROR_VALUE, old_index = 0;
    int cnt = abs(end_index - start_index), cur_index = start_index, step_delay;
    step_delay = (total_time / cnt);
    lidbg(TAG"step_on_stream_volume:[%d/%d->%d/%d/%d]\n", mstream, start_index, end_index, step_delay, cnt);
    if(cnt != 0)
    {
        for (i = 0; i < cnt; i++)
        {
            if(end_index > start_index)
            {
                if (abs(end_index - cur_index) > 6)
                {
                    cur_index +=  3;
                    if(dbg_volume)
                        lidbg(TAG"newstep:3/%d /%d/[%d,%d->%d]\n", cnt, 6, cur_index, start_index, end_index);
                }
                else if (abs(end_index - cur_index) > 3)
                {
                    cur_index +=  2;
                    if(dbg_volume)
                        lidbg(TAG"newstep:2/%d /%d/[%d,%d->%d]\n", cnt, 3, cur_index, start_index, end_index);
                }
                else
                {
                    cur_index += 1;
                    if(dbg_volume)
                        lidbg(TAG"newstep:1/%d /%d/[%d,%d->%d]\n", cnt, 1 , cur_index, start_index, end_index);
                }
                usleep(200 * 1000);
                if(cur_index > end_index)
                    break;
            }
            else
            {
                cur_index = start_index - i;
                usleep( step_delay * 1000);
            }

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
int check_and_restore_volume(const char *who, int dbg, int minlevel)
{
    if(dbg)
        lidbg(TAG"check_and_restore_volume:[%s]\n", who);
    if( get_stream_volume(false, AUDIO_STREAM_MUSIC) < minlevel)
    {
        print_stream_volume();
        lidbg(TAG"check_and_restore_volume:%d\n", music_max_level);
        if(music_max_level < 15)
            music_max_level = 15;
        set_all_stream_volume(music_max_level);
    }
    return 1;
}

void check_ring_stream(void)
{
    TRACE_STEP;
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
        lidbg(TAG"ring.[%d],music_level:[%d/%d],mypid:%d\n", ring, music_level, max_volume, mypid);
        if(ring)
        {
            if(delay_off_volume_policy_cnt == 0)
                step_on_stream_volume(AUDIO_STREAM_MUSIC, max_volume, music_level, 100);
            //set_stream_volume(AUDIO_STREAM_MUSIC, music_level);
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
                //set_all_stream_volume(max_volume);
                step_on_stream_volume(AUDIO_STREAM_MUSIC, music_level, music_max_level, delay_step_on_music);
            }
        }
    }

}
static void *thread_check_ring_stream(void *data)
{
    static int loop = 0;
    TRACE_STEP;
    lidbg( TAG"thread_check_ring_stream:in\n");
    data = data;
    while(1)
    {
        if(gAudioPolicyService != 0 && gAudioFlingerService != 0)
        {
            if(navi_policy_en && max_volume > 0)
            {
                if(recheck_navi_policy)
                {
                    recheck_navi_policy = false;
                    ring_old = false;
                    lidbg(TAG"check_ring_stream\n");
                }
                check_ring_stream();
            }
        }
        else
        {
            lidbg( TAG"thread_check_ring_stream:gAudioPolicyService == 0 gAudioFlingerService==0\n");
            GetAudioPolicyService(true);
            GetAudioFlingerService(true);
            sleep(3);
        }
        usleep(100000);//100ms

        loop++;
        if(!ring && delay_off_volume_policy_cnt == 0 && (!(loop % 30)))
            check_and_restore_volume("checkstep1:", false, 15);
    }
    return ((void *) 0);
}

void init_para(bool dbg)
{
    TRACE_STEP;
    max_volume = get_max_volume(dbg);
    music_level = max_volume * DEFAULT_MUSIC_LEVEL_PERCENT / 100;
    music_max_level = max_volume ;
}
void print_para(void)
{
    TRACE_STEP;
    lidbg(TAG"mypid:%d,delay_step_on_music:[%dms],delay_off_volume_policy:[%dms],music_max_level:%d,music_level:%d,navi_policy_en:[%d],max_volume[%d]count_num[%d]\n",
          mypid, delay_step_on_music, delay_off_volume_policy, music_max_level, music_level, navi_policy_en, max_volume, count_num);
}
int handle_sound_state(const char *who, int sound)
{
    TRACE_STEP;
    char cmd[16];
    sprintf(cmd, "sound %d", (sound > 0 ? 1 : 2));
    LIDBG_WRITE("/dev/fly_sound0", cmd);
    lidbg(TAG"handle_sound_state:[%s/%s/%d/%d]\n", cmd, who, playing, phone_call_state);
    return 1;
}

int main(int argc, char **argv)
{
    pthread_t ntid;
    argc = argc;
    argv = argv;
    bool first_boot = 0;
    lidbg(TAG"main\n");

    //add all sig 
    for (int i = 1; i < SIGUSR2; i++)
        signal(i, recvSignal);

    pthread_create(&ntid, NULL, thread_check_boot_complete, NULL);
    while(boot_completed == 0)
    {
        first_boot = 1;
        lidbg(TAG"wait\n");
        sleep(5);
    }
    if(first_boot == 1)
    {
        lidbg(TAG"sleep5s\n");    //ensure audioserver.java to init para.
        sleep(5);
    }

    GetAudioPolicyService(true);
    GetAudioFlingerService(true);

    mypid = getpid();
    lidbg(TAG"start:navi_policy_en=%d  max_volume=%d,mypid:%d\n", navi_policy_en, max_volume, mypid);

    pthread_create(&ntid, NULL, thread_check_ring_stream, NULL);//double check:it must be in running state.

    //sync:some platform as mtk,not the value [AUDIO_MODE_INVALID],we should sync it first.
    if(gAudioPolicyService != 0)
    {
        sp<IAudioPolicyService> &aps = gAudioPolicyService;
#ifdef ANDROID_AT_LEAST_50
        phone_call_state = aps->getPhoneState();
#endif
        if(phone_call_state < AUDIO_MODE_IN_CALL)
            phone_call_state_old = phone_call_state;
    }

    if(0)//test cpu useage
    {
        while(1)
        {
            usleep(100000);//100ms
            loop_count++;
            if(loop_count > 100)
            {
                loop_count = 0;
                print_para();
            }
        }
    }

    while(1)
    {
        trace_step = (char *)"main while1";
        if(gAudioPolicyService != 0 && gAudioFlingerService != 0)
        {
            sp<IAudioPolicyService> &aps = gAudioPolicyService;

#ifdef ANDROID_AT_LEAST_50
            phone_call_state = aps->getPhoneState();
#endif
            playing = aps->isStreamActive(AUDIO_STREAM_VOICE_CALL, 0);
            count_num = 1;
            if(1)
            {
                for (int i = 1; i < AUDIO_STREAM_CNT; i++)
                {
                    if((i >= AUDIO_STREAM_BLUETOOTH_SCO && i <= AUDIO_STREAM_TTS && i != AUDIO_STREAM_DTMF) || (i >= AUDIO_STREAM_ACCESSIBILITY && i <= AUDIO_STREAM_CNT))
                        continue;
                    count_num++;
                    if( get_stream_volume(false, (audio_stream_type_t)i) == 0)
                    {
                        playing |=  false;
                        if(i > 0 && first_mute_stream_id == -1)
                        {
                            first_mute_stream_id = i;
                            lidbg(TAG"first_mute_stream_id:%d\n", first_mute_stream_id);
                        }
                    }
                    else
                        playing = aps->isStreamActive((audio_stream_type_t)i, 0) | playing;

                    trace_step = (char *)"main while2";

                    //audiomanager.java (setstreammute API ) will broke the navi policy logic,so I should restart it again when there is a mute event happend.
                    if(first_mute_stream_id > 0)
                    {
                        if( get_stream_volume(false, (audio_stream_type_t)first_mute_stream_id) > 0)
                        {
                            lidbg(TAG"first_mute_stream_id:%d.unmute\n", first_mute_stream_id);
                            first_mute_stream_id = -1;
                            recheck_navi_policy = true;
                        }
                    }
                }
            }
            else
            {
                //save old stable method
                playing = aps->isStreamActive(AUDIO_STREAM_VOICE_CALL, 0) |
                          aps->isStreamActive(AUDIO_STREAM_RING, 0) |
                          aps->isStreamActive(AUDIO_STREAM_SYSTEM , 0) |
                          aps->isStreamActive(AUDIO_STREAM_DTMF, 0) |
                          aps->isStreamActive(AUDIO_STREAM_NOTIFICATION , 0);

                if(get_stream_volume(dbg_music, AUDIO_STREAM_MUSIC) > 0)
                    playing = aps->isStreamActive(AUDIO_STREAM_MUSIC, 0) | playing;
            }

            if(dbg_music)
                lidbg(TAG"playing=%d/phone_call_state=%d\n", playing, phone_call_state);
        }
        else
        {
            lidbg( TAG"gAudioPolicyService == 0 gAudioFlingerService==0\n");
            GetAudioPolicyService(true);
            GetAudioFlingerService(true);
        }
        trace_step = (char *)"main while3";

        if(phone_call_state != phone_call_state_old)
        {
            char cmd[32];
            phone_call_state_old = phone_call_state;
            sprintf(cmd, "phoneCallState %d", phone_call_state);
            LIDBG_WRITE("/dev/fly_sound0", cmd);
        }
        if( (playing || phone_call_state >= AUDIO_MODE_RINGTONE))
        {
            if(sound_on == 0)
            {
                sound_on = 1;
                handle_sound_state("vote", sound_on);
            }
        }
        else
        {
            if(sound_on == 1)
            {
                sound_on = 0;
                handle_sound_state("vote", sound_on);
            }
        }

        loop_count++;
        if(loop_count > 200)
        {
            GetAudioPolicyService(false);
            GetAudioFlingerService(false);
            loop_count = 0;
        }
        trace_step = (char *)"main while4";
        if(!(loop_count % 30))//per 3s
        {
            char value[PROPERTY_VALUE_MAX];
            property_get("persist.lidbg.sound.dbg", value, "n");
            if((value[0] != 'n') && strncmp(old_debug_cmd_value, value, PROPERTY_VALUE_MAX) != 0) //not equ
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
                    lidbg(TAG"init_para,add check\n");
                    init_para(true);
                    print_para();

                    if(navi_policy_en == false)
                    {
                        lidbg(TAG"restore music to max:[%d]\n", max_volume);
                        set_all_stream_volume(max_volume);
                    }
#if defined(VENDOR_MTK)
                    lidbg(TAG"force disable navi_policy_en\n");
                    navi_policy_en = false;
#endif
                    if(navi_policy_en == true)//skip set VENDOR_MTK
                    {
                        lidbg(TAG"init all Stream to max:[%d]\n", max_volume);
                        set_all_stream_volume(max_volume);
                    }
                    print_stream_volume();
                    print_para();
                    break;
                case 4 :
                    dbg_volume = true;
                    set_stream_volume((audio_stream_type_t)para[1], para[2]);
                    break;
                case 5 :
                    dbg_volume = true;
                    set_all_stream_volume(para[1]);
                    break;
                case 6 :
                    dbg_volume = true;
                    music_level = (max_volume * para[1] / 100);
                    lidbg(TAG"music_level:[%d/%d/%d]\n", music_level, para[1], max_volume);
                    break;
                case 7 :
                    dbg_volume = true;
                    music_max_level = (max_volume * para[1] / 100);
                    lidbg(TAG"music_max_level:[%d/%d/%d]\n", music_max_level, para[1], max_volume);
                    break;
                case 8 :
                    delay_off_volume_policy = para[1] ;
                    lidbg(TAG"delay_off_volume_policy:[%dms]\n", delay_off_volume_policy);
                    break;
                case 9 :
                    delay_step_on_music = para[1] ;
                    lidbg(TAG"delay_step_on_music:[%dms]\n", delay_step_on_music);
                    break;
                case 10 :
                    print_para();
                    break;
                case 11 :
                    print_stream_volume();
                    print_para();
                    break;
                case 13 :
                    navi_policy_en = (para[1] == 1);
                    if(navi_policy_en == 0)
                    {
                        lidbg(TAG"restore music to max:[%d]\n", max_volume);
                        set_all_stream_volume(max_volume);
                    }
                    print_para();
                    break;
                case 14 :
                    lidbg(TAG"do crash\n");
                    {
                        int *s = 0;
                        (*s) = 1;
                    }
                    break;
                default :
                    break;
                }
            }
        }
        usleep(100000);//100ms
    }

}

