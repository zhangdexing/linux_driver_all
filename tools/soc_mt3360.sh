function soc_make_kernelconfig()
{
	echo $FUNCNAME
}

function soc_build_system()
{
	echo $FUNCNAME
	soc_prebuild && ./selfbuild android
}

function soc_build_kernel()
{
	echo $FUNCNAME
	soc_prebuild && ./selfbuild kernel
}

function soc_build_all()
{
	echo $FUNCNAME
	soc_prebuild && ./selfbuild
}

function soc_prebuild()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR

	cp -r $RELEASE_REPOSITORY/driver/out $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/lib/modules/
	cp $RELEASE_REPOSITORY/app/FastBoot.apk $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
	cp $RELEASE_REPOSITORY/app/FlyBootService.apk $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/app/
	cp $RELEASE_REPOSITORY/driver/out/lidbg_load $DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/system/bin/


	if [[ $TARGET_PRODUCT = "" ]];then
		source ./selfenv&&lunch 5
	fi
}


function soc_build_release()
{

	cd $RELEASE_REPOSITORY
	expect $DBG_TOOLS_PATH/pull master $DBG_REPO_PASSWORD
	cd $DBG_SYSTEM_DIR
	expect $DBG_TOOLS_PATH/pull master $DBG_PASSWORD

	soc_build_all 
}

function soc_make_otapackage()
{
	echo $FUNCNAME
# cp lk,bp to /device/qcom/msm8226/radio
	cd $DBG_SYSTEM_DIR
	if [[ $TARGET_PRODUCT = "" ]];then
		source ./selfenv&&lunch 5
	fi
		./selfbuild makeimage
}
. $DBG_TOOLS_PATH/soc_common.sh
