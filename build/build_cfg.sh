
source ./env_entry.sh
echo input :"$1" "$2" "$3" "$4"

echo EXTRA_CFLAGS += -DSOC_${1} > $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DSOC_${1} >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -DBOARD_${2} >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DBOARD_${2} >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -DPLATFORM_${3} >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DPLATFORM_${3} >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -D${4} >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -D${4} >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -DPLATFORM_ID_$DBG_PLATFORM_ID >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DPLATFORM_ID_$DBG_PLATFORM_ID >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -DANDROID_VERSION=$ANDROID_VERSION >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DANDROID_VERSION=$ANDROID_VERSION >> $DBG_BUILD_PATH/build_cfg.mk

echo EXTRA_CFLAGS += -DSOC_STRING='\'\"${1}'\'\" >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DSOC_STRING='\'\"${1}'\'\" >> $DBG_BUILD_PATH/build_cfg.mk


echo EXTRA_CFLAGS += -DPLATFORM_STRING='\'\"$DBG_PLATFORM'\'\" >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DPLATFORM_STRING='\'\"$DBG_PLATFORM'\'\" >> $DBG_BUILD_PATH/build_cfg.mk


echo EXTRA_CFLAGS += -DDBG_PLATFORM_ID=$DBG_PLATFORM_ID >> $DBG_BUILD_PATH/build_cfg.mk
echo LOCAL_CFLAGS += -DDBG_PLATFORM_ID=$DBG_PLATFORM_ID >> $DBG_BUILD_PATH/build_cfg.mk
