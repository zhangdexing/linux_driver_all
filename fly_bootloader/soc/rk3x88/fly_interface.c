#include "../soc.h"
#include "fly_platform.h"

int boot_flyrecovery_from_mmc()
{
    char *const boot_flyrecovery_cmd[] = {"booti", "flyrecovery"};
    do_booti(NULL, 0, ARRAY_SIZE(boot_flyrecovery_cmd), boot_flyrecovery_cmd);

    /* returns if recovery.img is bad */
    FBTERR("\nfastboot: Error: Invalid recovery img\n");
}

char *dbg_msg_en(const char *system_cmd, int dbg_msg_en)
{
    return NULL;
}
