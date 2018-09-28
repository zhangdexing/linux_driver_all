
source ../build/env_entry.sh
expect $DBG_TOOLS_PATH/remount
adb wait-for-device root
adb wait-for-device root
adb wait-for-device remount
adb shell sync
adb shell rm -rf /data/lidbg
adb shell rm -rf /system/lib/modules/out
adb shell rm -rf /data/out
adb push $DBG_OUT_PATH /flysystem/lib/out
adb push $DBG_OUT_PATH/vold /system/bin/
adb push $DBG_OUT_PATH/lidbg_load /system/bin/
adb push $DBG_OUT_PATH/LidbgCommenLogic.apk /flysystem/app/LidbgCommenLogic.apk
adb push $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk  /flysystem/app/LidbgCommenLogic.apk
adb push $DBG_OUT_PATH/FlyBootService.apk /flysystem/app/FlyBootService.apk
adb push $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  /flysystem/app/FlyBootService.apk
adb push $DBG_HAL_PATH/apk/H264ToMp4Service.apk  /flysystem/app/H264ToMp4Service.apk
adb shell chmod 777 /system/bin/vold
adb shell chmod 777 /system/bin/lidbg_load
. $DBG_TOOLS_PATH/pushfly_$DBG_SOC.sh
adb shell chmod 777 /flysystem/lib/out/*.ko
adb shell chmod 777 /flysystem/app/*.apk
adb shell sync
adb reboot
