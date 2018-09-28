#include "../soc.h"
#include "fly_platform.h"

int boot_flyrecovery_from_mmc()
{
	run_command("boota mmc2 flyrecovery",0);
}

char *dbg_msg_en(const char *system_cmd, int dbg_msg_en)
{
    return NULL;
}
