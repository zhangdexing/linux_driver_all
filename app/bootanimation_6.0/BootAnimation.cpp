/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
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

#define LOG_NDEBUG 0
#define LOG_TAG "BootAnimation"

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <utils/misc.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/select.h>

#include <cutils/properties.h>

#include <androidfw/AssetManager.h>
#include <binder/IPCThreadState.h>
#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>
#include <ui/DisplayInfo.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

// TODO: Fix Skia.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#pragma GCC diagnostic pop

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include <media/AudioSystem.h>
#include <media/mediaplayer.h>
#include <media/IMediaHTTPService.h>

#include "BootAnimation.h"
#include "AudioPlayer.h"

#define OEM_BOOTANIMATION_FILE "/oem/media/bootanimation.zip"
#define SYSTEM_BOOTANIMATION_FILE "/system/media/bootanimation.zip"
#define SYSTEM_ENCRYPTED_BOOTANIMATION_FILE "/system/media/bootanimation-encrypted.zip"

#define OEM_SHUTDOWN_ANIMATION_FILE "/oem/media/shutdownanimation.zip"
#define SYSTEM_SHUTDOWN_ANIMATION_FILE "/system/media/shutdownanimation.zip"
#define SYSTEM_ENCRYPTED_SHUTDOWN_ANIMATION_FILE "/system/media/shutdownanimation-encrypted.zip"

#define OEM_BOOT_MUSIC_FILE "/oem/media/boot.wav"
#define SYSTEM_BOOT_MUSIC_FILE "/system/media/boot.wav"

#define OEM_SHUTDOWN_MUSIC_FILE "/oem/media/shutdown.wav"
#define SYSTEM_SHUTDOWN_MUSIC_FILE "/system/media/shutdown.wav"

#define EXIT_PROP_NAME "service.bootanim.exit"

/*extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                           const struct timespec *request,
                           struct timespec *remain);*/

namespace android {

static void* playMusic(void* arg);

//eidted by yeguanping 2013-7-23
const int part=15;
   GLshort vertices[2*(part+1)*2];
   GLfloat texCoords[2*(part+1)*2];
   GLfloat colorarr[4*(part+1)*2];
   float colors[(part+10)*4];
/**************************/

static const int ANIM_ENTRY_NAME_MAX = 256;

// ---------------------------------------------------------------------------

static pthread_mutex_t mp_lock;
static pthread_cond_t mp_cond;
static bool isMPlayerPrepared = false;
static bool isMPlayerCompleted = false;
static sp<MediaPlayer> mPlayer;

class MPlayerListener : public MediaPlayerListener
{
    void notify(int msg, int ext1, int ext2, const Parcel *obj)
    {
	UNUSED(ext1); 
	UNUSED(ext2); 
	UNUSED(obj); 
        switch (msg) {
        case MEDIA_NOP: // interface test message
            break;
        case MEDIA_PREPARED:
            pthread_mutex_lock(&mp_lock);
            isMPlayerPrepared = true;
            pthread_cond_signal(&mp_cond);
            pthread_mutex_unlock(&mp_lock);
            break;
        case MEDIA_PLAYBACK_COMPLETE:
            pthread_mutex_lock(&mp_lock);
            isMPlayerCompleted = true;
            pthread_cond_signal(&mp_cond);
            pthread_mutex_unlock(&mp_lock);
            break;
        default:
            break;
        }
    }
};

static long getFreeMemory(void)
{
    int fd = open("/proc/meminfo", O_RDONLY);
    const char* const sums[] = { "MemFree:", "Cached:", NULL };
    const int sumsLen[] = { (const int)strlen("MemFree:"), (const int)strlen("Cached:"), 0 };
    int num = 2;

    if (fd < 0) {
        ALOGW("Unable to open /proc/meminfo");
        return -1;
    }

    char buffer[256];
    const int len = read(fd, buffer, sizeof(buffer)-1);
    close(fd);

    if (len < 0) {
        ALOGW("Unable to read /proc/meminfo");
        return -1;
    }
    buffer[len] = 0;

    size_t numFound = 0;
    long mem = 0;

    char* p = buffer;
    while (*p && numFound < (size_t)num) {
        int i = 0;
        while (sums[i]) {
            if (strncmp(p, sums[i], sumsLen[i]) == 0) {
                p += sumsLen[i];
                while (*p == ' ') p++;
                char* num = p;
                while (*p >= '0' && *p <= '9') p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                    if (*p == 0) p--;
                }
                mem += atoll(num);
                numFound++;
                break;
            }
            i++;
        }
        p++;
    }

    return numFound > 0 ? mem : -1;
}

BootAnimation::BootAnimation() : Thread(false), mZip(NULL)
{
    mSession = new SurfaceComposerClient();
}

BootAnimation::~BootAnimation() {
    if (mZip != NULL) {
        delete mZip;
    }
}

void BootAnimation::onFirstRef() {
    status_t err = mSession->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));
    if (err == NO_ERROR) {
        run("BootAnimation", PRIORITY_DISPLAY);
    }
}

sp<SurfaceComposerClient> BootAnimation::session() const {
    return mSession;
}


void BootAnimation::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
    if (mAudioPlayer != NULL) {
        mAudioPlayer->requestExit();
    }
}

status_t BootAnimation::initTexture(Texture* texture, AssetManager& assets,
        const char* name) {
    Asset* asset = assets.open(name, Asset::ACCESS_BUFFER);
    if (asset == NULL)
        return NO_INIT;
    SkBitmap bitmap;
    SkImageDecoder::DecodeMemory(asset->getBuffer(false), asset->getLength(),
            &bitmap, kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode);
    asset->close();
    delete asset;

    // ensure we can call getPixels(). No need to call unlock, since the
    // bitmap will go out of scope when we return from this method.
    bitmap.lockPixels();

    const int w = bitmap.width();
    const int h = bitmap.height();
    const void* p = bitmap.getPixels();

    GLint crop[4] = { 0, h, w, -h };
    texture->w = w;
    texture->h = h;

    glGenTextures(1, &texture->name);
    glBindTexture(GL_TEXTURE_2D, texture->name);

    switch (bitmap.colorType()) {
        case kAlpha_8_SkColorType:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA,
                    GL_UNSIGNED_BYTE, p);
            break;
        case kARGB_4444_SkColorType:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                    GL_UNSIGNED_SHORT_4_4_4_4, p);
            break;
        case kN32_SkColorType:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                    GL_UNSIGNED_BYTE, p);
            break;
        case kRGB_565_SkColorType:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
                    GL_UNSIGNED_SHORT_5_6_5, p);
            break;
        default:
            break;
    }

    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return NO_ERROR;
}

status_t BootAnimation::initTexture(void* buffer, size_t len)
{
    //StopWatch watch("blah");

    SkBitmap bitmap;
    SkMemoryStream  stream(buffer, len);
    SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
    if (codec != NULL) {
        codec->setDitherImage(false);
        codec->decode(&stream, &bitmap,
                kN32_SkColorType,
                SkImageDecoder::kDecodePixels_Mode);
        delete codec;
    }

    // ensure we can call getPixels(). No need to call unlock, since the
    // bitmap will go out of scope when we return from this method.
    bitmap.lockPixels();

    const int w = bitmap.width();
    const int h = bitmap.height();
    const void* p = bitmap.getPixels();

    GLint crop[4] = { 0, h, w, -h };
    int tw = 1 << (31 - __builtin_clz(w));
    int th = 1 << (31 - __builtin_clz(h));
    if (tw < w) tw <<= 1;
    if (th < h) th <<= 1;

    switch (bitmap.colorType()) {
        case kN32_SkColorType:
            if (tw != w || th != h) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, 0);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, p);
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, p);
            }
            break;

        case kRGB_565_SkColorType:
            if (tw != w || th != h) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, 0);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, p);
            }
            break;
        default:
            break;
    }

    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);

    return NO_ERROR;
}

status_t BootAnimation::readyToRun() {
    mAssets.addDefaultAssets();

    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain));
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status)
        return -1;

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.panel.orientation", value, "0");
    int orient = atoi(value)/90;
    if (orient == eOrientation90 || orient == eOrientation270) {
        int temp = dinfo.h;
        dinfo.h = dinfo.w;
        dinfo.w = temp;
    }

    Rect destRect(dinfo.w, dinfo.h);
    mSession->setDisplayProjection(dtoken, orient, destRect, destRect);

    // create the native surface
    sp<SurfaceControl> control = session()->createSurface(String8("BootAnimation"),
            dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565);

    SurfaceComposerClient::openGlobalTransaction();
    control->setLayer(0x40000000);
    SurfaceComposerClient::closeGlobalTransaction();

    sp<Surface> s = control->getSurface();

    // initialize opengl and egl
    const EGLint attribs[] = {
            EGL_RED_SIZE,   8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE,  8,
            EGL_DEPTH_SIZE, 0,
            EGL_NONE
    };
    EGLint w, h;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    surface = eglCreateWindowSurface(display, config, s.get(), NULL);
    context = eglCreateContext(display, config, NULL, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        return NO_INIT;

    mDisplay = display;
    mContext = context;
    mSurface = surface;
    mWidth = w;
    mHeight = h;
    mFlingerSurfaceControl = control;
    mFlingerSurface = s;

    // If the device has encryption turned on or is in process
    // of being encrypted we show the encrypted boot animation.
    char decrypt[PROPERTY_VALUE_MAX];
    property_get("vold.decrypt", decrypt, "");

    // Use customized resources for boot and showdown animation
    // instead of system predefined boot animation files.
    bool encryptedAnimation = atoi(decrypt) != 0 || !strcmp("trigger_restart_min_framework", decrypt);

    ZipFileRO* zipFile = NULL;
    if ((encryptedAnimation &&
            (access(getAnimationFileName(IMG_ENC), R_OK) == 0) &&
            ((zipFile = ZipFileRO::open(getAnimationFileName(IMG_ENC))) != NULL)) ||

            ((access(getAnimationFileName(IMG_DATA), R_OK) == 0) &&
            ((zipFile = ZipFileRO::open(getAnimationFileName(IMG_DATA))) != NULL)) ||

            ((access(getAnimationFileName(IMG_SYS), R_OK) == 0) &&
            ((zipFile = ZipFileRO::open(getAnimationFileName(IMG_SYS))) != NULL))) {
        mZip = zipFile;
    }

    return NO_ERROR;
}

bool BootAnimation::threadLoop()
{
    bool r;
    // We have no bootanimation file, so we use the stock android logo
    // animation.
    if (mZip == NULL) {
        r = android();
    } else {
        r = movie();
    }

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    eglDestroySurface(mDisplay, mSurface);
    mFlingerSurface.clear();
    mFlingerSurfaceControl.clear();
    eglTerminate(mDisplay);
    IPCThreadState::self()->stopProcess();
    return r;
}

bool BootAnimation::android()
{

	  initGL();
	   initTexture();
	   // initTexture(&mAndroid[0], mAssets, "images/android-logo-mask.png");
	  //  initTexture(&mAndroid[1], mAssets, "images/android-logo-shine.png");

	    // clear screen

	    glClear(GL_COLOR_BUFFER_BIT);
	    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);		//Red
	    eglSwapBuffers(mDisplay, mSurface);
	    printf("my boot");
	    //glEnable(GL_TEXTURE_2D);
	    //glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	    const GLint xc = (mWidth  - mAndroid[0].w) / 2;
	    const GLint yc = (mHeight - mAndroid[0].h) / 2;
	    const Rect updateRect(xc, yc, xc + mAndroid[0].w, yc + mAndroid[0].h);

	    // draw and update only what we need
	//    mFlingerSurface->setSwapRectangle(updateRect);
	  //   eglSwapBuffers(mDisplay, mSurface);

	    glScissor(updateRect.left, mHeight - updateRect.bottom, updateRect.width(),
	            updateRect.height());

	    // Blend state
	    //GLfloat light_position[]={0.0,0.0,5.0,0.0};
	    int start=0;
		float colorline[]={1,1,1,0.2, 1,1,1,1 ,1,1,1,0.2,};

	    printf("patt:----------%d\n",part);
	    do {
	        nsecs_t now = systemTime();
	        //------------
	         //  printf("doing painting:----------%d\n",part);
	            glClear(GL_COLOR_BUFFER_BIT);
	           glColor4f(1.0f, 0.0f, 0.0f, 1.0f);		//Red

		    glEnable(GL_BLEND);
		    glLoadIdentity();
		    glColor4f(1,1,1,1);
		    //DrawTextrueOrigin(mAndroid[0],mAndroid[0].w/2,-mAndroid[0].h/2,0);
		    glEnable(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, mAndroid[0].name);
		    glDrawTexiOES(xc, yc, 0, mAndroid[0].w, mAndroid[0].h);
		    memset(colors,0,(part+10)*4*sizeof(float));
	        memcpy(colors+start*4,colorline,3*4*sizeof(float));

	        DrawTextrueColor(mAndroid[1],part,colors,mAndroid[1].w/2,mAndroid[1].h/2);

	        if (eglSwapBuffers(mDisplay, mSurface) == EGL_FALSE)
	            break;
	        start++;if(start>=part+7){
	          start=0;
	         }
	        // 12fps: don't animate too fast to preserve CPU
	        const nsecs_t sleepTime = 83333 - ns2us(systemTime() - now);
	        if (sleepTime > 0)
	            usleep(sleepTime);
	         checkExit();
	    } while (!exitPending());

	    glDeleteTextures(1, &mAndroid[0].name);
	    glDeleteTextures(1, &mAndroid[1].name);
	    return false;
}


//void BootAnimation::checkExit() {
//    // Allow surface flinger to gracefully request shutdown
//    char value[PROPERTY_VALUE_MAX];
//    property_get(EXIT_PROP_NAME, value, "0");
//    int exitnow = atoi(value);
//    if (exitnow) {
//        requestExit();
//        if (mAudioPlayer != NULL) {
//            mAudioPlayer->requestExit();
//        }
//    }
//}
void BootAnimation::checkExit() {
    // Allow surface flinger to gracefully request shutdown
    char value[PROPERTY_VALUE_MAX];
    property_get(EXIT_PROP_NAME, value, "0");
    int exitnow = atoi(value);
    if (exitnow) {
    	//printf("ygp bootanimation exit\n");
        requestExit();
    }
}



// Parse a color represented as an HTML-style 'RRGGBB' string: each pair of
// characters in str is a hex number in [0, 255], which are converted to
// floating point values in the range [0.0, 1.0] and placed in the
// corresponding elements of color.
//
// If the input string isn't valid, parseColor returns false and color is
// left unchanged.
static bool parseColor(const char str[7], float color[3]) {
    float tmpColor[3];
    for (int i = 0; i < 3; i++) {
        int val = 0;
        for (int j = 0; j < 2; j++) {
            val *= 16;
            char c = str[2*i + j];
            if      (c >= '0' && c <= '9') val += c - '0';
            else if (c >= 'A' && c <= 'F') val += (c - 'A') + 10;
            else if (c >= 'a' && c <= 'f') val += (c - 'a') + 10;
            else                           return false;
        }
        tmpColor[i] = static_cast<float>(val) / 255.0f;
    }
    memcpy(color, tmpColor, sizeof(tmpColor));
    return true;
}

bool BootAnimation::readFile(const char* name, String8& outString)
{
    ZipEntryRO entry = mZip->findEntryByName(name);
    ALOGE_IF(!entry, "couldn't find %s", name);
    if (!entry) {
        return false;
    }

    FileMap* entryMap = mZip->createEntryFileMap(entry);
    mZip->releaseEntry(entry);
    ALOGE_IF(!entryMap, "entryMap is null");
    if (!entryMap) {
        return false;
    }

    outString.setTo((char const*)entryMap->getDataPtr(), entryMap->getDataLength());
    delete entryMap;
    return true;
}

bool BootAnimation::movie()
{
    char value[PROPERTY_VALUE_MAX];
    String8 desString;

    if (!readFile("desc.txt", desString)) {
        return false;
    }
    char const* s = desString.string();

    // Create and initialize an AudioPlayer if we have an audio_conf.txt file
    String8 audioConf;
    if (readFile("audio_conf.txt", audioConf)) {
        mAudioPlayer = new AudioPlayer;
        if (!mAudioPlayer->init(audioConf.string())) {
            ALOGE("mAudioPlayer.init failed");
            mAudioPlayer = NULL;
        }
    }

    Animation animation;

    // Parse the description file
    for (;;) {
        const char* endl = strstr(s, "\n");
        if (endl == NULL) break;
        String8 line(s, endl - s);
        const char* l = line.string();
        int fps, width, height, count, pause;
        char path[ANIM_ENTRY_NAME_MAX];
        char color[7] = "000000"; // default to black if unspecified

        char pathType;
        if (sscanf(l, "%d %d %d", &width, &height, &fps) == 3) {
            // ALOGD("> w=%d, h=%d, fps=%d", width, height, fps);
            animation.width = width;
            animation.height = height;
            animation.fps = fps;
        }
        else if (sscanf(l, " %c %d %d %s #%6s", &pathType, &count, &pause, path, color) >= 4) {
            // ALOGD("> type=%c, count=%d, pause=%d, path=%s, color=%s", pathType, count, pause, path, color);
            Animation::Part part;
            part.playUntilComplete = pathType == 'c';
            part.count = count;
            part.pause = pause;
            part.path = path;
            part.audioFile = NULL;
            if (!parseColor(color, part.backgroundColor)) {
                ALOGE("> invalid color '#%s'", color);
                part.backgroundColor[0] = 0.0f;
                part.backgroundColor[1] = 0.0f;
                part.backgroundColor[2] = 0.0f;
            }
            animation.parts.add(part);
        }

        s = ++endl;
    }

    // read all the data structures
    const size_t pcount = animation.parts.size();
    void *cookie = NULL;
    if (!mZip->startIteration(&cookie)) {
        return false;
    }

    ZipEntryRO entry;
    char name[ANIM_ENTRY_NAME_MAX];
    while ((entry = mZip->nextEntry(cookie)) != NULL) {
        const int foundEntryName = mZip->getEntryFileName(entry, name, ANIM_ENTRY_NAME_MAX);
        if (foundEntryName > ANIM_ENTRY_NAME_MAX || foundEntryName == -1) {
            ALOGE("Error fetching entry file name");
            continue;
        }

        const String8 entryName(name);
        const String8 path(entryName.getPathDir());
        const String8 leaf(entryName.getPathLeaf());
        if (leaf.size() > 0) {
            for (size_t j=0 ; j<pcount ; j++) {
                if (path == animation.parts[j].path) {
                    uint16_t method;
                    // supports only stored png files
                    if (mZip->getEntryInfo(entry, &method, NULL, NULL, NULL, NULL, NULL)) {
                        if (method == ZipFileRO::kCompressStored) {
                            FileMap* map = mZip->createEntryFileMap(entry);
                            if (map) {
                                Animation::Part& part(animation.parts.editItemAt(j));
                                if (leaf == "audio.wav") {
                                    // a part may have at most one audio file
                                    part.audioFile = map;
                                } else {
                                    Animation::Frame frame;
                                    frame.name = leaf;
                                    frame.map = map;
                                    part.frames.add(frame);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    mZip->endIteration(cookie);

    // clear screen
    glShadeModel(GL_FLAT);
    glDisable(GL_DITHER);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(mDisplay, mSurface);

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_TEXTURE_2D);
    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int xc = (mWidth - animation.width) / 2;
    const int yc = ((mHeight - animation.height) / 2);
    //nsecs_t lastFrame = systemTime();
    nsecs_t frameDuration = s2ns(1) / animation.fps;

    Region clearReg(Rect(mWidth, mHeight));
    clearReg.subtractSelf(Rect(xc, yc, xc+animation.width, yc+animation.height));

    pthread_mutex_init(&mp_lock, NULL);
    pthread_cond_init(&mp_cond, NULL);

    property_get("persist.sys.silent", value, "null");
    if (strncmp(value, "1", 1) != 0) {
        playBackgroundMusic();
    }
    for (size_t i=0 ; i<pcount ; i++) {
        const Animation::Part& part(animation.parts[i]);
        const size_t fcount = part.frames.size();
        glBindTexture(GL_TEXTURE_2D, 0);

        /*calculate if we need to runtime save memory
        * condition: runtime free memory is less than the textures that will used.
        * needSaveMem default to be false
        */
        GLint mMaxTextureSize;
        bool needSaveMem = false;
        GLuint mTextureid;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
        //ALOGD("freemem:%ld, %d", getFreeMemory(), mMaxTextureSize);
        if((unsigned long)getFreeMemory() < mMaxTextureSize * mMaxTextureSize * fcount / 1024) {
            ALOGD("Use save memory method, maybe small fps in actual.");
            needSaveMem = true;
            glGenTextures(1, &mTextureid);
            glBindTexture(GL_TEXTURE_2D, mTextureid);
            glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        }

        for (int r=0 ; !part.count || r<part.count ; r++) {
            // Exit any non playuntil complete parts immediately
            if(exitPending() && !part.playUntilComplete)
                break;

            // only play audio file the first time we animate the part
            if (r == 0 && mAudioPlayer != NULL && part.audioFile) {
                mAudioPlayer->playFile(part.audioFile);
            }

            glClearColor(
                    part.backgroundColor[0],
                    part.backgroundColor[1],
                    part.backgroundColor[2],
                    1.0f);

            for (size_t j=0 ; j<fcount && (!exitPending() || part.playUntilComplete) ; j++) {
                const Animation::Frame& frame(part.frames[j]);
                nsecs_t lastFrame = systemTime();

                if (r > 0 && !needSaveMem) {
                    glBindTexture(GL_TEXTURE_2D, frame.tid);
                } else {
                    if (!needSaveMem && part.count != 1) {
                        glGenTextures(1, &frame.tid);
                        glBindTexture(GL_TEXTURE_2D, frame.tid);
                        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    }
                    initTexture(
                            frame.map->getDataPtr(),
                            frame.map->getDataLength());
                }

                if (!clearReg.isEmpty()) {
                    Region::const_iterator head(clearReg.begin());
                    Region::const_iterator tail(clearReg.end());
                    glEnable(GL_SCISSOR_TEST);
                    while (head != tail) {
                        const Rect& r(*head++);
                        glScissor(r.left, mHeight - r.bottom,
                                r.width(), r.height());
                        glClear(GL_COLOR_BUFFER_BIT);
                    }
                    glDisable(GL_SCISSOR_TEST);
                }
		// specify the y center as ceiling((mHeight - animation.height) / 2)
                // which is equivalent to mHeight - (yc + animation.height)
                glDrawTexiOES(xc, mHeight - (yc + animation.height),
                              0, animation.width, animation.height);
                eglSwapBuffers(mDisplay, mSurface);

                nsecs_t now = systemTime();
                nsecs_t delay = frameDuration - (now - lastFrame);
                //ALOGD("%lld, %lld", ns2ms(now - lastFrame), ns2ms(delay));
                lastFrame = now;

                if (delay > 0) {
                    struct timespec spec;
                    spec.tv_sec  = (now + delay) / 1000000000;
                    spec.tv_nsec = (now + delay) % 1000000000;
                    int err;
                    do {
                        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
                    } while (err<0 && errno == EINTR);
                }

                checkExit();
            }

            usleep(part.pause * ns2us(frameDuration));

            // For infinite parts, we've now played them at least once, so perhaps exit
            if(exitPending() && !part.count)
                break;
        }

        // free the textures for this part
        if (!needSaveMem && part.count != 1) {
            for (size_t j=0 ; j<fcount ; j++) {
                const Animation::Frame& frame(part.frames[j]);
                glDeleteTextures(1, &frame.tid);
            }
        }

        if (needSaveMem) {
            glDeleteTextures(1, &mTextureid);
        }

    }

    ALOGD("waiting for media player to complete.");
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5; //timeout after 5s.

    pthread_mutex_lock(&mp_lock);
    while (!isMPlayerCompleted) {
        int err = pthread_cond_timedwait(&mp_cond, &mp_lock, &timeout);
        if (err == ETIMEDOUT) {
            break;
        }
    }
    pthread_mutex_unlock(&mp_lock);
    ALOGD("media player is completed.");

    pthread_cond_destroy(&mp_cond);
    pthread_mutex_destroy(&mp_lock);

    return false;
}

char *BootAnimation::getAnimationFileName(ImageID image)
{
    const char *fileName[2][3] = { { OEM_BOOTANIMATION_FILE,
            SYSTEM_BOOTANIMATION_FILE,
            SYSTEM_ENCRYPTED_BOOTANIMATION_FILE }, {
            OEM_SHUTDOWN_ANIMATION_FILE,
            SYSTEM_SHUTDOWN_ANIMATION_FILE,
            SYSTEM_ENCRYPTED_SHUTDOWN_ANIMATION_FILE} };
    int state;

    state = checkBootState() ? 0 : 1;

    return (char*)fileName[state][image];
}

char *BootAnimation::getBootRingtoneFileName(ImageID image)
{
    if (image == IMG_ENC) {
        return NULL;
    }

    const char *fileName[2][2] = { { OEM_BOOT_MUSIC_FILE,
            SYSTEM_BOOT_MUSIC_FILE }, {
            OEM_SHUTDOWN_MUSIC_FILE,
            SYSTEM_SHUTDOWN_MUSIC_FILE } };
    int state;

    state = checkBootState() ? 0 : 1;

    return (char*)fileName[state][image];
}


void BootAnimation::playBackgroundMusic(void)
{
    //Shutdown music is playing in ShutdownThread.java
    if (!checkBootState()) {
        return;
    }

    /* Make sure sound cards are populated */
    FILE* fp = NULL;
    if ((fp = fopen("/proc/asound/cards", "r")) == NULL) {
        ALOGW("Cannot open /proc/asound/cards file to get sound card info.");
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("qcom.audio.init", value, "null");
    if (strncmp(value, "complete", 8) != 0) {
        ALOGW("Audio service is not initiated.");
    }

    fclose(fp);

    char *fileName;
    if (((fileName = getBootRingtoneFileName(IMG_DATA)) != NULL && access(fileName, R_OK) == 0) ||
                ((fileName = getBootRingtoneFileName(IMG_SYS)) != NULL
                && access(fileName, R_OK) == 0)) {
        pthread_t tid;
        pthread_create(&tid, NULL, playMusic, (void *)fileName);
        pthread_join(tid, NULL);
    }
}
bool BootAnimation::checkBootState(void)
{
    char value[PROPERTY_VALUE_MAX];
    bool ret = true;

    property_get("sys.shutdown.requested", value, "null");
    if (strncmp(value, "null", 4) != 0) {
        ret = false;
    }

    return ret;
}

static void* playMusic(void* arg)
{
    arg = arg;
/*
    int index = 0;
    char *fileName = (char *)arg;
    sp<MediaPlayer> mp = new MediaPlayer();
    sp<MPlayerListener> mListener = new MPlayerListener();
    if (mp != NULL) {
        ALOGD("starting to play %s", fileName);
        mp->setListener(mListener);

        if (mp->setDataSource(NULL, fileName, NULL) == NO_ERROR) {
            mp->setAudioStreamType(AUDIO_STREAM_ENFORCED_AUDIBLE);
            mp->prepare();
        } else {
            ALOGE("failed to setDataSource for %s", fileName);
            return NULL;
        }

        //waiting for media player is prepared.
        pthread_mutex_lock(&mp_lock);
        while (!isMPlayerPrepared) {
            pthread_cond_wait(&mp_cond, &mp_lock);
        }
        pthread_mutex_unlock(&mp_lock);

        audio_devices_t device = AudioSystem::getDevicesForStream(AUDIO_STREAM_ENFORCED_AUDIBLE);
        AudioSystem::initStreamVolume(AUDIO_STREAM_ENFORCED_AUDIBLE,0,7);
        AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_ENFORCED_AUDIBLE, 7, device);

        AudioSystem::getStreamVolumeIndex(AUDIO_STREAM_ENFORCED_AUDIBLE, &index, device);
        if (index != 0) {
            ALOGD("playing %s", fileName);
            mp->seekTo(0);
            mp->start();
        } else {
            ALOGW("current volume is zero.");
        }
    }
*/
    return NULL;
}
// ---------------------------------------------------------------------------
//edited by yeguanping
bool BootAnimation::initGL()
{
    glShadeModel(GL_SMOOTH);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);    // ¸æËßÏµÍ³¶ÔÍ¸ÊÓ½øÐÐÐÞÕý
        glClearColor(0.0,0.0,0.0,0.0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        //--------------ÉèÖÃÊÓ¿Ú¼°Í¶Ó°---------------------
        printf("viewport: %d  %d\n",mWidth, mHeight);
        glViewport(0, 0, mWidth, mHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        //gluPerspectivef(VIEW_ANGEL,(GLfloat)viewW/viewH,1,9999);
        glOrthof(-mWidth/2,mWidth/2,-mHeight/2,mHeight/2,-mWidth/2,mWidth/2);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        return true;
}
////
typedef struct flybootloader_header{
	char index[16];
	unsigned int flybootloader_addr;
	unsigned int len;
	int boot_parameter[4];
}flybootloader_header_t;

int OpenDevicesAndReadDate(const char *DevicesName,int len,char* out)
{
	int fb;
	char *buf = (char*)malloc(len+1);
	//flybootloader_header_t* head;
	//chmod(DevicesName,0777);
	fb = open(DevicesName,O_RDONLY);
	printf("fb = %d\n",fb);
	if(fb<0){
		printf("failed:failed to open %s\n",DevicesName);
		free(buf);
		return -1;
	}
	//printf("fb = %d(1)\n",fb);
	if(read(fb,buf,len) != len){
		printf("failed:failed to read %s\n",DevicesName);
		free(buf);
		close(fb);
		return -1;
	}
	//printf("fb = %d(2)\n",fb);
	memcpy(out,buf,len);
	printf("success: success to read date of the %s\n",DevicesName);
	free(buf);
	close(fb);
	return 0;
}
//
int BootAnimation::read_pic(char *buf,unsigned int buflen)
{
	UNUSED(buflen);
        int len;
	const char *devicesname = "/dev/block/platform/msm_sdcc.1/by-name/logo";
        //char *devicesname="/dev/block/platform/msm_sdcc.1/by-name/logo";
	flybootloader_header_t* head = (flybootloader_header_t*)malloc(sizeof(flybootloader_header_t));

	if(OpenDevicesAndReadDate(devicesname,sizeof(flybootloader_header_t),(char *)head)){
		printf("failed:failed to get date head in %s\n",devicesname);
		free(head);
		return -1;
	}
	printf("head->index= %s\n",head->index);
	if (memcmp(head->index, "FLYBOOTLOADER", strlen("FLYBOOTLOADER")))
	{
		printf("ERROR: Invalid image header found !\n");
		free(head);
		return -1;
	}
	len = head->len + sizeof(flybootloader_header_t);
	char *temp = (char*)malloc(len);
	if(OpenDevicesAndReadDate(devicesname,len,temp)){
		printf("failed:failed to get date in %s\n",devicesname);
		free(head);
		free(temp);
		return -1;
	}
	printf("len = %d////head->len= %d\n",len,head->len);
        memcpy(buf,temp+sizeof(flybootloader_header_t),len-sizeof(flybootloader_header_t));
	free(head);
	free(temp);
	return len-sizeof(flybootloader_header_t);

}

unsigned char ED(unsigned char *p,int tw,unsigned char bit)
{
        int temp=0;
                temp+= abs(p[-bit]-p[0]);
                temp+= abs(p[-tw-bit]-p[0]);

                temp+= abs(p[bit]-p[0]);
                temp+= abs(p[tw+bit]-p[0]);
        return temp>255?255:temp;
}
// ---------------------------------------------------------------------------
#define max(a,b) ((a)>(b)?(a):(b))
status_t BootAnimation::initTexture()
{
    char* plogo=(char*)malloc(1024*1024);
    int getlenght=read_pic(plogo,1024*1024);

    sLogo* logodata = (sLogo *)(plogo + 16);
     int data_size = logodata->data_len/2;
    //-----------------------
    printf("get_lenght:-----------------%d\n",getlenght);
    char str[9];memcpy(str,plogo,8);str[8]='\0';
    printf("head_str: -----------------%s\n",str);
    printf("logo info\nx:%d y:%d\nSize_logo:%ld\n",logodata->width,logodata->height,logodata->data_len);
//  return 0;
    //------------------------
    int width=logodata->width;
    int height=logodata->height;
    mAndroid[0].w= mAndroid[1].w=width;
    mAndroid[0].h= mAndroid[1].h=height;
    unsigned char* pdata=(unsigned char*)malloc(width*height*sizeof(unsigned short));
	unsigned short *Fb = (unsigned short *)(plogo+16+sizeof(sLogo)-4);
	int k=0,i,j;
	unsigned short s,s1;
	for(i=0;i<data_size;)
	{
		s = Fb[i++];
		s1 = s;
		s1 &= 0x7fff;
		if(s > 0x8000)
		{
			for(j=0;j<s1;j++)
				((unsigned short*)pdata)[k++]=Fb[i];
			i++;
		}
		else
		{
			for(j=0;j<s1;j++)
				((unsigned short*)pdata)[k++]=Fb[i++];
		}
	}
    //-----------------------------------------
    unsigned long* ptdata=(unsigned long*)malloc(width*height*sizeof(unsigned));
    unsigned long color;
    unsigned short color565;
    printf("backcolor: %d\n",logodata->back_color);
  // logodata->back_color  = 0xffff;
    //---------------------
    unsigned long* lbits=(unsigned long*)malloc(width*height*4);
    for(i=0;i<width*height;i++)
    {
        color565=((unsigned short*)pdata)[i];
        color=(color565 & 0x001f) <<19 |
              (color565 & 0x07e0) <<5 |
              (color565 & 0xf800) >>8;
        if(color!=logodata->back_color)
        {
            ptdata[i]=color | 0xff000000;
            //lbits[i]=0xffffffff;
        }
        else
        {
            ptdata[i]=color;
            //lbits[i]=0x00000000;
        }
    }
    glGenTextures(1, &mAndroid[0].name);
    glBindTexture(GL_TEXTURE_2D, mAndroid[0].name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,GL_RGBA,/*GL_UNSIGNED_SHORT_5_6_5*/GL_UNSIGNED_BYTE, ptdata);
    GLint crop[4] = { 0, height, width, -height };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	memset(lbits,0,width*height*4);
	unsigned char dr,dg,db;
	int wbyte=width*4;
	unsigned char* pb=(unsigned char*)ptdata;

	for (int i=1;i<height-1;i++)
	    for (int j=1;j<width-1;j++)
		{
			dr=ED(pb+i*wbyte+j*4+2,wbyte,4);
			dg=ED(pb+i*wbyte+j*4+1,wbyte,4);
			db=ED(pb+i*wbyte+j*4,wbyte,4);
			unsigned char alpha=max(max(dr,dg),db);
			lbits[i*width+j]=0x00ffffff | (alpha<<24);
		}
	/*
	for (int i=0;i<height;i++)
	   for (int j=0;j<width;j++)
		{
			if(ptdata[i*width + j]!=0x00000000)
			lbits[i*width + j]=0xffffffff;
			else lbits[i*width + j]=0x00000000;
		}
    */
	glGenTextures(1, &mAndroid[1].name);
	glBindTexture(GL_TEXTURE_2D, mAndroid[1].name);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		0, GL_RGBA, GL_UNSIGNED_BYTE, lbits);
	free(lbits);
    free(pdata);
    free(ptdata);
    free(plogo);
    float fd=(float)1/part;
    for(int i=0;i<part+1;i++)
	{
		vertices[i*4+2]=vertices[i*4]=width/2*(i*fd*2-1);
		vertices[i*4+1]=height/2;vertices[i*4+3]=-height/2;

		texCoords[i*4+2]=texCoords[i*4]=i*fd;
		texCoords[i*4+1]=0;texCoords[i*4+3]=1;
	}
    return NO_ERROR;
}

void BootAnimation::DrawTextrueColor(Texture imgobj,int part,GLfloat colors[],int halfw,int halfh)
{
	UNUSED(halfw);
	UNUSED(halfh);

	if(part<1)return;
	/*GLshort* vertices=(GLshort*)malloc((part+1)*2*sizeof(GLshort));
	GLfloat* texCoords=(GLfloat*)malloc((part+1)*2*sizeof(GLfloat));
	GLfloat* colorarr=(GLfloat*)malloc((part+1)*8*sizeof(GLfloat));
	*/
    for(int i=0;i<part+1;i++)
	{
		memcpy(colorarr+i*8,colors+i*4,sizeof(float)*4);
		memcpy(colorarr+i*8+4,colors+(i+1)*4,sizeof(float)*4);
	}
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,imgobj.name);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_SHORT, 0, vertices);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, colorarr);

	glDrawArrays(GL_TRIANGLE_STRIP, 0,(part+1)*2);

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}
//////////////////////////////////////////////
}
; // namespace android
