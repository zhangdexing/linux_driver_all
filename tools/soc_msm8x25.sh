function soc_make_kernelconfig()
{
	echo $FUNCNAME
}

function soc_build_system()
{
	echo $FUNCNAME
	soc_prebuild && make systemimage -j8
}

function soc_build_kernel()
{
	echo $FUNCNAME
	soc_prebuild && make bootimage -j8
}

function soc_build_all()
{
	echo $FUNCNAME
	soc_prebuild && make -j8
}

function soc_prebuild()
{
	echo $FUNCNAME
	cd $DBG_SYSTEM_DIR
	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
	fi
}


function soc_build_release()
{
	echo $FUNCNAME
	soc_build_all 
}


function soc_make_otapackage()
{
	echo $FUNCNAME
}

. $DBG_TOOLS_PATH/soc_common.sh
