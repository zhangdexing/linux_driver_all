#ifndef _LIGDBG_LOADER__
#define _LIGDBG_LOADER__

enum boot_mode
{
    MD_RECOVERY = 0,
    MD_ORIGIN,
    MD_FLYSYSTEM,
    MD_DEBUG,

};

extern int load_modules_count;
extern enum boot_mode gboot_mode;
void  lidbg_insmod(char path[]);

#define LIDBG_MODULE_LOG do{DUMP_BUILD_TIME;load_modules_count++;}while(0)

#endif

