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

#include "Vfs.h"
#include "../inc/lidbg_servicer.h"

static char FSCK_MSDOS_PATH[] = "/system/bin/fsck_msdos";
static char MKDOSFS_PATH[] = "/system/bin/newfs_msdos";
extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

static bool isFatFileSys(unsigned char *pfsystem)
{	
	if(   *(pfsystem) == 0x4D
		&& *(pfsystem+1) == 0x53
		&& *(pfsystem+2) == 0x44
		&& *(pfsystem+3) == 0x4F
		&& *(pfsystem+4) == 0x53
		&& *(pfsystem+5) == 0x35
		&& *(pfsystem+6) == 0x2E
		&& *(pfsystem+7) == 0x30){
		
			return true;
		}
	return false;
}

static bool isExfatFileSys(unsigned char *pfsystem)
{
	if(   *(pfsystem) == 0x45
		&& *(pfsystem+1) == 0x58
		&& *(pfsystem+2) == 0x46
		&& *(pfsystem+3) == 0x41
		&& *(pfsystem+4) == 0x54
		&& *(pfsystem+5) == 0x20
		&& *(pfsystem+6) == 0x20
		&& *(pfsystem+7) == 0x20){
		
			return true;
		}
	return false;
}

static bool isNtfsFileSys(unsigned char *pfsystem)
{
	if(	   *pfsystem == 0x4E  
		&& *(pfsystem+1) == 0x54
		&& *(pfsystem+2) == 0x46
		&& *(pfsystem+3) == 0x53
		&& *(pfsystem+4) == 0x20
		&& *(pfsystem+5) == 0x20
		&& *(pfsystem+6) == 0x20
		&& *(pfsystem+7) == 0x20){
		
		return true;	
	}
	return false;
}
int Vfs::getFileSysType(const char *fsPath){
	
	int fd,i;
	unsigned char sectorBuf[512];
	unsigned char fsystem[8]; 
	unsigned int cnt;
	if((fd = open(fsPath, O_RDONLY)) == -1){
		SLOGE("----->>> Open Disk Failed: %s <<<-----\n", fsPath);
		return -1;
	}

	cnt = read(fd, sectorBuf, 512);
	if (cnt <= 0)
	{
		SLOGE("----->>> read Disk Failed: %d <<<-----\n", cnt);
		close(fd);
		return -1;
	}
	close(fd);
	
	for(i = 0; i < 8; i++)
		fsystem[i] = sectorBuf[0x3 + i];

	if(isFatFileSys(fsystem)){
		return FILE_SYS_TYPE_FAT32;
	}
	else if(isNtfsFileSys(fsystem)){
		return FILE_SYS_TYPE_NTFS;
	}
	else if(isExfatFileSys(fsystem)){
		return FILE_SYS_TYPE_EXFAT;
	}
	else{
		SLOGE("----->>> Unknown fileSystemType <<<-----\n");
		return FILE_SYS_TYPE_FAT32;
	}
	
	return 0;
}


int Vfs::check(const char *fsPath, const int fsType)
{
	int rc = -1;
	switch(fsType){
	case FILE_SYS_TYPE_FAT32:
		rc = Fat::check(fsPath);
		LIDBG_PRINT("[flyaudio vold]: Vfs check ,fs type is FAT32\n");
		break;
	case FILE_SYS_TYPE_EXFAT:
		rc = ExFat::check(fsPath);
		LIDBG_PRINT("[flyaudio vold]: Vfs check ,fs type is EXFAT\n");
		break;
	case FILE_SYS_TYPE_NTFS:
	case FILE_SYS_TYPE_UNKNOWN:
		rc = 0;
		break;
	default:
		break;
	}
    return rc;
}

int Vfs::doMount(const char *fsPath, const char *mountPoint,
                 const int fsType, bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost)
{

	int rc = -1;

	switch(fsType){
	case FILE_SYS_TYPE_FAT32:
		rc = Fat::doMount(fsPath, mountPoint, false, false, false,
                     1015, 1015, 0702, false);
		LIDBG_PRINT("[flyaudio vold]: Vfs doMount ,fs type is FAT32\n");
		break;
	case FILE_SYS_TYPE_NTFS:
		rc = Ntfs_3g::doMount(fsPath, mountPoint, false, false, false,
                     	1015, 1015, 0702, false);
		LIDBG_PRINT("[flyaudio vold]: Vfs doMount ,fs type is NTFS\n");
		break;
	case FILE_SYS_TYPE_EXFAT:
		rc = ExFat::doMount(fsPath, mountPoint, false, false, false,
                        1015, 1015, 0702, false); 
		LIDBG_PRINT("[flyaudio vold]: Vfs doMount ,fs type is EXFAT\n");
		break;
	case FILE_SYS_TYPE_UNKNOWN:
		rc = Fat::doMount(fsPath, mountPoint, false, false, false,
                     1015, 1015, 0702, false);
		if(rc){
			rc = ExFat::doMount(fsPath, mountPoint, false, false, false,
                     		1015, 1015, 0702, false);
			if(rc){
				rc = Ntfs_3g::doMount(fsPath, mountPoint, false, false, false,
                     	1015, 1015, 0702, false);
			}
		}
		break;
	default:
		break;
	}
    return rc;
}

int Vfs::format(const char *fsPath, const int fsType, unsigned int numSectors)
{
	int rc = 0;
	switch(fsType){
	case FILE_SYS_TYPE_FAT32:
		rc = Fat::format(fsPath, numSectors);
		break;
	case FILE_SYS_TYPE_NTFS:
	case FILE_SYS_TYPE_EXFAT:
	case FILE_SYS_TYPE_UNKNOWN:
		break;
	default:
		break;
	}
    return rc;
}



