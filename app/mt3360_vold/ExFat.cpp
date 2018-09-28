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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "../inc/lidbg_servicer.h"

#include "ExFat.h"
#include "VoldUtil.h"

static char FSCK_EXFAT_PATH[] = "/flysystem/lib/out/fsck.exfat";
static char MK_EXFAT_PATH[] = "/flysystem/lib/out/mkfs.exfat";

extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);
int ExFat::check(const char *fsPath)
{
	SLOGI("ExFat::check");
	if (access(FSCK_EXFAT_PATH, X_OK)) {
        LIDBG_PRINT("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;

    const char *args[5];
    args[0] = FSCK_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = NULL;
    int status;
//   rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
	rc = logwrap(ARRAY_SIZE(args), args, 1);
	if( rc != 0 )
	{
       SLOGE("Filesystem check failed (unknown exit code %d)", rc);
	 LIDBG_PRINT("Filesystem check failed (unknown exit code %d)", rc);
    }
/*
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
			   
			   return -1;
	
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

*/
	return rc;
}
int ExFat::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];
    const char *args[4];

    flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    // Testing/security, mount ro up to now
    //flags |= MS_RDONLY;

    /*
     * Note: This is a temporary hack. If the sampling profiler is enabled,
     * we make the SD card world-writable so any process can write snapshots.
     *
     * TODO: Remove this code once we have a drop box in system_server.
     */
    char value[PROPERTY_VALUE_MAX];


    sprintf(mountData,
		"uid=%d,gid=%d,fmask=%o,dmask=%o",
		ownerUid, ownerGid, permMask, permMask);

    if (!remount) {
        LIDBG_PRINT("[flyaudio vold]: Trying to use exfat program to mount %s", fsPath);

		args[0] = MK_EXFAT_PATH;
		args[1] = fsPath;
		args[2] = mountPoint;
		args[3] = NULL;
	int status;
//         rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
	rc = logwrap(ARRAY_SIZE(args), args, 1);

        if (rc == 0) {
		SLOGI("mkfs.exfat executed successfully.");
		LIDBG_PRINT("[flyaudio vold]: mkfs.exfat executed successfully.");
        } else {
		SLOGE("Failed to execute mkfs.exfat.");
		LIDBG_PRINT("Failed to execute mkfs.exfat.");
        }
    } else {
        rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
    }
/*
    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        if (!remount) {
            SLOGI("Trying to use ntfs-3g program to mount %s as read-only", fsPath);

            snprintf(options, sizeof(options), "ro,%s", mountData);

            args[0] = MK_EXFAT_PATH;
            args[1] = "-o";
            args[2] = options;
            args[3] = fsPath;
            args[4] = mountPoint;
            args[5] = NULL;

	     int status;
             rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);

            if (rc == 0) {
                SLOGI("ntfs-3g executed successfully for read-only.");
		LIDBG_PRINT("exfat executed successfully for read-only.");
            } else {
                SLOGE("Failed to execute ntfs-3g for read-only.");
		LIDBG_PRINT("Failed to execute exfat for read-only.");
            }
        } else {
            rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
        }
    }*/

    return rc;
}



