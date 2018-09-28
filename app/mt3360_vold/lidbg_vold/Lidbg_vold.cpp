//add by wangyihong for supporting multi partitions
/* path: partition mount path. eg: '/mnt/usbhost1/8_1' */
#include "Lidbg_vold.h"
int Lidbg_vold::deleteDeviceNode(const char *path){
#if 0
    int major = 0, minor = 0;
	char devicePath[255];

	char *temp_str1 = NULL;
	char *temp_str2 = NULL;
	char str_major[256];
	char str_path[256];
	int len = 0;

	if(!path){
		SLOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
		return -1;
	}

	SLOGI("Volume::deleteDeviceNode: path=%s\n", path);

	/* get device major and minor from path */
	memset(str_major, 0, 256);
	memset(str_path, 0, 256);
	strcpy(str_path, path);

	temp_str1 = strrchr(str_path, '/');
	temp_str2 = strrchr(str_path, '_');
	if(temp_str1 == NULL || temp_str2 == NULL){
		SLOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
		return -1;
	}

	/* delete '/' & '_' */
	temp_str1++;
	temp_str2++;
	if(temp_str1 == NULL || temp_str2 == NULL){
		SLOGE("Volume::deleteDeviceNode: path(%s) is invalid\n", path);
		return -1;
	}

	len = strcspn(temp_str1, "_");
	strncpy(str_major, temp_str1, len);

	major = strtol(str_major, NULL, 10);
	minor = strtol(temp_str2, NULL, 10);

	SLOGI("Volume::deleteDeviceNode: major=%d, minor=%d\n", major, minor);

	/* delete DeviceNode */
	memset(devicePath, 0, 255);
	sprintf(devicePath, "/dev/block/vold/%d:%d", major, minor);

	if (unlink(devicePath)) {
		SLOGE("Volume::deleteDeviceNode: Failed to remove %s (%s)", path, strerror(errno));
		return -1;
	}else{
		SLOGI("Volume::deleteDeviceNode: delete DeviceNode '%s' successful\n", path);
	}

#endif

	return 0;
}

char* Lidbg_vold::createMountPoint(const char *path, int major, int minor) {
	char* mountpoint = (char*) malloc(sizeof(char)*256);

	memset(mountpoint, 0, sizeof(char)*256);
	//sprintf(mountpoint, "%s/%d_%d", path, major, minor);
	sprintf(mountpoint, "%s/disk_%d", path, minor);
	if( access(mountpoint, F_OK) ){
		SLOGI("Volume: file '%s' is not exist, create it", mountpoint);

		if(mkdir(mountpoint, 0777)){
			SLOGW("Volume: create file '%s' failed, errno is %d", mountpoint, errno);
			return NULL;
		}
	}else{
	
		SLOGW("Volume: file '%s' is exist, can not create it", mountpoint);
		return mountpoint;
	}

	return mountpoint;
}

int Lidbg_vold::deleteMountPoint(char* mountpoint) {
	if(mountpoint){
		SLOGW("Volume::deleteMountPoint: %s exist", mountpoint); 
		rmdir(mountpoint);
		if( !access(mountpoint, F_OK) ){
			SLOGW("Volume::deleteMountPoint: %s", mountpoint);
			if(rmdir(mountpoint)){
				SLOGW("Volume: remove file '%s' failed, errno is %d", mountpoint, errno);
				return -1;
			}
		}

	
		free(mountpoint);
		mountpoint = NULL;
	}

	return 0;
}

void Lidbg_vold::saveUnmountPoint(char* mountpoint){
	int i = 0;

	for(i = 0; i < MAX_UNMOUNT_PARTITIONS; i++){
		if(mUnMountPart[i] == NULL){
			mUnMountPart[i] = mMountPart[i];
		}
	}

	if(i >= MAX_UNMOUNT_PARTITIONS){
		SLOGI("Volume::saveUnmountPoint: unmount point is over %d", MAX_UNMOUNT_PARTITIONS);
	}

	return;
}


void Lidbg_vold::deleteUnMountPoint(int clear){
	int i = 0;

	for(i = 0; i < MAX_UNMOUNT_PARTITIONS; i++){
		if(mUnMountPart[i]){
			SLOGW("Volume::deleteUnMountPoint: %s", mUnMountPart[i]);

			if(deleteMountPoint(mUnMountPart[i]) == 0){
				deleteDeviceNode(mUnMountPart[i]);
				mUnMountPart[i] = NULL;
			}
		}
	}

	return;
}

void Lidbg_vold::startFuse(void){
	property_set("ctl.start", "fuse_disk1");
	property_set("ctl.start", "fuse_disk2");
	property_set("ctl.start", "fuse_disk3");
	property_set("ctl.start", "fuse_disk4");

	return;
}
void Lidbg_vold::stopFuse(void)
{
	property_set("ctl.stop", "fuse_disk1");
	property_set("ctl.stop", "fuse_disk2");
	property_set("ctl.stop", "fuse_disk3");
	property_set("ctl.stop", "fuse_disk4");
	return;
}




