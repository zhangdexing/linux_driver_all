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
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <fcntl.h>
#include <dirent.h>

#define LOG_TAG "Vold"

#include "cutils/log.h"

#include "VolumeManager.h"
#include "CommandListener.h"
#include "NetlinkManager.h"
#include "DirectVolume.h"
#include "cryptfs.h"

static int process_config(VolumeManager *vm);
static void coldboot(const char *path);
const int MaxNumMountPartition =15;
char* ParitionMountPoints[MaxNumMountPartition*5]={
 "/udisk1_partition1",
 "/udisk1_partition2",
 "/udisk1_partition3",
 "/udisk1_partition4",
 "/udisk1_partition5",
 "/udisk1_partition6",
 "/udisk1_partition7",
 "/udisk1_partition8",
 "/udisk1_partition9",
 "/udisk1_partition10",
 "/udisk1_partition11",
 "/udisk1_partition12",
 "/udisk1_partition13",
 "/udisk1_partition14",
 "/udisk1_partition15",
 "/udisk2_partition1",
 "/udisk2_partition2",
 "/udisk2_partition3",
 "/udisk2_partition4",
 "/udisk2_partition5",
 "/udisk2_partition6",
 "/udisk2_partition7",
 "/udisk2_partition8",
 "/udisk2_partition9",
 "/udisk2_partition10",
 "/udisk2_partition11",
 "/udisk2_partition12",
 "/udisk2_partition13",
 "/udisk2_partition14",
 "/udisk2_partition15",
 "/udisk3_partition1",
 "/udisk3_partition2",
 "/udisk3_partition3",
 "/udisk3_partition4",
 "/udisk3_partition5",
 "/udisk3_partition6",
 "/udisk3_partition7",
 "/udisk3_partition8",
 "/udisk3_partition9",
 "/udisk3_partition10",
 "/udisk3_partition11",
 "/udisk3_partition12",
 "/udisk3_partition13",
 "/udisk3_partition14",
 "/udisk3_partition15",
 "/udisk4_partition1",
 "/udisk4_partition2",
 "/udisk4_partition3",
 "/udisk4_partition4",
 "/udisk4_partition5",
 "/udisk4_partition6",
 "/udisk4_partition7",
 "/udisk4_partition8",
 "/udisk4_partition9",
 "/udisk4_partition10",
 "/udisk4_partition11",
 "/udisk4_partition12",
 "/udisk4_partition13",
 "/udisk4_partition14",
 "/udisk4_partition15",
 "/udisk5_partition1",
 "/udisk5_partition2",
 "/udisk5_partition3",
 "/udisk5_partition4",
 "/udisk5_partition5",
 "/udisk5_partition6",
 "/udisk5_partition7",
 "/udisk5_partition8",
 "/udisk5_partition9",
 "/udisk5_partition10",
 "/udisk5_partition11",
 "/udisk5_partition12",
 "/udisk5_partition13",
 "/udisk5_partition14",
 "/udisk5_partition15"
 };

int main() {

    VolumeManager *vm;
    CommandListener *cl;
    NetlinkManager *nm;

    SLOGI("Vold 2.1 (the revenge) firing up");

    mkdir("/dev/block/vold", 0755);

    /* Create our singleton managers */
    if (!(vm = VolumeManager::Instance())) {
        SLOGE("Unable to create VolumeManager");
        exit(1);
    };

    if (!(nm = NetlinkManager::Instance())) {
        SLOGE("Unable to create NetlinkManager");
        exit(1);
    };


    cl = new CommandListener();
    vm->setBroadcaster((SocketListener *) cl);
    nm->setBroadcaster((SocketListener *) cl);

    if (vm->start()) {
        SLOGE("Unable to start VolumeManager (%s)", strerror(errno));
        exit(1);
    }

    if (process_config(vm)) {
        SLOGE("Error reading configuration (%s)... continuing anyways", strerror(errno));
    }

    if (nm->start()) {
        SLOGE("Unable to start NetlinkManager (%s)", strerror(errno));
        exit(1);
    }

    coldboot("/sys/block");
//    coldboot("/sys/class/switch");

    /*
     * Now that we're up, we can respond to commands
     */
    if (cl->startListener()) {
        SLOGE("Unable to start CommandListener (%s)", strerror(errno));
        exit(1);
    }

    // Eventually we'll become the monitoring thread
    while(1) {
        sleep(1000);
    }

    SLOGI("Vold exiting");
    exit(0);
}

static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if(d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}

static int parse_mount_flags(char *mount_flags)
{
    char *save_ptr;
    int flags = 0;

    if (strcasestr(mount_flags, "encryptable")) {
        flags |= VOL_ENCRYPTABLE;
    }

    if (strcasestr(mount_flags, "nonremovable")) {
        flags |= VOL_NONREMOVABLE;
    }

    return flags;
}

static int process_config(VolumeManager *vm) {
    FILE *fp;
    int n = 0;
    int usb1hubnum=0;
    char line[255];

    if (!(fp = fopen("/etc/vold.fstab", "r"))) {
        return -1;
    }

	SLOGE("========= Max_Xia: Start Vold Process =========");

    while(fgets(line, sizeof(line), fp)) {
        const char *delim = " \t";
        char *save_ptr;
        char *reservemedia;
        char *type, *label, *mount_point, *mount_flags, *sysfs_path;
        int flags;

        n++;
        line[strlen(line)-1] = '\0';

        if (line[0] == '#' || line[0] == '\0')
            continue;

        if (!(type = strtok_r(line, delim, &save_ptr))) {
            SLOGE("Error parsing type");
            goto out_syntax;
        }
        if (!(label = strtok_r(NULL, delim, &save_ptr))) {
            SLOGE("Error parsing label");
            goto out_syntax;
        }
        if (!(mount_point = strtok_r(NULL, delim, &save_ptr))) {
            SLOGE("Error parsing mount point");
            goto out_syntax;
        }

        if (!(reservemedia = strtok_r(NULL, delim, &save_ptr))){
            SLOGE("Error parsing reserve media point");
            goto out_syntax;
        }

        if (!strcmp(reservemedia, "internal")){
            if (!strcmp(type, "dev_mount")) {
                DirectVolume *dv = NULL;
                char *part;
                char *hub;
		
		if (!(hub = strtok_r(NULL, delim, &save_ptr))) {
                    SLOGE("Can't get hub number\r\n");
                    goto out_syntax;
                }
		
                if (!(part = strtok_r(NULL, delim, &save_ptr))) {
                    SLOGE("internal: Error parsing partition");
                    goto out_syntax;
                }
                if (strcmp(part, "auto") && atoi(part) == 0) {
                    SLOGE("internal: Partition must either be 'auto' or 1 based index instead of '%s'", part);
                    goto out_syntax;
                }

                if (!strcmp(part, "auto")) {
                    dv = new DirectVolume(vm, label, mount_point, -1, atoi(hub));
                } else {
                    dv = new DirectVolume(vm, label, mount_point, atoi(part), atoi(hub));
                }

                while ((sysfs_path = strtok_r(NULL, delim, &save_ptr))) {
                    if (*sysfs_path != '/') {
                        /* If the first character is not a '/', it must be flags */
                        break;
                    }
                    if (dv->addPath(sysfs_path)) {
                        SLOGE("internal: Failed to add devpath %s to volume %s", sysfs_path,
                            label);
                        goto out_fail;
                    }
                }

                /* If sysfs_path is non-null at this point, then it contains
                * the optional flags for this volume
                */
                if (sysfs_path)
                    flags = parse_mount_flags(sysfs_path);
                else
                    flags = 0;
                dv->setFlags(flags);

                vm->addVolume(dv);
            } else if (!strcmp(type, "map_mount")) {
            } else {
                SLOGE("Unknown type '%s'", type);
                goto out_syntax;
            }
        }
        else if (!strcmp(reservemedia, "external"))
        {
            if (!strcmp(type, "dev_mount")) {
                DirectVolume *dv = NULL;
                char *part;
		char *sysfs_path2;
		char *hub;
		
		if (!(hub = strtok_r(NULL, delim, &save_ptr))) {
                    SLOGE("Can't get hub number\r\n");
                    goto out_syntax;
                }
		
                if (!(part = strtok_r(NULL, delim, &save_ptr))) {
                    SLOGE("external: Error parsing partition");
                    goto out_syntax;
                }
                if (strcmp(part, "auto") && atoi(part) == 0) {
                    SLOGE("external: Partition must either be 'auto' or 1 based index instead of '%s'", part);
                    goto out_syntax;
                }

                if (!strcmp(part, "auto")) {
                    dv = new DirectVolume(vm, label, mount_point, -1, atoi(hub));
                } else {
                    dv = new DirectVolume(vm, label, mount_point, atoi(part), atoi(hub));
                }

                while ((sysfs_path = strtok_r(NULL, delim, &save_ptr))) {
                    if (*sysfs_path != '/') {
                        /* If the first character is not a '/', it must be flags */
                        break;
                    }
                    if (dv->addPath(sysfs_path)) {
                        SLOGE("external: Failed to add devpath %s to volume %s", sysfs_path,
                            label);
                        goto out_fail;
                    }else{
 			sysfs_path2=sysfs_path;
                    }
                }

                /* If sysfs_path is non-null at this point, then it contains
                * the optional flags for this volume
                */
                if (sysfs_path)
                    flags = parse_mount_flags(sysfs_path);
                else
                    flags = 0;
                dv->setFlags(flags);
                dv->setReserveMedia(0);

                vm->addVolume(dv);
		if(strcmp(label,"udisk")==0)
		{
			int j;
			int offset;
			int iRet;
			char path[255];
			errno=0;

			if(strstr(sysfs_path2,"usb1")!=NULL){
			    if(usb1hubnum==0)
			        offset=0;
			    else
				offset=(usb1hubnum+1)*MaxNumMountPartition;
				
			     usb1hubnum++;
			}
			else if (strstr(sysfs_path2,"usb2")!=NULL)
			   offset=MaxNumMountPartition;
			else{
			    SLOGE("Invalide usb port\r\n");
			    return -1;
			}
			for(j=0;j<MaxNumMountPartition;j++){
			    strcpy(path,mount_point);
			    strcat(path,ParitionMountPoints[offset+j]);
			 
			    dv = new DirectVolume(vm, label, path, j+1, atoi(hub));	
			    dv->addPath(sysfs_path2);
			    dv->setFlags(flags);
                	    dv->setReserveMedia(0);
			    vm->addVolume(dv);
			}
		}
            } else if (!strcmp(type, "map_mount")) {
            } else {
                SLOGE("external: Unknown type '%s'", type);
                goto out_syntax;
            }
        }
    }

    fclose(fp);
    return 0;

out_syntax:
    SLOGE("Syntax error on config line %d", n);
    errno = -EINVAL;
out_fail:
    fclose(fp);
    return -1;   
}
