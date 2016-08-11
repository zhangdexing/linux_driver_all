
function soc_make_kernelconfig()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	soc_prebuild
	make kernelconfig
}

function soc_build_system()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	soc_prebuild && make systemimage -j16 && soc_postbuild
}

function soc_build_kernel()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	soc_prebuild && soc_build_common 'make bootimage -j16'
}


function soc_build_recovery()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	git checkout $RECOVERY_BUILD_BRANCH
	soc_prebuild && soc_build_common 'make recovery -j16'
}

function soc_build_recoveryimage()
{
	echo $FUNCNAME
	lidbg_build_all
	rm -rf $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/out
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/recovery
	rm -rf $DBG_OUT_PATH/*.apk $DBG_OUT_PATH/ES.ko $DBG_OUT_PATH/ST.ko $DBG_OUT_PATH/mkfs.exfat $DBG_OUT_PATH/GPS.ko $DBG_OUT_PATH/*.so $DBG_OUT_PATH/FlyBootService
        rm -rf $DBG_OUT_PATH/LidbgPmService $DBG_OUT_PATH/SleepTest $DBG_OUT_PATH/bma2x2.ko $DBG_OUT_PATH/lidbg_rgb_led.ko
        rm -rf $DBG_OUT_PATH/sslcapture.ko $DBG_OUT_PATH/uvccam.ko $DBG_OUT_PATH/busybox $DBG_OUT_PATH/Firewall.ko $DBG_OUT_PATH/BugReport.ko 
        rm -rf $DBG_OUT_PATH/CallMessage.ko $DBG_OUT_PATH/vold $DBG_OUT_PATH/camera4hal.ko $DBG_OUT_PATH/mobileTrafficstats.ko $DBG_OUT_PATH/app4haljni.ko 
        rm -rf $DBG_OUT_PATH/LidbgCommenLogic $DBG_OUT_PATH/mc3xxx.ko $DBG_OUT_PATH/lidbg_flycam.ko $DBG_OUT_PATH/lidbg_spi.ko
        rm -rf $DBG_OUT_PATH/mount.ntfs $DBG_OUT_PATH/flysemdriver.ko $DBG_OUT_PATH/mkfs.ntfs $DBG_OUT_PATH/bootanimation $DBG_OUT_PATH/lidbg_gps.ko
        rm -rf $DBG_OUT_PATH/tef6638.ko $DBG_OUT_PATH/saf7741.ko $DBG_OUT_PATH/sound_det.ko
        rm -rf $DBG_OUT_PATH/mount.exfat $DBG_OUT_PATH/fsck.ntfs $DBG_OUT_PATH/lidbg_umount
	cd $DBG_SYSTEM_DIR
	git checkout $RECOVERY_BUILD_BRANCH
	if [ ! -d "$DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/.git/" ]; then
	  echo flyrecovery_file_no_found  start_clone
	  rm -rf $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery
          expect $DBG_TOOLS_PATH/pull_recovery  $DBG_SYSTEM_DIR
	elif [[ -e "$DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/.git/" && ! -f "$DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/test_mode" ]]; then
	  echo flyrecovery_file_found start_pull
	  cd $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery
	  git clean -d -df
	  git reset --hard
	  expect $DBG_TOOLS_PATH/pull master git
	else
	 echo test_mode
        fi
	cp -rf $DBG_OUT_PATH  $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery
	#echo "$(expr $ANDROID_VERSION / 100 )"
	cp $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/Android$(expr $ANDROID_VERSION / 100 )/$DBG_PLATFORM/recovery.conf  $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/out
	soc_prebuild && soc_build_common 'make recoveryimage -j16'
}

function soc_build_common()
{
	echo $FUNCNAME $1 $2 $3
	cd $DBG_SYSTEM_DIR
	set_env && $1 $2 $3
}

function soc_build_all()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR

	soc_prebuild && make -j16 && soc_postbuild
}


function soc_postbuild()
{
	echo $FUNCNAME
	hostname > $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	date >> $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	git log --oneline | sed -n '1,5p' >> $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	#cp $DBG_OUT_PATH/su	$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/xbin/su
	echo "soc_build_all ok"
	#if [ $DBG_PLATFORM = msm8226 ];then	
	#	mmm $DBG_SYSTEM_DIR/system/core/libdiskconfig -B
	#fi
}

function set_env()
{
	echo $FUNCNAME $TARGET_PRODUCT $DBG_PLATFORM $SYSTEM_BUILD_TYPE
#	if [[ $TARGET_PRODUCT != $DBG_PLATFOR ]];then
		echo "do source/choosecombo"
		source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
#	fi
}

function soc_prebuild()
{
	echo $FUNCNAME
	echo $DBG_PLATFORM
	cd $DBG_SYSTEM_DIR

if [ $DBG_PLATFORM = msm8226 ];then
	echo "msm8226"
#	rm -rf $DBG_SYSTEM_DIR/kernel/drivers/flyaudio
#	mkdir -p $DBG_SYSTEM_DIR/kernel/drivers/flyaudio
#	cp -ru $DBG_DRIVERS_PATH/build_in/*	        $DBG_SYSTEM_DIR/kernel/drivers/flyaudio/
#	cp -u $DBG_DRIVERS_PATH/inc/lidbg_interface.h   $DBG_SYSTEM_DIR/kernel/drivers/flyaudio/
#	cp -u $DBG_CORE_PATH/cmn_func.c   $DBG_SYSTEM_DIR/kernel/drivers/flyaudio/
#	cp -u $DBG_CORE_PATH/inc/cmn_func.h   $DBG_SYSTEM_DIR/kernel/drivers/flyaudio/
#	cp -u $DBG_CORE_PATH/inc/lidbg_def.h   $DBG_SYSTEM_DIR/kernel/drivers/flyaudio/
#	cp -u $DBG_SOC_PATH/$DBG_SOC/conf/lidbg.selinux.te   $DBG_SYSTEM_DIR//external/sepolicy/

#	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules/out
#	cp -r $RELEASE_REPOSITORY/driver/out $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules/
#	cp $RELEASE_REPOSITORY/driver/out/vold $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/
#	cp $RELEASE_REPOSITORY/app/FastBoot.apk $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
#	cp $RELEASE_REPOSITORY/app/FlyBootService.apk $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
#	cp $RELEASE_REPOSITORY/driver/out/lidbg_load $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/
fi

	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/root
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/ETC
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/EXECUTABLES/vold_intermediates

	set_env
}


function soc_build_release()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	expect $DBG_TOOLS_PATH/pull $REPOSITORY_WORK_BRANCH $DBG_REPO_PASSWORD
	cd $DBG_SYSTEM_DIR
	expect $DBG_TOOLS_PATH/pull $SYSTEM_WORK_BRANCH $DBG_PASSWORD

	soc_build_all
	soc_make_otapackage
}

function soc_make_otapackage()
{
	echo $FUNCNAME
# cp lk,bp to /device/qcom/msm8226/radio
	#cp -u $RELEASE_REPOSITORY/lk/emmc_appsboot.mbn  $DBG_SYSTEM_DIR/device/qcom/msm8226/radio/
	#cp -u $RELEASE_REPOSITORY/radio/* 	        $DBG_SYSTEM_DIR/device/qcom/msm8226/radio/
	cd $DBG_SYSTEM_DIR

	if [ -d "$DBG_BOOTLOADER_DIR/flyaudio" ]; then
		rm -rf "$DBG_BOOTLOADER_DIR/flyaudio"
	fi

	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
	fi

	make otapackage -j16
}

function soc_build_origin_image()
{
	echo $FUNCNAME
	lidbg_build_all
#	soc_build_recoveryimage
	soc_build_all
if [ $ANDROID_VERSION -ge 600 ];then
	cp $DBG_ROOT_PATH/conf/init.lidbg.new.rc        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/root/init.lidbg.rc
else
	cp $DBG_ROOT_PATH/conf/init.lidbg.rc        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/root/init.lidbg.rc
fi
	cp $DBG_OUT_PATH/lidbg_load		       $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/lidbg_load
	cp $DBG_OUT_PATH/vold		       	       $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/vold
	cp -rf $DBG_OUT_PATH                           $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules/
	cp -rf $DBG_SYSTEM_DIR/origin-app/priv-app/*   $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/priv-app/
	cp -rf $DBG_SYSTEM_DIR/origin-app/app/*        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
	#copy fastboot apk
	#cd $RELEASE_REPOSITORY
	#git checkout $REPOSITORY_WORK_BRANCH
	#cp $DBG_OUT_PATH/FastBoot.apk        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/FastBoot.apk
	cp $DBG_OUT_PATH/FlyBootService.apk  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/FlyBootService.apk
	cp -rf $DBG_OUT_PATH/FlyBootService  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/FlyBootService
	cp $DBG_OUT_PATH/LidbgCommenLogic.apk  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/LidbgCommenLogic.apk
	cp -rf $DBG_OUT_PATH/LidbgCommenLogic  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/LidbgCommenLogic
	echo "build_origin" > $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_origin

	cd $DBG_SYSTEM_DIR
	make otapackage -j16

}


function soc_build_origin_bootimage()
{
	echo $FUNCNAME

	mv $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc  $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc.backup   
	#cp $DBG_ROOT_PATH/conf/init.lidbg.rc        $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
if [ $ANDROID_VERSION -ge 600 ];then
	cp $DBG_ROOT_PATH/conf/init.lidbg.new.rc			$DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
else
	cp $DBG_ROOT_PATH/conf/init.lidbg.rc        			$DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
fi
	soc_build_kernel
	rm $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
	mv $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc.backup   $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc	

}

function soc_build_bootloader()
{
	echo $FUNCNAME
        cd $DBG_SYSTEM_DIR
	git checkout $BOOTLOADER_BUILD_BRANCH
	if [ ! -d "$DBG_BOOTLOADER_DIR/flyaudio" ]; then
		mkdir "$DBG_BOOTLOADER_DIR/flyaudio"
	else
		rm -rf "$DBG_BOOTLOADER_DIR/flyaudio"
		mkdir "$DBG_BOOTLOADER_DIR/flyaudio"
	fi

	cp -rf $DBG_ROOT_PATH/fly_bootloader/* $DBG_BOOTLOADER_DIR/flyaudio
	cp -f $DBG_ROOT_PATH/build/build_cfg.mk $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/EMMC_BOOTLOADER_OBJ/
	echo DEFINES += $(echo BOOTLOADER_$DBG_PLATFORM | tr '[a-z]' '[A-Z]') >> $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	echo DEFINES += $(echo BOOTLOADER_$DBG_VENDOR | tr '[a-z]' '[A-Z]') >> $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	echo DEFINES += $(echo BOOTLOADER_TYPE_$DBG_BOOTLOADER_TYPE | tr '[a-z]' '[A-Z]') >> $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	echo DEFINES += $(echo BOOTLOADER_ID_$DBG_PLATFORM_ID | tr '[a-z]' '[A-Z]') >> $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	set_env
	make aboot -j16
	#git checkout $SYSTEM_WORK_BRANCH
}


. $DBG_TOOLS_PATH/soc_common.sh
