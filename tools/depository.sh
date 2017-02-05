. $DBG_TOOLS_PATH/make_ota_package.sh

function depository_request()
{
	echo $FUNCNAME
# while :
# do
#         if [ -s /dev/shm/depository_lock ]
#        then
# 	      echo depository waiting...
# 	      cat /dev/shm/depository_lock
# 	      sleep 1 
# 	else
#               echo working > /dev/shm/depository_lock
#               break
#         fi
# done
}

function depository_release()
{
	echo $FUNCNAME
#   rm /dev/shm/depository_lock
}

function depository_clean()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	git checkout $REPOSITORY_WORK_BRANCH && git reset --hard
}
function depository_clean_test()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
 #      git checkout $REPOSITORY_WORK_BRANCH &&
       git reset --hard
}

function depository_pull()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	git checkout $REPOSITORY_WORK_BRANCH
	expect $DBG_TOOLS_PATH/pull $REPOSITORY_WORK_BRANCH $DBG_REPO_PASSWORD
	expect $DBG_TOOLS_PATH/pull $REPOSITORY_WORK_BRANCH $DBG_REPO_PASSWORD
}

function depository_pull_test()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
#	git checkout $branch_name
	expect $DBG_TOOLS_PATH/pull $branch_name $DBG_REPO_PASSWORD
	expect $DBG_TOOLS_PATH/pull $branch_name $DBG_REPO_PASSWORD
}
function depository_add_push()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	git checkout $REPOSITORY_WORK_BRANCH
	git add .
	git add -f $UPDATA_BIN_DIR
	git commit -am "$1"
	expect $DBG_TOOLS_PATH/push $REPOSITORY_WORK_BRANCH $DBG_REPO_PASSWORD
	expect $DBG_TOOLS_PATH/push $REPOSITORY_WORK_BRANCH $DBG_REPO_PASSWORD
	gitk &
}

function depository_copy_lidbg()
{
	echo $FUNCNAME $DBG_OUT_PATH $UPDATA_BIN_DIR $UPDATA_BIN_PLATFORM_DIR
	git checkout $REPOSITORY_WORK_BRANCH
	cp -rfv $DBG_OUT_PATH  $UPDATA_BIN_DIR
	if [ -s $DBG_OUT_PATH/H264ToMp4Service.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/H264ToMp4Service.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/LidbgCommenLogic.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/LidbgCommenLogic.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	if [ -s $DBG_OUT_PATH/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FlyBootService/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	if [ -s $DBG_OUT_PATH/FastBoot.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FastBoot.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	#cp -r $DBG_OUT_PATH/*.so  $UPDATA_BIN_DIR/hw/
}
function depository_copy_lidbg_test()
{
	echo $FUNCNAME $DBG_OUT_PATH $UPDATA_BIN_DIR $UPDATA_BIN_PLATFORM_DIR
#	git checkout $branch_name
	cp -rfv $DBG_OUT_PATH  $UPDATA_BIN_DIR
	if [ -s $DBG_OUT_PATH/H264ToMp4Service.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/H264ToMp4Service.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	if [ -s $DBG_OUT_PATH/LidbgCommenLogic.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/LidbgCommenLogic.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/LidbgCommenLogic/LidbgCommenLogic.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	if [ -s $DBG_OUT_PATH/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FlyBootService/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FastBoot.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -rfv $DBG_OUT_PATH/FastBoot.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	#cp -r $DBG_OUT_PATH/*.so  $UPDATA_BIN_DIR/hw/
}
function depository_copy_basesystem()
{
	echo $FUNCNAME
	git checkout $REPOSITORY_WORK_BRANCH
	#cp -r $DBG_SYSTEM_DIR/flyaudio/out/*  $UPDATA_BASESYSTEM_DIR/
	cp -v $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$OTA_PACKAGE_NAME $UPDATA_BASESYSTEM_DIR/baseqcom.flb
}

function depository_copy_basesystem_test()
{
	echo $FUNCNAME
#	git checkout $REPOSITORY_WORK_BRANCH
	#cp -r $DBG_SYSTEM_DIR/flyaudio/out/*  $UPDATA_BASESYSTEM_DIR/
	cp -v $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$OTA_PACKAGE_NAME $UPDATA_BASESYSTEM_DIR/baseqcom.flb
}

function depository_copy_bpfile()
{
	echo $FUNCNAME
        cp -v $NON_HLOS_FILE_PATH/NON-HLOS.bin        $UPDATA_BIN_PLATFORM_DIR/radio
        cp -v $RPM_FILE_PATH/rpm.mbn                  $UPDATA_BIN_PLATFORM_DIR/radio  
        cp -v $SBL_FILE_PATH/sbl1.mbn    	           $UPDATA_BIN_PLATFORM_DIR/radio  
        cp -v $TZ_FILE_PATH/tz.mbn    		   $UPDATA_BIN_PLATFORM_DIR/radio
        cp -v $TZ_FILE_PATH/sdi.mbn    		   $UPDATA_BIN_PLATFORM_DIR/radio 
}

function depository_make_package()
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	rm -r $RELEASE_REPOSITORY/out/*
	expect $DBG_TOOLS_PATH/make_package $MAKE_PAKG_NUM
    echo "***********************************"
    echo "$MAKE_PAKG_NUM"
    echo "***********************************"
	nautilus $RELEASE_REPOSITORY/out
}

function make_ota_package()
{
	echo "start make ota-package"
	#do_make_ota_package
        soc_prebuild
	rm -rf   $DBG_SYSTEM_DIR/flyaudio/out
	mkdir -p $DBG_SYSTEM_DIR/flyaudio/out
if [ $DBG_PLATFORM = evb3561sv_w_65_m0 ];then
        cp -f $DBG_TOOLS_PATH/parameters.sh $DBG_SYSTEM_DIR/flyaudio/out/parameters3.sh
        cd $DBG_SYSTEM_DIR
        ./make_new_v3.sh
else
	cp -f $DBG_TOOLS_PATH/parameters.sh $DBG_SYSTEM_DIR/flyaudio/out/parameters.sh
	cd $DBG_SYSTEM_DIR
        ./make_new.sh
fi
	
}

function make_origin_ota_package()
{
	echo "start make ota-package"
	do_make_origin_ota_package
	
}

function push_flyFastBoot_apk
{
	expect $DBG_TOOLS_PATH/remount
	adb push $RELEASE_REPOSITORY/app/FastBoot.apk /system/app/
	adb push $RELEASE_REPOSITORY/app/FlyBootService.apk /system/app/
	adb shell chmod 777 /system/app/FastBoot.apk
	adb shell chmod 777 /system/app/FlyBootService.apk
}
function make_lidbg_udisk_shell_update_package
{
	echo $FUNCNAME
	cd $RELEASE_REPOSITORY
	./make.sh
	mkdir -p $RELEASE_REPOSITORY/out/lidbg_update
	cp -rf $RELEASE_REPOSITORY/out/8909-usbcamera-alpha/flysystem $RELEASE_REPOSITORY/out/lidbg_update/
	cp -rf $RELEASE_REPOSITORY/others/recovery $RELEASE_REPOSITORY/out/lidbg_update/
	cp -rf $RELEASE_REPOSITORY/radio/* $RELEASE_REPOSITORY/out/lidbg_update/
	cp -rf $RELEASE_REPOSITORY/lk/* $RELEASE_REPOSITORY/out/lidbg_update/
        unzip -o $RELEASE_REPOSITORY/basesystem/baseqcom.flb -d $RELEASE_REPOSITORY/out/
	cp -rf $RELEASE_REPOSITORY/out/boot.img $RELEASE_REPOSITORY/out/lidbg_update/
}


function depository_menu()
{
if [[ $RELEASE_REPOSITORY != "" ]];then
	echo $RELEASE_REPOSITORY
	echo [41] copy lidbg out to depository
	echo [42] copy basesystem to depository
	echo [43] make update package
	echo [44] make $DBG_PLATFORM full package
	echo [45] make $DBG_PLATFORM origin full package
	echo [46] make lidbg_udisk_shell update package
fi
}

function depository_handle()
{
	cd $RELEASE_REPOSITORY
	case $1 in
	41)	
		depository_copy_lidbg;;
	42)
		depository_copy_basesystem;;
	43)
		depository_make_package;;
	44)
		make_ota_package;;
	45)
		make_origin_ota_package;;
	46)
		make_lidbg_udisk_shell_update_package;;
#	44)
#		push_flyFastBoot_apk;;
	*)
		echo
	esac
}


