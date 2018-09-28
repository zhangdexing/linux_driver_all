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

#ifndef _NFS_H
#define _NFS_H

#include <unistd.h>
#include "Fat.h"
#include "ExFat.h"
#include "Ntfs-3g.h"

#define FILE_SYS_TYPE_UNKNOWN 0
#define FILE_SYS_TYPE_FAT32	1
#define FILE_SYS_TYPE_NTFS  2
#define	FILE_SYS_TYPE_EXFAT 3

class Vfs {
public:	
	static int getFileSysType(const char *fsPath);
    static int check(const char *fsPath, const int fsType);
    static int doMount(const char *fsPath, const char *mountPoint,
                       const int fsType, bool ro, bool remount, bool executable,
                       int ownerUid, int ownerGid, int permMask,
                       bool createLost);
    static int format(const char *fsPath, const int fsType, unsigned int numSectors);
};

#endif

