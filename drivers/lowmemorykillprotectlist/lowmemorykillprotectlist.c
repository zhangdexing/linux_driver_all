
#include "lidbg.h"
extern char **lmk_white_list;

char *lmk_protect_list[] =
{
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
	"o.flyaudiovideo",
        "audio.bluetooth",
        "cn.flyaudio.dvr",
        "io.proxyservice",
        "ncent.wecarnavi",

	"android.policy",
	"com.sygic.aura",
	"mediaserver",
	"system_server",
	"lidbg_userver",
	"system",
	"logcat",
	NULL,
};

static int lowmemorykillprotectlist_init(void)
{
    DUMP_BUILD_TIME;
    lmk_white_list = lmk_protect_list;
    return 0;
}


module_init(lowmemorykillprotectlist_init);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("flyaudio");
