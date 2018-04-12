
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/Log.h"

#include <stdio.h>
#include <utils/Log.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <errno.h>


#include "lidbg_servicer.h"
#include "hal_lidbg_commen.h"

#define DEBG_TAG "jni_futengfei."

typedef union
{
    JNIEnv *env;
    void *venv;
} UnionJNIEnvToVoid;
struct Devices
{
    lidbg_device_t *dev;
};

Devices *g_lidbg_devices = NULL;
static const HalInterface *sHalInterface = NULL;
static jobject mCallbacksObj = NULL;
static jmethodID method_call_java_test;
static jmethodID method_call_java_drivers_abnormal_event;

using namespace android;

static void checkAndClearExceptionFromCallback(JNIEnv *env, const char *methodName)
{
    if (env->ExceptionCheck())
    {
        lidbg(DEBG_TAG"An exception was thrown by callback '%s'\n.", methodName);
        LOGE_EX(env);
        env->ExceptionClear();
    }
}
static void test_callback(char *msg)
{
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    jstring extras = env->NewStringUTF(msg);
    if (extras)
    {
        env->CallVoidMethod(mCallbacksObj, method_call_java_test, extras);
        lidbg(DEBG_TAG"[%s].from hal.[%s]\n", __FUNCTION__, msg);
    }
    else
        lidbg(DEBG_TAG"[%s].fail.CallVoidMethod.out of memory\n", __FUNCTION__);
    if (extras)
        env->DeleteLocalRef(extras);
    checkAndClearExceptionFromCallback(env, __FUNCTION__);
}
static pthread_t create_thread_callback(const char *name, void (*start)(void *), void *arg)
{
    return (pthread_t)AndroidRuntime::createJavaThread(name, start, arg);
}
void driver_abnormal_callback(int value)
{
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    env->CallVoidMethod(mCallbacksObj, method_call_java_drivers_abnormal_event, value);
    checkAndClearExceptionFromCallback(env, __FUNCTION__);
};

JniCallbacks sJniCallbacks =
{
    sizeof(JniCallbacks),
    create_thread_callback,
    test_callback,
    driver_abnormal_callback,
};

static lidbg_device_t *get_device(hw_module_t *module, char const *name)
{
    int err;
    hw_device_t *device;
    err = module->methods->open(module, name, &device);
    if (err == 0)
    {
        //lidbg(DEBG_TAG"[%s].open.suc\n", __FUNCTION__);
        return (lidbg_device_t *)device;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].open.fail\n", __FUNCTION__);
        return NULL;
    }
}
static jlong native_init(JNIEnv *env, jobject clazz, jint id)
{
    int err;
    hw_module_t *module;
    if (!mCallbacksObj)
        mCallbacksObj = env->NewGlobalRef(clazz);

    g_lidbg_devices = (Devices *)malloc(sizeof(Devices));
    err = hw_get_module(LIDBG_HARDWARE_MODULE_ID, (hw_module_t const **)&module);
    if (!err)
    {
        g_lidbg_devices->dev = get_device(module, NULL);
        sHalInterface = g_lidbg_devices->dev ->get_hal_interface(g_lidbg_devices->dev);
        if(sHalInterface)
        {
            lidbg(DEBG_TAG"[%s].get_hal_interface.suc,%s\n", __FUNCTION__, LIDBG_HARDWARE_MODULE_ID);
            sHalInterface->hal_init(id, &sJniCallbacks);
        }
        else
            lidbg(DEBG_TAG"[%s].get_hal_interface.fail,%s\n", __FUNCTION__, LIDBG_HARDWARE_MODULE_ID);
    }
    else
    {
        memset(g_lidbg_devices, 0, sizeof(Devices));
        lidbg(DEBG_TAG"[%s].hw_get_module.fail.%s,strerror:%s\n", __FUNCTION__, LIDBG_HARDWARE_MODULE_ID, strerror(-err));
    }
    return (jlong)g_lidbg_devices;
}

static jint native_destroy(JNIEnv * /*env*/, jobject /*clazz*/, jlong /*ptr*/)
{
    if (g_lidbg_devices != NULL)
    {
        if( g_lidbg_devices->dev && g_lidbg_devices->dev->common.close)
            g_lidbg_devices->dev->common.close(&g_lidbg_devices->dev->common);
        lidbg(DEBG_TAG"[%s].in.free(g_lidbg_devices)\n", __FUNCTION__);
        free(g_lidbg_devices);
        return 0;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].in.g_lidbg_devices = NULL\n", __FUNCTION__);
        return -1;
    }
}

static jint Camera_setPath(JNIEnv *env, jobject /*clazz*/, jint id, jstring path )
{
    char *mpath = (char *)env->GetStringUTFChars(path, NULL);
    lidbg(DEBG_TAG"[%s].mpath=%s\n", __FUNCTION__, mpath);

    if(sHalInterface != NULL)
    {
        int ret = sHalInterface->set_path(id, mpath);
        env->ReleaseStringUTFChars(path, mpath);
        return ret;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface,g_lidbg_devices->dev[%s,%s]\n", __FUNCTION__, sHalInterface, g_lidbg_devices->dev);
        return -1;
    }
}
static jint Camera_start_record(JNIEnv * /*env*/, jobject /*clazz*/, jint id)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->start_record(id);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}
static jint Camera_stop_record(JNIEnv * /*env*/, jobject /*clazz*/, jint id)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->stop_record(id);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}
static jint Camera_take_picture(JNIEnv *env, jobject /*clazz*/, jint id, jstring path)
{
    char *mpath = (char *)env->GetStringUTFChars(path, NULL);
    if(sHalInterface != NULL)
    {
        int ret = sHalInterface->take_picture(id, mpath);
        env->ReleaseStringUTFChars(path, mpath);
        return ret;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}

static jint set_debug_level(JNIEnv * /*env*/, jobject /*clazz*/,  jint level)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->set_debug_level(level);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}
static jint urgent_record_camera_setPath(JNIEnv *env, jobject /*clazz*/, jint id, jstring path )
{
    char *mpath = (char *)env->GetStringUTFChars(path, NULL);
    lidbg(DEBG_TAG"[%s].mpath=%s\n", __FUNCTION__, mpath);

    if(sHalInterface != NULL)
    {
        int ret = sHalInterface->urgent_record_set_path(id, mpath);
        env->ReleaseStringUTFChars(path, mpath);
        return ret;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface,g_lidbg_devices->dev[%s,%s]\n", __FUNCTION__, sHalInterface, g_lidbg_devices->dev);
        return -1;
    }
}
static jint urgent_record_camera_setTimes(JNIEnv * /*env*/, jobject /*clazz*/, jint id, jint times_in_S)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->urgent_record_set_times(id, times_in_S);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}

static jstring urgent_record_camera_get_status(JNIEnv *env, jobject /*clazz*/, jint id)
{
    if(sHalInterface != NULL)
    {
        char *status = sHalInterface->urgent_record_get_status(id);
        //lidbg(DEBG_TAG"[%s].status=%s\n", __FUNCTION__, status);
        return env->NewStringUTF(status);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return env->NewStringUTF("sHalInterface = NULL");;
    }
}

static jint urgent_record_camera_ctrl(JNIEnv * /*env*/, jobject /*clazz*/, jint id, jint start_stop)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->urgent_record_ctrl(id, start_stop);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}
static jint urgent_record_camera_manual(JNIEnv * /*env*/, jobject /*clazz*/, jint id, jint start_stop)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->urgent_record_manual(id, start_stop);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
}
static jboolean jni_is_camera_connect(JNIEnv * /*env*/, jobject /*clazz*/, jint id)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->is_camera_connect(id);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return false;
    }
}

static JNINativeMethod methods[] =
{
    { "nativeInit", "(I)J", (void *)native_init },
    { "nativeDestroy", "(J)I", (void *)native_destroy },
    { "setDebugLevel", "(I)I", (void *)set_debug_level },
    { "CameraSetPath", "(ILjava/lang/String;)I", (void *)Camera_setPath },
    { "CameraStartRecord", "(I)I", (void *)Camera_start_record },
    { "CameraStopRecord", "(I)I", (void *)Camera_stop_record },
    { "CameraTakePicture", "(ILjava/lang/String;)I", (void *)Camera_take_picture },
    { "UrgentRecordCameraSetPath", "(ILjava/lang/String;)I", (void *)urgent_record_camera_setPath },
    { "UrgentRecordCameraSetTimes", "(II)I", (void *)urgent_record_camera_setTimes },
    { "UrgentRecordCameraCtrl", "(II)I", (void *)urgent_record_camera_ctrl },
    { "UrgentRecordCameraGetStatus", "(I)Ljava/lang/String;", (void *)urgent_record_camera_get_status },
    { "UrgentRecordCameraManual", "(II)I", (void *)urgent_record_camera_manual },
    { "isConnectCamera", "(I)Z", (void *)jni_is_camera_connect },
};

static jint jni_i2c_open  (JNIEnv *env, jobject /*clazz*/, jstring path)
{
    char *mpath = (char *)env->GetStringUTFChars(path, NULL);
    if(sHalInterface != NULL)
    {
        int ret = sHalInterface->i2c_open(mpath);
        env->ReleaseStringUTFChars(path, mpath);
        return ret;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }
};
static jint jni_i2c_read  (JNIEnv *env, jobject /*clazz*/,  jint fd, jint slaveAddr, jintArray bufArr, jint len)
{
    if(sHalInterface != NULL && fd > 0 && len > 0)
    {
        jint *bufInt;
        char *bufByte;
        int res = 0, i = 0, j = 0;

        bufInt = (jint *) malloc(len * sizeof(int));
        if (bufInt == 0)
        {
            lidbg(DEBG_TAG"%s: nomem1", __FUNCTION__);
            return -1;
        }
        bufByte = (char *) malloc(len);
        if (bufByte == 0)
        {
            lidbg(DEBG_TAG"%s: nomem2", __FUNCTION__);
            free(bufInt);
            return -1;
        }

        env->GetIntArrayRegion(bufArr, 0, len, bufInt);
        memset(bufByte, '\0', len);
        for (i = 0; i < len ; i++)
            bufByte[i] = bufInt[i];

        j = sHalInterface->i2c_read( fd, slaveAddr, (unsigned  char *)bufByte,  len);

        //lidbg(DEBG_TAG"%s:%d %d %d %d \n", __FUNCTION__, bufByte[0], bufByte[1], bufByte[2], bufByte[3]);
        for (i = 0; i < len ; i++)
            bufInt[i] = bufByte[i];
        env->SetIntArrayRegion( bufArr, 0, len, bufInt);
        free(bufByte);
        free(bufInt);
        return j;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }

};
static jint jni_i2c_write  (JNIEnv *env, jobject /*clazz*/, jint fd, jint slaveAddr,  jintArray bufArr, jint len)
{
    if(sHalInterface != NULL && fd > 0 && len > 0)
    {
        jint *bufInt;
        char *bufByte;
        int res = 0, i = 0, j = 0;

        bufInt = (jint *) malloc(len * sizeof(int));
        if (bufInt == 0)
        {
            lidbg(DEBG_TAG"%s: nomem1", __FUNCTION__);
            return -1;
        }
        bufByte = (char *) malloc(len);
        if (bufByte == 0)
        {
            lidbg(DEBG_TAG"%s: nomem2", __FUNCTION__);
            free(bufInt);
            return -1;
        }

        env->GetIntArrayRegion( bufArr, 0, len, bufInt);
        memset(bufByte, '\0', len);
        for (i = 0; i < len ; i++)
            bufByte[i]   = bufInt[i];

        j = sHalInterface->i2c_write( fd,  slaveAddr, (unsigned  char *) bufByte,  len);
        free(bufByte);
        free(bufInt);
        return j;
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return -1;
    }

};
static jint jni_i2c_close  (JNIEnv * /*env*/, jobject /*clazz*/,  jint fd)
{
    if(sHalInterface != NULL)
    {
        return sHalInterface->i2c_close(fd);
    }
    else
    {
        lidbg(DEBG_TAG"[%s].sHalInterface:%s\n", __FUNCTION__, sHalInterface);
        return false;
    }
};

static JNINativeMethod I2C_methods[] =
{
    { "IICOpen", "(Ljava/lang/String;)I", (void *)jni_i2c_open },
    { "IICRead", "(II[II)I", (void *)jni_i2c_read },
    { "IICWrite", "(II[II)I", (void *)jni_i2c_write },
    { "IICClose", "(I)I", (void *)jni_i2c_close },
};
//add up below:
static jclass registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL)
    {
        env->ExceptionClear();
        lidbg(DEBG_TAG"[%s].unable to find class '%s'\n", __FUNCTION__, className);
        return clazz;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0)
    {
        lidbg(DEBG_TAG"[%s].RegisterNatives failed for '%s'\n", __FUNCTION__, className);
        return clazz;
    }

    return clazz;
}

const char *LidbgJniNativeclassPathName[] =
{
    "com/android/mypftf99/app4haljni/LidbgJniNative",
    "cn/flyaudio/clientservice/video/LidbgJniNative",
    "com/lidbg/sdk/LidbgJniNative",
    NULL,
};
const char *LidbgIICNativeclassPathName[] =
{
    "com/android/mypftf99/app4haljni/LidbgIICNative",
    "com/lidbg/sdk/LidbgIICNative",
    NULL,
};
static int registerNatives(JNIEnv *env)
{
    int ret = 0, i = 0;

    while(LidbgJniNativeclassPathName[i] != NULL)
    {
        const char *classPathName = LidbgJniNativeclassPathName[i];
        i++;
        jclass clazz = registerNativeMethods(env, classPathName, methods, sizeof(methods) / sizeof(methods[0]));
        if (clazz)
        {
            lidbg(DEBG_TAG"[%s].suc.%s\n", __FUNCTION__, classPathName);
            method_call_java_test = env->GetMethodID(clazz, "hal2jni2appCallBack", "(Ljava/lang/String;)V");
            method_call_java_drivers_abnormal_event = env->GetMethodID(clazz, "driverAbnormalEvent", "(I)V");
            ret++;
        }
        else
        {
            lidbg(DEBG_TAG"[%s].fail.%s\n", __FUNCTION__, classPathName);
        }
    }

    i = 0;
    while(LidbgIICNativeclassPathName[i] != NULL)
    {
        const char *classPathName = LidbgIICNativeclassPathName[i];
        i++;
        jclass clazz = registerNativeMethods(env, classPathName, I2C_methods, sizeof(I2C_methods) / sizeof(I2C_methods[0]));
        if (clazz)
        {
            lidbg(DEBG_TAG"[%s].suc.%s\n", __FUNCTION__, classPathName);
            ret++;
        }
        else
        {
            lidbg(DEBG_TAG"[%s].fail.%s\n", __FUNCTION__, classPathName);
        }
    }

    //result
    lidbg(DEBG_TAG"[%s].ret.%d\n", __FUNCTION__, ret);
    if(ret > 0)
        return JNI_TRUE;
    else
        return JNI_FALSE;
}
jint JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv *env = NULL;

    lidbg(DEBG_TAG"[%s].\n", __FUNCTION__);

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK)
    {
        lidbg(DEBG_TAG"ERROR: GetEnv failed\n");
        goto bail;
    }
    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE)
    {
        lidbg(DEBG_TAG"ERROR: registerNatives failed\n");
        goto bail;
    }
    result = JNI_VERSION_1_4;
bail:
    return result;
}

