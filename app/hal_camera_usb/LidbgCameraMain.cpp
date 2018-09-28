

#define ALOG_NIDEBUG 1
#define LOG_TAG "lidbgUsbCamera"

#include "LidbgCameraCommon.h"
#include "LidbgCameraMain.h"
#include "LidbgCameraUsb.h"
#include <cutils/properties.h>

extern "C" {
#include <sys/time.h>
}

static hw_module_methods_t camera_module_methods =
{
open:
    camera_device_open,
};

static hw_module_t camera_common  =
{
tag:
    HARDWARE_MODULE_TAG,
module_api_version:
    CAMERA_MODULE_API_VERSION_1_0,
hal_api_version:
    HARDWARE_HAL_API_VERSION,
id:
    CAMERA_HARDWARE_MODULE_ID,
name: "futengfei.camera.hal"
    ,
author: "futengfei"
    ,
methods:
    &camera_module_methods,
dso:
    NULL,
reserved:
    {0},
};
camera_module_t HAL_MODULE_INFO_SYM =
{
common:
    camera_common,
get_number_of_cameras:
    get_number_of_cameras,
get_camera_info:
    get_camera_info,
set_callbacks:
    NULL,
get_vendor_tag_ops:
    NULL,
#ifdef ANDROID_AT_LEAST_50
open_legacy:
    NULL,
#endif
#if ANDROID_VERSION>=600
set_torch_mode:
	NULL,
init:
	NULL,
#endif
reserved:
    {0},
};



namespace android
{
    extern "C" int get_number_of_cameras()
    {
        ALOGE("Q%s: E", __func__);
        return usbcam_get_number_of_cameras();
    }

    extern "C" int get_camera_info(int camera_id, struct camera_info *info)
    {
        ALOGE("Q%s: E", __func__);
        return usbcam_get_camera_info(camera_id, info);
    }

    extern "C" int  camera_device_open(const struct hw_module_t *module, const char *id, struct hw_device_t **hw_device)
    {
        ALOGE("%s:  end camera_device_open", __func__);
        return usbcam_camera_device_open(module, id, hw_device);
    }

};
