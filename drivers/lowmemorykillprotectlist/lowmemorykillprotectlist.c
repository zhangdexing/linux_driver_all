
#include "lidbg.h"
extern char **lmk_white_list;

char *lmk_protect_list[] =
{
	".service:remote",
	"chips.bluetooth",
	"ice:pushservice",
	"o.clientservice",
	"d.process.acore",
	"ndroid.launcher",
	"d.process.media",
	"roid.flyaudioui",
	"flyaudioservice",
	".flybootservice",
	"dio.flyaudioram",
	"mediatorservice",
	"droid.launcher3",
	".flyaudio.media",
	"ndroid.systemui",
	"lyaudio.Weather",
	"oadcastreceiver",
	"c2739.mainframe",
	"alcomm.fastboot",
	"udio.navigation",
	"dio.osd.service",
	"droid.deskclock",
	"ys.DeviceHealth",
	"io.proxyservice",
	"mobile.rateflow",
	"goodocom.gocsdk",
	"tonavi.amapauto",
	"shirts.firewall",
	"irts.sslcapture",
	"le.trafficstats",
	"ftf.callmessage",
	"ample.sleeptest",
	"_android_server",
	"idbg_testuvccam",
	"kware.thinknavi",
	"ndroid.keyguard",
    "r4u.cellokeypad",
    ".mgroup.weather",
    "up.sendlocation",
    "mgroup.jenefota",
    "p.colmobilsuite",
    "om.sendlocation",
    "o.flyaudiovideo",
    "audio.bluetooth",
    "cn.flyaudio.dvr",
    "io.proxyservice",
    "rocessing.voice",
	"ncent.wecarnavi",
	"android.GpsInfo",
	"android.policy",
	"com.sygic.aura",
	"mediaserver",
	"system_server",
	"lidbg_userver",
	"system",
	"logcat",
	"com.waze",
	"baidu.car.radio",
	"codriver:remote",
	".baidu.naviauto",
	"du.che.codriver",
	"roup.wazehelper",
	"r4u.cellokeypad",
	"m.ituran.mbkusb",
	"m.mgroup.opener",
	".ecar.AppManger",
	"om.coagent.ecar",
	"m.aispeech.aios",
    "ch.aios.adapter",
	".morservice.app",
	"flyaudio.simnet",

	
	NULL,
};
void dump_list(void)
{
    int i = 0;
    while(1)
    {
        if(lmk_protect_list[i] == NULL)
            break;
        printk(KERN_CRIT"%s:%d->[%s] \n" , __func__, i, lmk_protect_list[i]);
        i++;
    }
}
static int lowmemorykillprotectlist_init(void)
{
    DUMP_BUILD_TIME;
    lmk_white_list = lmk_protect_list;
    dump_list();
    return 0;
}


module_init(lowmemorykillprotectlist_init);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("flyaudio");
