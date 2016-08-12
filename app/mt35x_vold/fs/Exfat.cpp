/*
 * Copyright (C) 2015 Mediatek Inc.
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
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <base/logging.h>
#include <base/stringprintf.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <selinux/selinux.h>

#include <logwrap/logwrap.h>

#include "Vfat.h"
#include "Utils.h"
#include "VoldUtil.h"

using android::base::StringPrintf;

namespace android {
namespace vold {
namespace exfat {

static const char *FLY_PRODUCT_PATH = "/flysystem/lib/out/mkfs.exfat";
static const char *NATIVE_SYSTEM_PATH = "/system/lib/modules/out/mkfs.exfat";

static const char* kMkfsPath = "/system/bin/mkfs.exfat";
static const char* kFsckPath = "/system/bin/fsck.exfat";
static const char* kMountPath = "/system/bin/mount.exfat";

int selectPath(){
	if(!access(FLY_PRODUCT_PATH, X_OK))
	{
		kMkfsPath = "/flysystem/lib/out/mkfs.exfat";
		kFsckPath = "/flysystem/lib/out/fsck.exfat";
		kMountPath = "/flysystem/lib/out/mount.exfat";
		return 0;
	}
	else if(!access(NATIVE_SYSTEM_PATH, X_OK))
	{
		kMkfsPath = "/system/lib/modules/out/mkfs.exfat";
		kFsckPath = "/system/lib/modules/out/fsck.exfat";
		kMountPath = "/system/lib/modules/out/mount.exfat";
		return 0;
	}
	else
		return -1;
}


bool IsSupported() {
    bool ret;
	ret = selectPath();
    ret = access(kMkfsPath, X_OK) == 0
            && access(kFsckPath, X_OK) == 0;
    SLOGI("exfat::IsSupported() = %s", ret?"true":"false");
    return true;
}

status_t Check(const std::string& source) {
    if (access(kFsckPath, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;
    do {
        std::string exec;
        exec = kFsckPath;
        exec = exec + " " + source;

        rc = system(exec.c_str());

        switch(rc) {
        case 0:
            SLOGI("Filesystem check completed OK");
            return 0;
        default:
            SLOGE("Filesystem check failed (unknown exit code %d)", rc);
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}

status_t Mount(const std::string& source, const std::string& target, bool ro,
        bool remount, bool executable, int ownerUid, int ownerGid, int permMask,
        bool createLost) {
    char mountData[255];

    const char* c_source = source.c_str();
    const char* c_target = target.c_str();

    sprintf(mountData,
#ifdef CONFIG_KERNEL_HAVE_EXFAT
            "noatime,nodev,nosuid,uid=%d,gid=%d,fmask=%o,dmask=%o,%s,%s",
#else
            "noatime,nodev,nosuid,dirsync,uid=%d,gid=%d,fmask=%o,dmask=%o,%s,%s",
#endif
            ownerUid, ownerGid, permMask, permMask,
            (executable ? "exec" : "noexec"),
            (ro ? "ro" : "rw"));

    std::vector<std::string> cmd;
    cmd.push_back(kMountPath);
#ifdef CONFIG_KERNEL_HAVE_EXFAT
    cmd.push_back("-t");
    cmd.push_back("exfat");
#endif
    cmd.push_back("-o");
    cmd.push_back(mountData);
    cmd.push_back(c_source);
    cmd.push_back(c_target);

    return ForkExecvp(cmd);
}

status_t Format(const std::string& source, unsigned int numSectors) {
    std::string exec;
    exec = kMkfsPath;
    exec = exec + " " + source;
    int rc;

    rc = system(exec.c_str());

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

}  // namespace exfat
}  // namespace vold
}  // namespace android
