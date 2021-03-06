function soc_make_kernelconfig()
{
	echo $FUNCNAME
	#soc_prebuild
	cd $DBG_KERNEL_SRC_DIR
	if [ ! -d "$DBG_KERNEL_SRC_DIR/out" ]; then
		mkdir "$DBG_KERNEL_SRC_DIR/out"
	fi
	ARCH=arm make O=out imx_v7_android_defconfig
	ARCH=arm make O=out menuconfig
	cp out/.config arch/arm/configs/imx_v7_android_defconfig
	rm -rf out/source
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
	export ARCH=arm
	export CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-

	cd $DBG_SYSTEM_DIR
	soc_prebuild && soc_build_common 'make bootimage -j16'
}


function soc_build_recovery()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	soc_prebuild && soc_build_common 'make recovery -j16'
}

function soc_build_recoveryimage()
{
	echo $FUNCNAME
	lidbg_build_all
	rm -rf $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/out
	rm -rf $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/recovery
	rm -rf $DBG_OUT_PATH/*.apk $DBG_OUT_PATH/ES.ko $DBG_OUT_PATH/ST.ko $DBG_OUT_PATH/mkfs.exfat $DBG_OUT_PATH/GPS.ko $DBG_OUT_PATH/*.so $DBG_OUT_PATH/FlyBootService
        rm -rf $DBG_OUT_PATH/LidbgPmService $DBG_OUT_PATH/SleepTest $DBG_OUT_PATH/build_time.conf $DBG_OUT_PATH/bma2x2.ko $DBG_OUT_PATH/lidbg_rgb_led.ko
	cd $DBG_SYSTEM_DIR
	rm -rf $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/test_mode #disable this special OPS.
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
	cp $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/$DBG_PLATFORM/recovery.conf  $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/out
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
	echo "soc_build_all ok"
}

function set_env()
{
	echo $FUNCNAME $TARGET_PRODUCT $DBG_PLATFORM $SYSTEM_BUILD_TYPE
	cd $DBG_SYSTEM_DIR
	source build/envsetup.sh && lunch $DBG_PLATFORM-$SYSTEM_BUILD_TYPE
}

function soc_prebuild()
{
	echo $FUNCNAME
	echo $DBG_PLATFORM
	cd $DBG_SYSTEM_DIR

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
	cd $DBG_SYSTEM_DIR

	if [ -d "$DBG_BOOTLOADER_DIR/flyaudio" ]; then
		rm -rf "$DBG_BOOTLOADER_DIR/flyaudio"
	fi

	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh && lunch $DBG_PLATFORM-$SYSTEM_BUILD_TYPE
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
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/priv-app
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app
	mkdir -p  $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc
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

function mx6_menu()
{
	echo [0] make imx6q bootloader
	echo [1] make imx6qplus bootloader
}

function soc_select_mx6qsabresdandroid_defconfig()
{
	make mx6qsabresdandroid_defconfig
	make -j32
	cp u-boot.imx $DBG_SYSTEM_DIR/out/target/product/sabresd_6dq/u-boot-imx6q.imx
}

function soc_select_mx6qpsabresdandroid_defconfig()
{
	make mx6qpsabresdandroid_defconfig
	make -j32
	cp u-boot.imx $DBG_SYSTEM_DIR/out/target/product/sabresd_6dq/u-boot-imx6qp.imx
}

function soc_build_bootloader()
{
	echo $FUNCNAME
	mx6_menu
	read -p "Enter your select:" name1

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

	cd $DBG_SYSTEM_DIR/bootable/bootloader/uboot-imx
	export ARCH=arm
	export CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
	make distclean
	case $name1 in
	0)
	        soc_select_mx6qsabresdandroid_defconfig;;
	1)
	        soc_select_mx6qpsabresdandroid_defconfig;;
	*)
	        echo
	esac

}

. $DBG_TOOLS_PATH/soc_common.sh
