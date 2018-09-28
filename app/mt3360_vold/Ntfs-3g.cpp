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

#include "Ntfs-3g.h"

static char MOUNT_NTFS_PATH[] = "/system/bin/ntfs-3g";
extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);
int Ntfs_3g::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) 
{
    int rc = 0;
    unsigned long flags;
    char mountData[255];
	char options[255] = {};
	const char *args[6];

   	flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    /*
     * Note: This is a temporary hack. If the sampling profiler is enabled,
     * we make the SD card world-writable so any process can write snapshots.
     *
     * TODO: Remove this code once we have a drop box in system_server.
     */
    char value[PROPERTY_VALUE_MAX];
	property_set("device.node", fsPath);
    property_get("persist.sampling_profiler", value, "");
    if (value[0] == '1') {
        SLOGW("The SD card is world-writable because the"
            " 'persist.sampling_profiler' system property is set to '1'.");
        permMask = 0;
    }

	sprintf(mountData,
            "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
            ownerUid, ownerGid, permMask, permMask);

	if(!remount){
		SLOGI("Trying to use ntfs-3g program to mount %s\n", fsPath);

		if(ro)
			snprintf(options, sizeof(options), "ro,%s", mountData);
		else
			snprintf(options, sizeof(options), "%s", mountData);

		args[0] = MOUNT_NTFS_PATH;
		args[1] = "-o";
		args[2] = options;
		args[3] = fsPath;
		args[4] = mountPoint;
		args[5] = NULL;

		rc = logwrap(6, args, 1);

		if(rc == 0){
			SLOGI("ntfs-3g executed success.\n");
		}else{
			SLOGI("Failed to execute ntfs-3g.\n");
		}
	}else{
		rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
	}

	if(rc && errno == EROFS){
		SLOGE("%s appears to be a read only filesystem - retrying mount R0.\n", fsPath);
		flags |= MS_RDONLY;
		if(!remount){
			SLOGI("Trying to use ntfs-3g program to mount %s as read-only", fsPath);

			snprintf(options, sizeof(options), "ro,%s", mountData);

			args[0] = MOUNT_NTFS_PATH;
			args[1] = "-o";
			args[2] = options;
			args[3] = fsPath;
			args[4] = mountPoint;
			args[5] = NULL;

			rc = logwrap(6, args, 1);
			if(rc == 0){
				SLOGE("ntfs-3g executed success for read-only.\n");
			}else{
				SLOGE("Failed to execute ntfs-3g for read-only.\n");
			}
		}else{
			rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
		}
	}
    return rc;
}

