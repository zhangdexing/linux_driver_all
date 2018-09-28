
#ifndef _LIDBG_VOLD_H
#define _LIDBG_VOLD_H

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/param.h>

#include <linux/kdev_t.h>

#include <cutils/properties.h>

#include <diskconfig/diskconfig.h>

#include <private/android_filesystem_config.h>

#define LOG_TAG "Vold"

#include <cutils/fs.h>
#include <cutils/log.h>

#include <string>





#include <unistd.h>
#include "../inc/lidbg_servicer.h"

class Lidbg_vold {


public:
    static const int MAX_PARTITIONS = 16;
    static const int MAX_UNMOUNT_PARTITIONS = 256;
    int mMountedPartNum;


    char *mFuseMountPart[MAX_PARTITIONS];
    char *mMountPart[MAX_PARTITIONS];		   
    char *mUnMountPart[MAX_UNMOUNT_PARTITIONS]; 


    int deleteDeviceNode(const char *path);
    char*  createMountPoint(const char *path, int major, int minor);
    int deleteMountPoint(char* mountpoint);
    void saveUnmountPoint(char* mountpoint);
    void deleteUnMountPoint(int clear);

    void stopFuse(void);
    void startFuse(void);

};

#endif

