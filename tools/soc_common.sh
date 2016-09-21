function soc_flash_kernel()
{
	echo $FUNCNAME
if [ $WORK_REMOTE = 0 ];then
	if [ $DBG_PLATFORM = msm7627a ];then
		echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash boot $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/2kpagenand_images/boot.2knand.img
	elif [ $DBG_VENDOR = VENDOR_ROCKCHIP ];then
		echo $DBG_PLATFORM && sudo $DBG_SYSTEM_DIR/RKTools/linux/Linux_Upgrade_Tool_v1.16/upgrade_tool di boot $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/boot.img
	else
		echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash boot $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/boot.img
	fi
else
	rm -rf $WORK_LOCAL_PATH/out
	mkdir $WORK_LOCAL_PATH/out
if [ $WORK_REMOTE = 1 ];then
	expect $DBG_TOOLS_PATH/scp $WORK_REMOTE_USERNAME $WORK_REMOTE_PASSWORD $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/boot.img $WORK_LOCAL_PATH/out/boot.img
fi
	echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash boot $WORK_LOCAL_PATH/out/boot.img
fi
}

function soc_flash_system()
{
	echo $FUNCNAME
if [ $WORK_REMOTE = 0 ];then
	if [ $DBG_PLATFORM = msm7627a ];then
		echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash system $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/2kpagenand_images/system.2knand.img
	elif [ $DBG_VENDOR = VENDOR_ROCKCHIP ];then
		echo $DBG_PLATFORM && sudo $DBG_SYSTEM_DIR/RKTools/linux/Linux_Upgrade_Tool_v1.16/upgrade_tool di system $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system.img
	else
		echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash system $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system.img
	fi
else
	rm -rf $WORK_LOCAL_PATH/out
	mkdir $WORK_LOCAL_PATH/out
if [ $WORK_REMOTE = 1 ];then
	expect $DBG_TOOLS_PATH/scp $WORK_REMOTE_USERNAME $WORK_REMOTE_PASSWORD $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system.img $WORK_LOCAL_PATH/out/system.img
fi
	echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash system $WORK_LOCAL_PATH/out/system.img
fi

}

function soc_flash_recovery()
{
	echo $FUNCNAME
if [ $WORK_REMOTE = 0 ];then
	if [ $DBG_VENDOR = VENDOR_ROCKCHIP ];then
		echo $DBG_PLATFORM && sudo $DBG_SYSTEM_DIR/RKTools/linux/Linux_Upgrade_Tool_v1.16/upgrade_tool di recovery $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/recovery.img
	else
		echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash recovery $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/recovery.img
	fi
else
	rm -rf $WORK_LOCAL_PATH/out
	mkdir $WORK_LOCAL_PATH/out
if [ $WORK_REMOTE = 1 ];then
	expect $DBG_TOOLS_PATH/scp $WORK_REMOTE_USERNAME $WORK_REMOTE_PASSWORD $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/recovery.img $WORK_LOCAL_PATH/out/recovery.img
fi
	echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash recovery $WORK_LOCAL_PATH/out/recovery.img
fi
}

function soc_flash_bootloader()
{
	echo $FUNCNAME
if [ $WORK_REMOTE = 0 ];then
	echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash $DBG_BOOTLOADER_PARTITION_NAME $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$DBG_BOOTLOADER_IMG
else
	rm -rf $WORK_LOCAL_PATH/out
	mkdir $WORK_LOCAL_PATH/out
if [ $WORK_REMOTE = 1 ];then
	expect $DBG_TOOLS_PATH/scp $WORK_REMOTE_USERNAME $WORK_REMOTE_PASSWORD $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/$DBG_BOOTLOADER_IMG $WORK_LOCAL_PATH/out/$DBG_BOOTLOADER_IMG
fi
	echo $DBG_PLATFORM&& sudo $DBG_TOOLS_PATH/fastboot flash $DBG_BOOTLOADER_PARTITION_NAME $WORK_LOCAL_PATH/out/$DBG_BOOTLOADER_IMG
fi
}

function soc_menu()
{
	echo $DBG_SYSTEM_DIR
	echo [21] make bootimage
	echo [22] make system
	echo [23] make
	echo [24] make otapackage
	echo [25] make release basesystem
	echo [26] make originsystem image
	echo [27] make recovery
	echo [28] make recoveryimage
	echo [29] make bootloader
	echo [30] common
	echo [31] make git commit image
	echo [32] make origin bootimage
	echo [33] make kernerconfig
}

function soc_build_git_commit_image()
{
	echo $DBG_SYSTEM_DIR
	cd $DBG_SYSTEM_DIR

}

function soc_handle()
{
	echo $DBG_SYSTEM_DIR $1 $2 $3 $4
	cd $DBG_SYSTEM_DIR
	case $1 in
	21)	
		soc_build_kernel;;
	22)	
		soc_build_system;;
	23)
		soc_build_all;;
	24)
		soc_make_otapackage;;
	25)
		soc_build_release;;
	26)
		soc_build_origin_image;;
	27)
		soc_build_recovery;;
	28)
		soc_build_recoveryimage;;
	29)
		soc_build_bootloader;;
	30)
		soc_build_common "$2 $3 $4";;
	31)
		soc_build_git_commit_image;;
	32)
		soc_build_origin_bootimage;;
	33)
		soc_make_kernelconfig;;
	*)
		echo
	esac
}


