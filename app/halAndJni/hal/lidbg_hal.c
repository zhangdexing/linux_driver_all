

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#include <linux/i2c-dev.h>

#include "lidbg_servicer.h"
#include "lidbg_flycam_par.h"
#include "hal_lidbg_commen.h"

// #define LOG_NDEBUG 0
#define OUR_LIDBG_FILE   "/dev/lidbg_flycam0"
#define DEBG_TAG "hal_futengfei."

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_havelidbgNode = 0;
static JniCallbacks jni_call_backs;
static int dbg_level = 0;
static int flycam_fd = -1;
static int thread_exit = 0;
char para[128] ;


static int send_driver_ioctl(const char *who, char magic , char nr, unsigned long  arg)
{
    int ret = DEFAULT_ERR_VALUE;

    if(flycam_fd < 0)
    {
        flycam_fd = open(OUR_LIDBG_FILE, O_RDWR);
        if(flycam_fd < 0)
        {
            lidbg(DEBG_TAG"[%s].fail2open[%s].%s\n", __FUNCTION__, OUR_LIDBG_FILE, strerror(errno));
            return ret;
        }
    }
    ret = ioctl(flycam_fd, _IO(magic, nr), arg);
    lidbg(DEBG_TAG"[%s].%s.suc.[(%c,%d),ret=%d,errno=%s]\n", who, __FUNCTION__  , magic, nr, ret, strerror(errno));
    return ret;
}

static void lidbg_hal_thread( void *arg)
{
    char msg[256] = {0};
    int cnt = 0;
    arg = arg;
    while(thread_exit == 0)
    {
        switch(dbg_level)
        {
        case 0 :
        {
            int ret = send_driver_ioctl(__FUNCTION__, FLYCAM_STATUS_IOC_MAGIC, 0x06, (unsigned long) NULL);
            if ((thread_exit == 0) && jni_call_backs.driver_abnormal_cb)
            {
                jni_call_backs.driver_abnormal_cb(ret);
                lidbg(DEBG_TAG"[%s].call driver_abnormal_cb:ret=%d\n", __FUNCTION__, ret );
                if(flycam_fd < 0)
                {
					lidbg(DEBG_TAG"[%s].flycam_fd < 0 more delay sleep(5)\n", __FUNCTION__ );
					sleep(5);
                }
            }
        }
        continue;
        case 1 :
        {
            if ((thread_exit == 0) && jni_call_backs.test_cb )
            {
                sprintf(msg, "hal2jni2apk thread: %d\n", cnt);
                jni_call_backs.test_cb(msg);
                lidbg(DEBG_TAG"[%s].call test_cb:msg=%s\n", __FUNCTION__, msg );
                sleep(3);
            }
        }
        break;

        default :
            break;
        }
        cnt++;
    }
    lidbg(DEBG_TAG"[%s].exit\n", __FUNCTION__ );
}

int   lidbg_hal_set_debg_level(int level)
{
    dbg_level = level;
    return 1;
};
int   lidbg_hal_init(int camera_id, JniCallbacks *callbacks)
{
    thread_exit = 0;
    jni_call_backs = *callbacks;
    flycam_fd = open(OUR_LIDBG_FILE, O_RDWR);
    lidbg(DEBG_TAG"[%s].in.camera_id:%d,flycam_fd:%d\n", __FUNCTION__, camera_id , flycam_fd);

    if ( !jni_call_backs.create_thread_cb || !(jni_call_backs.create_thread_cb( "lidbg_hal", lidbg_hal_thread, NULL )) )
    {
        lidbg(DEBG_TAG"could not create  lidbg_hal_thread: %s\n", strerror(errno));
    }
    return 1;
};
int   lidbg_hal_set_path(int camera_id, char *path)
{
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,%s]\n", __FUNCTION__, camera_id, path);
    return send_driver_ioctl(__FUNCTION__, FLYCAM_FRONT_ONLINE_IOC_MAGIC, NR_PATH, (unsigned long) path);
};
int   lidbg_hal_start_record(int camera_id)
{
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,%s]\n", __FUNCTION__, camera_id, "null");
    return  send_driver_ioctl(__FUNCTION__, FLYCAM_FRONT_ONLINE_IOC_MAGIC, NR_START_REC, (unsigned long) NULL);
};
int   lidbg_hal_stop_record(int camera_id)
{
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,%s]\n", __FUNCTION__, camera_id, "null");
    return  send_driver_ioctl(__FUNCTION__, FLYCAM_FRONT_ONLINE_IOC_MAGIC, NR_STOP_REC, (unsigned long) NULL);
};
int   lidbg_hal_take_picture(int camera_id, char *path)
{
    int ret = -1;
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,%s]\n", __FUNCTION__, camera_id, path);
    if(camera_id == CAM_ID_FRONT)
    {
        ret = send_driver_ioctl(__FUNCTION__, FLYCAM_FRONT_ONLINE_IOC_MAGIC, NR_CAPTURE_PATH, (unsigned long) path);
        ret = send_driver_ioctl(__FUNCTION__, FLYCAM_FRONT_ONLINE_IOC_MAGIC, NR_CAPTURE, (unsigned long) NULL);
    }
    else
    {
        ret = send_driver_ioctl(__FUNCTION__, FLYCAM_REAR_ONLINE_IOC_MAGIC, NR_CAPTURE_PATH, (unsigned long) path);
        ret = send_driver_ioctl(__FUNCTION__, FLYCAM_REAR_ONLINE_IOC_MAGIC, NR_CAPTURE, (unsigned long) NULL);
    }
    return ret;
};
int   lidbg_hal_urgent_record_set_path(int camera_id, char *path)
{
    int ret = -1;
    strncpy(para, path, sizeof(para) - 1);
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_EM_PATH), para);
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,path:%s,para:%s,ret:%d]\n", __FUNCTION__, camera_id, path, para, ret);
    return ret;
};
int   lidbg_hal_urgent_record_set_times(int camera_id, int times_in_S)
{
    int ret = -1;
    para[0] = camera_id;
    para[1] = times_in_S;
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_EM_TIME), para);
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,times_in_S:%d/%d,ret:%d]\n", __FUNCTION__, camera_id, times_in_S, para[0], ret);

    return ret;
};
int   lidbg_hal_urgent_record_ctrl(int camera_id, int start_stop)
{
    int ret = -1;
    para[0] = camera_id;
    para[1] = start_stop;
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_EM_START), para);
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,start_stop:%d/%d,ret:%d]\n", __FUNCTION__, camera_id, start_stop, para[0], ret);
    return ret;
};
char    *lidbg_hal_urgent_record_get_status(int camera_id)
{
    int ret = -1;
    memset(para, 0, sizeof(para) - 1);
    para[0] = camera_id;
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_EM_STATUS), para);
    //lidbg(DEBG_TAG"[%s].in.[camera_id:%d,ret:%d,para[0]:%d/%d/%d]\n", __FUNCTION__, camera_id,  ret, para[0], para[1], para[2]);
    for(ret = 0; ret < (int)(sizeof(para) / 3); ret++)
    {
        para[ret] +=  48;
    }
    para[ret] = '\0';
    return para;
};
int   lidbg_hal_urgent_record_manual(int camera_id, int start_stop)
{
    int ret = -1;
    para[0] = camera_id;
    para[1] = start_stop;
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_EM_MANUAL), para);
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,start_stop:%d/%d,ret:%d]\n", __FUNCTION__, camera_id, start_stop, para[0], ret);
    return ret;
};
bool   lidbg_hal_is_camera_connect(int camera_id)
{
    int ret = -1;
    para[0] = camera_id;
    ret = ioctl(flycam_fd, _IO(FLYCAM_EM_MAGIC, NR_CAM_STATUS), para);
    lidbg(DEBG_TAG"[%s].in.[camera_id:%d,ret:%d,para[2]:%d]\n", __FUNCTION__, camera_id, ret, para[2]);
    return (para[2] == 0);
};
int lidbg_hal_i2c_open(char *nodeName)
{
    int fd = 0 , res;
    int retrycount = 10 ;
    fd = open(nodeName, O_RDWR | O_NONBLOCK, 0);
    while((retrycount > 0) && ( fd < 0) )
    {
        if (fd < 0)
        {
            lidbg(DEBG_TAG"Can't open i2c bus:%s\n", nodeName);
            usleep(20 * 1000);
        }
        fd = open(nodeName, O_RDWR | O_NONBLOCK, 0);
        lidbg(DEBG_TAG"retrycount =%d", retrycount);
        retrycount--;
    }
    return fd;
};
int lidbg_hal_i2c_read(int fd,  unsigned short slaveAddr, unsigned char *dataBuf, int len)
{
    int ret, reg;
    lidbg(DEBG_TAG"[%s].in slaveAddr:0x%x reg:0x%x len:%d\n", __FUNCTION__, slaveAddr, dataBuf[0], len);
    if (ioctl(fd, I2C_SLAVE_FORCE , slaveAddr) < 0)
    {
        lidbg(DEBG_TAG"Could not set address to 0x%02x: %s\n", slaveAddr, strerror(errno));
        return -errno;
    }

    reg = dataBuf[0];
    for(ret = 0; ret < len; ret++)
    {
        int this_reg = ret + reg;
        dataBuf[ret] = i2c_smbus_read_byte_data(fd, this_reg);
        lidbg(DEBG_TAG"[reg.0x%x=0x%x]\n", this_reg, dataBuf[ret]);
    }
    return 0;
}
int lidbg_hal_i2c_write(int fd,  unsigned short slaveAddr, unsigned char *dataBuf, int len)
{
    int ret;
    lidbg(DEBG_TAG"[%s].in slaveAddr:0x%x len:%d\n", __FUNCTION__, slaveAddr, len);
    for(ret = 0; ret < len; ret++)
    {
        lidbg(DEBG_TAG"[%s]. %d = 0x%x:\n", __FUNCTION__, ret, dataBuf[ret]);
    }
    if (ioctl(fd, I2C_SLAVE_FORCE , slaveAddr) < 0)
    {
        lidbg(DEBG_TAG"Could not set address to 0x%02x: %s\n", slaveAddr, strerror(errno));
        return -errno;
    }
    if(1)
    {
        ret = i2c_smbus_write_i2c_block_data(fd, dataBuf[0], len - 1, dataBuf + 1);//first byte is the reg.
    }
    else
    {
        union i2c_smbus_data data;
        struct i2c_smbus_ioctl_data args;
        int i;

        len -= 1;
        dataBuf += 1;

        if (len > I2C_SMBUS_I2C_BLOCK_DATA)
            len = I2C_SMBUS_I2C_BLOCK_DATA;
        for (i = 1; i <= len; i++)
            data.block[i] = dataBuf[i - 1];
        data.block[0] = len;

        args.read_write = I2C_SMBUS_WRITE;
        args.command = dataBuf[0];
        args.size = I2C_SMBUS_I2C_BLOCK_BROKEN;
        args.data = &data;
        ret = ioctl(fd, I2C_SMBUS, &args);
    }
    return ret;
}
int lidbg_hal_i2c_close(int fd)
{
    return close(fd);
};
static HalInterface sLidbgHalInterface =
{
    sizeof(HalInterface),
    lidbg_hal_set_debg_level,
    lidbg_hal_init,
    lidbg_hal_set_path,
    lidbg_hal_start_record,
    lidbg_hal_stop_record,
    lidbg_hal_take_picture,
    lidbg_hal_urgent_record_set_path,
    lidbg_hal_urgent_record_set_times,
    lidbg_hal_urgent_record_ctrl,
    lidbg_hal_urgent_record_get_status,
    lidbg_hal_urgent_record_manual,
    lidbg_hal_is_camera_connect,
    lidbg_hal_i2c_open,
    lidbg_hal_i2c_read,
    lidbg_hal_i2c_write,
    lidbg_hal_i2c_close,
};
const HalInterface *lidbg_hal_interface(struct lidbg_device_t *dev)
{
    dev = dev;
    lidbg(DEBG_TAG"[%s].in\n", __FUNCTION__ );
    return &sLidbgHalInterface;
}

void lidbg_init_globals(void)
{
    pthread_mutex_init(&g_lock, NULL);
    //    pthread_mutex_lock(&g_lock);
    //    pthread_mutex_unlock(&g_lock);

    g_havelidbgNode = (access(OUR_LIDBG_FILE, W_OK) == 0) ? 1 : 0;
    if(!g_havelidbgNode)
        lidbg(DEBG_TAG"[%s]miss node.[%s,%d]\n", __FUNCTION__, OUR_LIDBG_FILE, g_havelidbgNode );
}

static int close_lidbg(struct lidbg_device_t *dev)
{
    thread_exit = 1;
    if (dev)
    {
        lidbg(DEBG_TAG"[%s].free(dev)\n", __FUNCTION__ );
        free(dev);
    }
    lidbg(DEBG_TAG"[%s].close(flycam_fd)\n", __FUNCTION__ );
    close(flycam_fd);
    pthread_mutex_destroy(&g_lock);
    return 0;
}
static int lidbg_module_open(const struct hw_module_t *module, char const *name, struct hw_device_t **device)
{
    struct lidbg_device_t *dev = malloc(sizeof(struct lidbg_device_t));

    lidbg(DEBG_TAG"[%s].[%s]\n", __FUNCTION__ , name == NULL ? "default" : name);

    pthread_once(&g_init, lidbg_init_globals);

    memset(dev, 0, sizeof(*dev));
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int ( *)(struct hw_device_t *))close_lidbg;
    dev->get_hal_interface = lidbg_hal_interface;

    *device = (struct hw_device_t *)dev;
    return 0;
}


static struct hw_module_methods_t lidbg_module_methods =
{
    .open =  lidbg_module_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM =
{
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIDBG_HARDWARE_MODULE_ID,
    .name = "lidbg hal futengfei 2016-03-21 14:42:31",
    .author = "XJS, Inc.",
    .methods = &lidbg_module_methods,
};
