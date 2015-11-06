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
	cp -ruv $DBG_OUT_PATH  $UPDATA_BIN_DIR
	if [ -s $DBG_OUT_PATH/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FlyBootService/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FastBoot.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FastBoot.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	#cp -r $DBG_OUT_PATH/*.so  $UPDATA_BIN_DIR/hw/
}
function depository_copy_lidbg_test()
{
	echo $FUNCNAME $DBG_OUT_PATH $UPDATA_BIN_DIR $UPDATA_BIN_PLATFORM_DIR
#	git checkout $branch_name
	cp -r $DBG_OUT_PATH  $UPDATA_BIN_DIR
	if [ -s $DBG_OUT_PATH/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FlyBootService/FlyBootService.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FlyBootService/FlyBootService.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi

	if [ -s $DBG_OUT_PATH/FastBoot.apk ]
	    then
		echo $FUNCNAME  $UPDATA_BIN_PLATFORM_DIR
		cp -ruv $DBG_OUT_PATH/FastBoot.apk  $UPDATA_BIN_PLATFORM_DIR/app/
	fi
	#cp -r $DBG_OUT_PATH/*.so  $UPDATA_BIN_DIR/hw/
}
function depository_copy_basesystem()
{
	echo $FUNCNAME
	git checkout $REPOSITORY_WORK_BRANCH
	#cp -r $DBG_SYSTEM_DIR/flyaudio/out/*  $UPDATA_BASESYSTEM_DIR/
	cp $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$OTA_PACKAGE_NAME $UPDATA_BASESYSTEM_DIR/baseqcom.flb
}

function depository_copy_basesystem_test()
{
	echo $FUNCNAME
#	git checkout $REPOSITORY_WORK_BRANCH
	#cp -r $DBG_SYSTEM_DIR/flyaudio/out/*  $UPDATA_BASESYSTEM_DIR/
	cp $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$OTA_PACKAGE_NAME $UPDATA_BASESYSTEM_DIR/baseqcom.flb
}

function depository_copy_bpfile()
{
	echo $FUNCNAME
        cp $NON_HLOS_FILE_PATH/NON-HLOS.bin        $UPDATA_BIN_PLATFORM_DIR/radio
        cp $RPM_FILE_PATH/rpm.mbn                  $UPDATA_BIN_PLATFORM_DIR/radio  
        cp $SBL_FILE_PATH/sbl1.mbn    	           $UPDATA_BIN_PLATFORM_DIR/radio  
        cp $TZ_FILE_PATH/tz.mbn    		   $UPDATA_BIN_PLATFORM_DIR/radio
        cp $TZ_FILE_PATH/sdi.mbn    		   $UPDATA_BIN_PLATFORM_DIR/radio 
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


function push_flyFastBoot_apk
{
	expect $DBG_TOOLS_PATH/remount
	adb push $RELEASE_REPOSITORY/app/FastBoot.apk /system/app/
	adb push $RELEASE_REPOSITORY/app/FlyBootService.apk /system/app/
	adb shell chmod 777 /system/app/FastBoot.apk
	adb shell chmod 777 /system/app/FlyBootService.apk
}

function depository_menu()
{
if [[ $RELEASE_REPOSITORY != "" ]];then
	echo $RELEASE_REPOSITORY
	echo [41] copy lidbg out to depository
	echo [42] copy basesystem to depository
	echo [43] make update package
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
#	44)
#		push_flyFastBoot_apk;;
	*)
		echo
	esac
}


