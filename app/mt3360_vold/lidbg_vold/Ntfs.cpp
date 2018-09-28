/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Ntfs.h"

#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>
#include "Fat.h"
#include "VoldUtil.h"
#include "../../inc/lidbg_servicer.h"

static char *FLY_PRODUCT_PATH = "/flysystem/lib/out/chkntfs";
static char *NATIVE_SYSTEM_PATH = "/system/lib/modules/out/chkntfs";

static char *FSCK_NTFS_PATH = "/system/bin/chkntfs";
static char *MKNTFS_PATH = "/system/bin/mkntfs";
static char *MOUNT_NTFS_PATH = "/system/bin/ntfs-3g";
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);


int Ntfs::selectPath(void){
	if(!access(FLY_PRODUCT_PATH, X_OK))
	{
		FSCK_NTFS_PATH = "/flysystem/lib/out/chkntfs";
		MKNTFS_PATH = "/flysystem/lib/out/mkntfs";
		MOUNT_NTFS_PATH = "/flysystem/lib/out/ntfs-3g";
		return 0;
	}
	else if(!access(NATIVE_SYSTEM_PATH, X_OK))
	{
		FSCK_NTFS_PATH = "/system/lib/modules/out/chkntfs";
		MKNTFS_PATH = "/system/lib/modules/out/mkntfs";
		MOUNT_NTFS_PATH = "/system/lib/modules/out/ntfs-3g";
		return 0;
	}
	else if(!access(FSCK_NTFS_PATH, X_OK))
	{
		return 0;
	}
	else 
		return -1;
}

int Ntfs::check(const char *fsPath) {
    bool rw = true;
    int pass = 1;
    int rc = 0;

   if(selectPath() != 0)
   {
   	LIDBG_PRINT("Skipping fs checks, NTFS Tools can't find!\n");
	return -1;
   }
   LIDBG_PRINT("\n\n\nxxxxxx FSCK_NTFS_PATH is %s\n",FSCK_NTFS_PATH);
    do {
        SLOGW("fsPath : %s" , fsPath);
        const char *args[5];
	int status;
        args[0] = FSCK_NTFS_PATH;
	args[1] = "--readwrite";
        args[2] = fsPath;	
       // args[1] = fsPath;
      //  args[2] = "-a";
       // args[3] = "-f";
        args[3] = NULL;

        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
        if (rc != 0) {
            SLOGE("Filesystem check failed due to logwrap error");
	    LIDBG_PRINT("Filesystem check failed due to logwrap error, rc = %d",rc);
            errno = EIO;
            return -1;
        }
       if (!WIFEXITED(status)) {
            SLOGE("Filesystem check did not exit properly");
	    LIDBG_PRINT("Filesystem check did not exit properly");	
            errno = EIO;
            return -1;
        }
        status = WEXITSTATUS(status);

	LIDBG_PRINT("status is %d\n", status);
        switch(status) {
        case 0:
            SLOGI("Filesystem check completed OK");
            return 0;
        case 1:
            SLOGI("no errors were found on the %s volume" , fsPath);
            return 0;

        case 2:
            SLOGE("error were found and fixed");
            return 0;

        case 3:
            SLOGW("only minor errors were found on the %d Volume", fsPath);
            return 0;
        case 4:
            SLOGW("errors were found onthe %s but they could not be fixed" , fsPath);
            return 0;
        case 6:
            SLOGW(" %s volume is not NTFS volume" , fsPath);
	    errno = ENODATA;
            return 0;
        default:
            SLOGE("Filesystem check failed (unknown exit code %d)", rc);
	    LIDBG_PRINT("Filesystem check failed (unknown exit code %d)", rc);
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}

int Ntfs::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];
    char options[255] = {};
    const char *args[6];

    flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    // Testing/security, mount ro up to now
    flags |= MS_RDONLY;

    /*
     * Note: This is a temporary hack. If the sampling profiler is enabled,
     * we make the SD card world-writable so any process can write snapshots.
     *
     * TODO: Remove this code once we have a drop box in system_server.
     */
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sampling_profiler", value, "");
    if (value[0] == '1') {
        SLOGW("The SD card is world-writable because the"
            " 'persist.sampling_profiler' system property is set to '1'.");
        permMask = 0;
    }

    sprintf(mountData,
            "uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);

    if (!remount) {
        SLOGI("Trying to use ntfs-3g program to mount %s", fsPath);

        if (ro)
            snprintf(options, sizeof(options), "ro,%s", mountData);
        else
            snprintf(options, sizeof(options), "%s", mountData);

        args[0] = MOUNT_NTFS_PATH;
        args[1] = "-o";
        args[2] = options;
        args[3] = fsPath;
        args[4] = mountPoint;
        args[5] = NULL;
	int status;
         rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);

        if (rc == 0) {
          SLOGI("ntfs-3g executed successfully.");
        } else {
            SLOGE("Failed to execute ntfs-3g.");
        }
    } else {
        rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
    }

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        if (!remount) {
            SLOGI("Trying to use ntfs-3g program to mount %s as read-only", fsPath);

            snprintf(options, sizeof(options), "ro,%s", mountData);

            args[0] = MOUNT_NTFS_PATH;
            args[1] = "-o";
            args[2] = options;
            args[3] = fsPath;
            args[4] = mountPoint;
            args[5] = NULL;

	     int status;
             rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);

            if (rc == 0) {
                SLOGI("ntfs-3g executed successfully for read-only.");
            } else {
                SLOGE("Failed to execute ntfs-3g for read-only.");
		LIDBG_PRINT("Failed to execute ntfs-3g for read-only.");
            }
        } else {
            rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
        }
    }

    return rc;
}

int Ntfs::format(const char *fsPath, unsigned int numSectors) {
    const char *args[5];
    char strNumOfSectors[16] = {};
    int rc;

    args[0] = MKNTFS_PATH;
    args[1] = "-f";
    args[2] = fsPath;

    if (numSectors) {
        snprintf(strNumOfSectors, sizeof(strNumOfSectors), "%u", numSectors);
        args[3] = strNumOfSectors;
        args[4] = NULL;
	int status;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
    } else {
        args[3] = NULL;
	int status;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
    }

    if (rc == 0) {
	SLOGI("Filesystem formatted OK");
	return 0;
    } else {
	SLOGE("Format failed (unknown exit code %d)", rc);
	errno = EIO;
	return -1;
    }
    return 0;
}
