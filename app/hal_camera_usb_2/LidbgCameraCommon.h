
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <pthread.h>
#include <hardware/camera.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>

#include <linux/videodev2.h>

#include <camera/CameraParameters.h>
#include <hardware/camera_common.h>
#include <hardware/camera.h>
#include <binder/IMemory.h>
#include <utils/String8.h>


#include <linux/ion.h>
#include <hardware/gralloc.h>
#include "gralloc_priv.h"

#define CAPTURE                 1
#define USE_ION

//flyaudio
#define NONE_HUB_SUPPORT	1

static const int PICTURE_FORMAT_JPEG = 1;
static const int PICTURE_FORMAT_RAW = 2;

#define DEFAULT_USBCAM_PRVW_WD  640
#define DEFAULT_USBCAM_PRVW_HT  480
#define DEFAULT_USBCAM_PICT_FMT     PICTURE_FORMAT_JPEG

#define DEFAULT_USBCAM_PICT_WD  640
#define DEFAULT_USBCAM_PICT_HT  480

#define DEFAULT_USBCAM_PICT_QLTY  85
#define DEFAULT_USBCAM_THUMBNAIL_WD    432
#define DEFAULT_USBCAM_THUMBNAIL_HT    288
#define DEFAULT_USBCAM_THUMBNAIL_QLTY  85
#define DEFAULT_USBCAM_PRVW_FMT HAL_PIXEL_FORMAT_YV12//HAL_PIXEL_FORMAT_YCrCb_420_SP //HAL_PIXEL_FORMAT_YV12
#define MIN_PREV_FPS            5000
#define MAX_PREV_FPS            121000

#define HISTOGRAM_STATS_SIZE 257
#define NUM_HISTOGRAM_BUFFERS 3
#define MM_CAMERA_MAX_NUM_FRAMES     16
#define PRVW_DISP_BUF_CNT   2
#define PRVW_CAP_BUF_CNT    4

#define USB_CAM_PREVIEW_EXIT    (0x100)
#define USB_CAM_PREVIEW_PAUSE   (0x101)
#define USB_CAM_PREVIEW_TAKEPIC (0x200)

#define ION_IOC_MSM_MAGIC 'M'
#define ION_IOC_CLEAN_CACHES	_IOWR(ION_IOC_MSM_MAGIC, 0, \
						struct ion_flush_data)
#define FILENAME_LENGTH     (256)


enum ion_heap_ids
{
    INVALID_HEAP_ID = -1,
    ION_CP_MM_HEAP_ID = 8,
    ION_CP_MFC_HEAP_ID = 12,
    ION_CP_WB_HEAP_ID = 16, /* 8660 only */
    ION_CAMERA_HEAP_ID = 20, /* 8660 only */
    ION_SYSTEM_CONTIG_HEAP_ID = 21,
    ION_ADSP_HEAP_ID = 22,
    ION_PIL1_HEAP_ID = 23, /* Currently used for other PIL images */
    ION_SF_HEAP_ID = 24,
    ION_SYSTEM_HEAP_ID = 25,
    ION_PIL2_HEAP_ID = 26, /* Currently used for modem firmware images */
    ION_QSECOM_HEAP_ID = 27,
    ION_AUDIO_HEAP_ID = 28,

    ION_MM_FIRMWARE_HEAP_ID = 29,

    ION_HEAP_ID_RESERVED = 31 /** Bit reserved for ION_FLAG_SECURE flag */
};

#define ION_IOMMU_HEAP_ID ION_SYSTEM_HEAP_ID
#define ION_HEAP_TYPE_IOMMU ION_HEAP_TYPE_SYSTEM

#if 0
typedef struct
{
    /* config a job -- async call */
    int (*start_job)(mm_jpeg_job_t *job, uint32_t *job_id);

    /* abort a job -- sync call */
    int (*abort_job)(uint32_t job_id);

    /* create a session */
    int (*create_session)(uint32_t client_hdl,
                          mm_jpeg_encode_params_t *p_params, uint32_t *p_session_id);

    /* destroy session */
    int (*destroy_session)(uint32_t session_id);

    /* close a jpeg client -- sync call */
    int (*close) (uint32_t clientHdl);
} mm_jpeg_ops_t;
#endif

#define VALIDATE_DEVICE_HDL(camHal, device, ret_err_code)     {\
    if(device && device->priv){\
        camHal = (camera_hardware_t *)device->priv;\
    }else{\
        ALOGE("%s: Null device or device->priv", __func__);\
        return ret_err_code;\
    }\
}


#define USB_CAM_OPEN(camHal)    {\
        camHal->fd = open(camHal->dev_name, O_RDWR | O_NONBLOCK, 0);\
        if(!camHal->fd)\
            ALOGE("%s: Error in open", __func__);\
        else\
            ALOGD("%s: Successfully opened", __func__);\
        }

#define USB_CAM_CLOSE(camHal) {\
        int rc;\
        if(camHal->fd){\
            rc = close(camHal->fd);\
            if(0 > rc){\
                ALOGE("%s: close failed ", __func__);\
            }\
            else{\
                camHal->fd = 0;\
                ALOGD("%s: close successful", __func__);\
            }\
        }\
    }
#define ERROR_CHECK_EXIT_THREAD(rc, string)    {\
    if(rc < 0) {\
        ALOGE("%s: Error %s", __func__, string);\
        return (void *)-1;\
    }\
}

#define MAX_JPEG_BUFFER_SIZE    (1024 * 1024)
namespace android
{

    using android::Mutex;

    struct camera_size_type
    {
        int width;
        int height;
    };

    struct ion_flush_data
    {

	ion_user_handle_t handle;

        int fd;
        void *vaddr;
        unsigned int offset;
        unsigned int length;
    };

    struct bufObj
    {
        void    *data;
        int     len;
    };

    typedef enum
    {
        MM_CAMERA_OK,
        MM_CAMERA_E_GENERAL,
        MM_CAMERA_E_NO_MEMORY,
        MM_CAMERA_E_NOT_SUPPORTED,
        MM_CAMERA_E_INVALID_INPUT,
        MM_CAMERA_E_INVALID_OPERATION, /* 5 */
        MM_CAMERA_E_ENCODE,
        MM_CAMERA_E_BUFFER_REG,
        MM_CAMERA_E_PMEM_ALLOC,
        MM_CAMERA_E_CAPTURE_FAILED,
        MM_CAMERA_E_CAPTURE_TIMEOUT, /* 10 */
    } mm_camera_status_type_t;

    enum
    {
        BUFFER_NOT_OWNED,
        BUFFER_UNLOCKED,
        BUFFER_LOCKED,
    };
    typedef struct
    {
        int                     fd;
        int                     main_ion_fd;

	ion_user_handle_t handle;

        uint32_t                size;
    } QCameraHalMemInfo_t;

    typedef struct
    {
        int                     buffer_count;
        buffer_handle_t        *buffer_handle[MM_CAMERA_MAX_NUM_FRAMES];
        struct private_handle_t *private_buffer_handle[MM_CAMERA_MAX_NUM_FRAMES];
        int                     stride[MM_CAMERA_MAX_NUM_FRAMES];
        uint32_t                addr_offset[MM_CAMERA_MAX_NUM_FRAMES];
        uint8_t                 local_flag[MM_CAMERA_MAX_NUM_FRAMES];
        camera_memory_t        *camera_memory[MM_CAMERA_MAX_NUM_FRAMES];
        QCameraHalMemInfo_t     mem_info[MM_CAMERA_MAX_NUM_FRAMES];
    } QCameraHalMemory_t;

    typedef struct
    {
        camera_device                       hw_dev;
        Mutex                               lock;
        int                                 previewEnabledFlag;
        int                                 prvwStoppedForPicture;
        int                                 msgEnabledFlag;
        volatile int                        prvwCmdPending;
        volatile int                        prvwCmd;
        pthread_t                           previewThread;
        pthread_t                           takePictureThread;

        camera_notify_callback              notify_cb;
        camera_data_callback                data_cb;
        camera_data_timestamp_callback      data_cb_timestamp;
        camera_request_memory               get_memory;
        void                               *cb_ctxt;

        int                                 prevFormat;
        int                                 prevFps;
        int                                 prevWidth;
        int                                 prevHeight;

		char *								dispData;

        int                                 captureFormat;
        char                                dev_name[FILENAME_LENGTH];
        int                                 fd;
        unsigned int                        n_buffers;
        struct v4l2_buffer                  curCaptureBuf;
        struct bufObj                       *buffers;


        preview_stream_ops                 *window;
        QCameraHalMemory_t                  previewMem;

        int                                 dispFormat;
        int                                 dispWidth;
        int                                 dispHeight;

        /* MJPEG decoder related members */
        /* MJPEG decoder object */
        void                               *mjpegd;

        /* JPEG picture and thumbnail related members */
        int                                 pictFormat;
        int                                 pictWidth;
        int                                 pictHeight;
        int                                 pictJpegQlty;
        int                                 thumbnailWidth;
        int                                 thumbnailHeight;
        int                                 thumbnailJpegQlty;
        QCameraHalMemory_t                  pictMem;
        int                                 takePictInProgress;
        int                                 jpegEncInProgress;
        pthread_mutex_t                     jpegEncMutex;
        pthread_cond_t                      jpegEncCond;

        /* */
        //    QCameraParameters                   qCamParams;
        String8                             prevSizeValues;
        String8                             pictSizeValues;
        String8                             thumbnailSizeValues;
        String8                             vidSizeValues;
        String8                             pictFormatValues;
        String8                             prevFormatValues;
        String8                             prevFpsRangesValues;
    } camera_hardware_t;

}

