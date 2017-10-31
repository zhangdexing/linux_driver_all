
#ifndef ANDROID_HARDWARE_QUALCOMM_CAMERA_H
#define ANDROID_HARDWARE_QUALCOMM_CAMERA_H

extern "C" {
    int get_number_of_cameras();
    int get_camera_info(int camera_id, struct camera_info *info);
    int camera_device_open(const struct hw_module_t *module, const char *id,
                           struct hw_device_t **device);
    hw_device_t *open_camera_device(int cameraId);
    int close_camera_device( hw_device_t *);

}

#endif

