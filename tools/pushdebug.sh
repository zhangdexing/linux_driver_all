
source ../build/env_entry.sh
adb wait-for-device root
adb push $DBG_OUT_PATH /data/out
adb shell chmod 777 /data/out/*
adb shell sync
adb reboot
