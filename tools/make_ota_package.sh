#!/bin/bash
DATE=$(date +%Y%m%d_%H%M)
VERSION_DATE=$(date +-%Y-%m-%d-%H-%M)

if [ $DBG_PLATFORM = msm8909 ];then
	if [ $ANDROID_VERSION = 601 ];then
		BUILD_TYPE=G8II-6.0-alpha
	else
		BUILD_TYPE=G8II-alpha
	fi
	SOURCE_OUT_DIR=$RELEASE_REPOSITORY/out/$BUILD_TYPE

elif [ $DBG_PLATFORM = sabresd_6dq ];then
	BUILD_TYPE=700s_usbcamera
	SOURCE_OUT_DIR=$RELEASE_REPOSITORY/out/8909-usbcamera-alpha

fi

FLYTMP_OUT_DIR=$DBG_SYSTEM_DIR/flyaudio/out
TARGET_OUT_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM
TARGET_FLYAUDIO_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/vendor/flyaudio


get_ready()
{
	cd $DBG_SYSTEM_DIR
	rm -rf $TARGET_FLYAUDIO_DIR/
	rm -rf $FLYTMP_OUT_DIR/
	mkdir -p $FLYTMP_OUT_DIR/
	mkdir -p $FLYTMP_OUT_DIR/flyupdate/
	mkdir -p $TARGET_FLYAUDIO_DIR/
}

copy_source()
{
	cd $RELEASE_REPOSITORY
	expect ./tools/pull
	./make.sh
	cp $SOURCE_OUT_DIR/flyapdata.flp $FLYTMP_OUT_DIR/flyupdate/
}

. $DBG_TOOLS_PATH/debug.sh

make_full_package()
{
	cd $DBG_SYSTEM_DIR
	soc_build_all
	cd $RELEASE_REPOSITORY
	cp -rf $RELEASE_REPOSITORY/systemlib/.   $TARGET_OUT_DIR/system/lib/
	cp -rf $SOURCE_OUT_DIR/flyupdate/flyaudio/. $TARGET_FLYAUDIO_DIR/
	cp $RELEASE_REPOSITORY/flyapdata/carlife/bdcl   $TARGET_OUT_DIR/system/bin/
	cd $DBG_SYSTEM_DIR
	soc_make_otapackage
	cp $TARGET_OUT_DIR/$OTA_PACKAGE_NAME $FLYTMP_OUT_DIR/flyupdate/baseqcom.flb
	cd $FLYTMP_OUT_DIR/flyupdate
	zip -r  $TARGET_OUT_DIR/flyupdate-$BUILD_TYPE-$DATE.fup .
	echo "output:"
	echo -e "\t\033[49;32;1m./out/target/product/$DBG_PLATFORM/flyupdate-$BUILD_TYPE-$DATE.fup\033[0m"
}

make_origin_full_package()
{
	cd $DBG_SYSTEM_DIR
	lidbg_build_all
	soc_build_all

	cp $DBG_ROOT_PATH/conf/init.lidbg.rc           $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/root/init.lidbg.rc
	cp $DBG_OUT_PATH/lidbg_load		       $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/lidbg_load
	cp $DBG_OUT_PATH/vold		       	       $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/vold
	cp -rf $DBG_OUT_PATH                           $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules/out
	cp -rf $DBG_SYSTEM_DIR/origin-app/priv-app/*   $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/priv-app/
	cp -rf $DBG_SYSTEM_DIR/origin-app/app/*        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
	cp $DBG_OUT_PATH/FlyBootService.apk  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/FlyBootService.apk
	cp -rf $DBG_OUT_PATH/FlyBootService  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/FlyBootService
	echo "build_origin" > $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_origin

	#cp -rf $RELEASE_REPOSITORY/systemlib/.   $TARGET_OUT_DIR/system/lib/
	#cp -rf $SOURCE_OUT_DIR/flyupdate/flyaudio/. $TARGET_FLYAUDIO_DIR/
	#cp  $RELEASE_REPOSITORY/flyapdata/carlife/bdcl   $TARGET_OUT_DIR/system/bin/
	rm -rf $TARGET_FLYAUDIO_DIR/
	cd $DBG_SYSTEM_DIR
	soc_make_otapackage
	cp $TARGET_OUT_DIR/$OTA_PACKAGE_NAME $FLYTMP_OUT_DIR/flyupdate/baseqcom.flb
	cp $SOURCE_OUT_DIR/flyupdate/TotalVersion.cypt $FLYTMP_OUT_DIR/flyupdate/
	cd $FLYTMP_OUT_DIR/flyupdate
	zip -r  $TARGET_OUT_DIR/flyupdate-$BUILD_TYPE-$DATE.fup .
	echo "output:"
	echo -e "\t\033[49;32;1m./out/target/product/$DBG_PLATFORM/flyupdate-$BUILD_TYPE-$DATE.fup\033[0m"
}

do_make_ota_package()
{
	clear
	get_ready
	copy_source
	make_full_package
	echo ""
	echo "done."
}

do_make_origin_ota_package()
{
	clear
	get_ready
	copy_source
	make_origin_full_package
	echo ""
	echo "done."
}


