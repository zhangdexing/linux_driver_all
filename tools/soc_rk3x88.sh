function soc_make_kernelconfig()
{
	echo $FUNCNAME
}

function soc_build_system()
{
	echo $FUNCNAME
        soc_prebuild && make systemimage -j16 && soc_postbuild
}

function soc_build_kernel()
{
	echo $FUNCNAME
        cd $DBG_SYSTEM_DIR/kernel
        make kernel.img -j16
        soc_prebuild && make bootimage -j16
}


function soc_build_recovery()
{
	echo $FUNCNAME
	soc_prebuild && soc_build_common 'make recovery -j16'
}

function soc_build_recoveryimage()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	rm -rf $DBG_OUT_PATH/*.apk $DBG_OUT_PATH/ES.ko $DBG_OUT_PATH/ST.ko $DBG_OUT_PATH/mkfs.exfat $DBG_OUT_PATH/GPS.ko $DBG_OUT_PATH/*.so $DBG_OUT_PATH/FlyBootService
	if [ ! -d "$DBG_SYSTEM_DIR/bootable/recovery/flyRecovery" ]; then
	  echo flyrecovery_file_no_found  start_clone
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
        cp $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/$DBG_PLATFORM/recovery.conf  $DBG_SYSTEM_DIR/bootable/recovery/flyRecovery/out
	soc_prebuild && soc_build_common 'make recoveryimage -j16'
}

function soc_build_bootloader()
{
	echo $FUNCNAME

	if [ ! -d "$DBG_BOOTLOADER_DIR/flyaudio" ]; then
		mkdir "$DBG_BOOTLOADER_DIR/flyaudio"
	else
		rm -rf "$DBG_BOOTLOADER_DIR/flyaudio"
		mkdir "$DBG_BOOTLOADER_DIR/flyaudio"
	fi

	cp -rf $DBG_ROOT_PATH/fly_bootloader/* $DBG_SYSTEM_DIR/uboot/flyaudio
	cp -f $DBG_ROOT_PATH/build/build_cfg.mk $DBG_BOOTLOADER_DIR/flyaudio/common/build_cfg.mk
	cd $DBG_SYSTEM_DIR/uboot
	make rk30xx -j16
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
        soc_build_bootloader
	soc_build_kernel
	set_env && make -j16 && soc_postbuild
}


function soc_postbuild()
{
	echo $FUNCNAME
	./mkimage.sh ota
	hostname > $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	date >> $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	git log --oneline | sed -n '1,5p' >> $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_time.conf
	echo "soc_build_all ok"
	cp $DBG_SYSTEM_DIR/rockdev/Image-rkpx3/system.img $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/
}


function set_env()
{
	echo $FUNCNAME
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

	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
	fi
        soc_build_all
	make otapackage -j16
}

function soc_build_origin_image()
{
	echo $FUNCNAME
	lidbg_build_all
	soc_build_all

	cp $DBG_SOC_PATH/$DBG_SOC/init.lidbg.rc        $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/root/init.lidbg.rc
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

	echo "build_origin" > $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/etc/build_origin

	cd $DBG_SYSTEM_DIR
	make otapackage -j16 && soc_postbuild
}

function soc_build_origin_bootimage()
{
	echo $FUNCNAME

	mv $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc  $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc.backup   
	cp $DBG_SOC_PATH/$DBG_SOC/init.lidbg.rc        $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
	soc_build_kernel
	rm $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc
	mv $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc.backup   $DBG_SYSTEM_DIR/system/core/rootdir/init.lidbg.rc	

}

. $DBG_TOOLS_PATH/soc_common.sh
