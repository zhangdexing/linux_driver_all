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

#include "Exfat.h"

#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>
#include "Fat.h"
#include "VoldUtil.h"
#include "../../inc/lidbg_servicer.h"

static const char FSCK_EXFAT_PATH[] = "/system/bin/fsck.exfat";
static const char MK_EXFAT_PATH[] = "/system/bin/mkfs.exfat";


extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Exfat::check(const char *fsPath)
{
	SLOGI("Exfat::check");
	if (access(FSCK_EXFAT_PATH, X_OK)) {
        lidbg("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;

    const char *args[5];
    args[0] = FSCK_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = NULL;
    int status;
   rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);
	if( rc != 0 )
	{
       SLOGE("Filesystem check failed (unknown exit code %d)", rc);
	 lidbg("Filesystem check failed (unknown exit code %d)", rc);
    }

	if (!WIFEXITED(status)) {
			   SLOGE("Filesystem check did not exit properly");
		   lidbg("Filesystem check did not exit properly");  
			   errno = EIO;
			   return -1;
		   }
		   status = WEXITSTATUS(status);
	
	   lidbg("status is %d\n", status);
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
		   lidbg("Filesystem check failed (unknown exit code %d)", rc);
			   errno = EIO;
			   return -1;
		   }


	return rc;
}


int Exfat::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];
    const char *args[4];

    int status;
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
        lidbg("Trying to use exfat program to mount %s", fsPath);

        args[0] = MK_EXFAT_PATH;
        args[1] = fsPath;
	args[2] = mountPoint;
        args[3] = NULL;
         rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,false, true);

        if (rc == 0) {
	SLOGI("mkfs.exfat executed successfully.");
          SLOGI("[cks]<<<<<<<<<<ntfs-3g executed successfully.>>>>>>>>>>>>>");
        } else {
            SLOGE("Failed to execute ntfs-3g.");
	lidbg("Failed to execute ntfs-3g.");
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
		lidbg("exfat executed successfully for read-only.");
            } else {
                SLOGE("Failed to execute ntfs-3g for read-only.");
		lidbg("Failed to execute exfat for read-only.");
            }
        } else {
            rc = mount(fsPath, mountPoint, "fuseblk", flags, mountData);
        }
    }*/

    return rc;
}

