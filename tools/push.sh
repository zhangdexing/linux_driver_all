
source ../build/env_entry.sh
expect $DBG_TOOLS_PATH/remount
adb wait-for-device root
adb wait-for-device root
adb wait-for-device remount
adb shell sync
adb shell rm -rf /data/lidbg
adb shell rm -rf /flysystem/lib/out
adb shell rm -rf /data/out
adb push $DBG_OUT_PATH /system/lib/modules/out
adb push $DBG_OUT_PATH/vold /system/bin/
adb push $DBG_OUT_PATH/lidbg_load /system/bin/
adb push $DBG_OUT_PATH/LidbgCommenLogic.apk /system/app/LidbgCommenLogic.apk
adb push $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk  /system/app/LidbgCommenLogic.apk
adb push $DBG_OUT_PATH/FlyBootService.apk /system/app/FlyBootService.apk
adb push $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  /system/app/FlyBootService.apk
adb push $DBG_OUT_PATH/FlyBootService  /system/app/FlyBootService
adb push $DBG_HAL_PATH/apk/H264ToMp4Service.apk  /system/app/H264ToMp4Service.apk
adb shell chmod 777 /system/bin/vold
adb shell chmod 777 /system/bin/lidbg_load
adb shell chmod 777 /system/lib/modules/out/*.ko
adb shell sync
adb reboot
