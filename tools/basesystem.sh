#========注意:您给的参数需按照如下参数给出==========
#1 SYSTEM_PLATFORM SYSTEM_BUILD_TYPE SYSTEM_DIR SYSTEM_DIR_PASSWORD BIN_DIR BIN_DIR_PASSWORD BASESYSTEM_DIR_IN_BIN_DIR GIT_MASTER_BRANCH BIN_GIT_COMMIT_DESCRIPTION
#========注意:如下是一个例子==========
#./basesystem.sh 1 msm8974 userdebug /home/swlee/flyaudio/M8974AAAAANLYD4120 git /home/swlee/flyaudio/8x26-release git /home/swlee/flyaudio/8x26-release/others/8974/basesystem master msm8974-ota-eng.root.zip 一个例子

DIR_LIDBG_PATH=`cd ../ && pwd`
DIR_BUILD_PATH=$DIR_LIDBG_PATH/build
DIR_TOOLS_PATH=$DIR_LIDBG_PATH/tools
DIR_CORE_PATH=$DIR_LIDBG_PATH/core
DIR_DRIVERS_PATH=$DIR_LIDBG_PATH/drivers

SYSTEM_PLATFORM="null"
SYSTEM_BUILD_TYPE="null"
SYSTEM_DIR="null"
SYSTEM_DIR_PASSWORD="null" 
BIN_DIR="null"
BIN_DIR_PASSWORD="null"
BASESYSTEM_DIR_IN_BIN_DIR="null"
BIN_GIT_COMMIT_DESCRIPTION="NULL"
GIT_MASTER_BRANCH="NULL"
OUT_BASESYSTEM_NAME="NULL"
MASTER_BRANCH_SYSTEM="NULL"

function soc_postbuild()
{
	echo ==$FUNCNAME
	git log --oneline | sed -n '1,5p' > $SYSTEM_DIR/out/target/product/$SYSTEM_PLATFORM/system/etc/build_time.conf
}

function soc_prebuild()
{
	echo ==$FUNCNAME $SYSTEM_PLATFORM
	if [ $SYSTEM_PLATFORM = msm8226 ];then
		echo $FUNCNAME $SYSTEM_PLATFORM
		rm -rf $SYSTEM_DIR/kernel/drivers/flyaudio
		mkdir -p $SYSTEM_DIR/kernel/drivers/flyaudio
		cp -rv $DIR_DRIVERS_PATH/build_in/*	        $SYSTEM_DIR/kernel/drivers/flyaudio/
		cp -v $DIR_DRIVERS_PATH/inc/lidbg_interface.h   $SYSTEM_DIR/kernel/drivers/flyaudio/
		cp -v $DIR_CORE_PATH/cmn_func.c   $SYSTEM_DIR/kernel/drivers/flyaudio/
		cp -v $DIR_CORE_PATH/inc/cmn_func.h   $SYSTEM_DIR/kernel/drivers/flyaudio/
		cp -v $DIR_CORE_PATH/inc/lidbg_def.h   $SYSTEM_DIR/kernel/drivers/flyaudio/
	fi
}

#$1-$BIN_DIR $2-$BIN_GIT_COMMIT_DESCRIPTION
function git_add_push()
{
	echo ==$FUNCNAME $1 $2 $3 $4
	cd $1
	git add .
	git commit -am "$SYSTEM_PLATFORM平台:$4"
	expect $DIR_TOOLS_PATH/push "$3" "$2"
	expect $DIR_TOOLS_PATH/push "$3" "$2"
	#gitk &
}

function copy_basesystem_system_to_bin_dir()
{
	echo ==$FUNCNAME $SYSTEM_DIR $SYSTEM_PLATFORM $BASESYSTEM_DIR_IN_BIN_DIR
	#cp -r $SYSTEM_DIR/flyaudio/out/*  $BASESYSTEM_DIR_IN_BIN_DIR/
	cp -f $SYSTEM_DIR/out/target/product/$SYSTEM_PLATFORM/$OUT_BASESYSTEM_NAME $BASESYSTEM_DIR_IN_BIN_DIR/baseqcom.flb
}

function system_dir_build()
{
	echo ====IN.1pull_bin_dir=====$FUNCNAME
	git_pull $BIN_DIR $BIN_DIR_PASSWORD $GIT_MASTER_BRANCH
	
	echo ====IN.2pull_system_dir=====$FUNCNAME
	git_pull $SYSTEM_DIR $SYSTEM_DIR_PASSWORD $MASTER_BRANCH_SYSTEM

	echo ====IN.3choosecombo=====$FUNCNAME
	if [ $SYSTEM_PLATFORM = rkpx3 ];then
              cd $SYSTEM_DIR/kernel/
              make kernel.img -j16
        fi
	cd $SYSTEM_DIR
#	if [[ $TARGET_PRODUCT = "" ]];then
		source build/envsetup.sh&&choosecombo release $SYSTEM_PLATFORM $SYSTEM_BUILD_TYPE
#	fi
	echo ====IN.4make-j8=====$FUNCNAME
	rm -rf ./out/target/product/$SYSTEM_PLATFORM/system
	rm -rf ./out/target/product/$SYSTEM_PLATFORM/root
	soc_prebuild
	make -j8
	soc_postbuild
	echo ====IN5make otapackage -j8=====$FUNCNAME
	make otapackage -j8
}

function git_reset_hard()
{
	echo ==$FUNCNAME $1 $2
	cd $1
	git reset --hard
	git checkout $2
	git reset --hard
}

#$SYSTEM_DIR $SYSTEM_DIR_PASSWORD
function git_pull()
{
	echo ==$FUNCNAME $1 $2
	cd $1
	expect $DIR_TOOLS_PATH/pull $3 $2
}

function show_env()
{
	echo ===============show_env====================
	echo $SYSTEM_PLATFORM
	echo $SYSTEM_BUILD_TYPE
	echo $SYSTEM_DIR
	echo $SYSTEM_DIR_PASSWORD
	echo $BIN_DIR
	echo $BIN_DIR_PASSWORD
	echo $BASESYSTEM_DIR_IN_BIN_DIR
	echo $GIT_MASTER_BRANCH
	echo $BIN_GIT_COMMIT_DESCRIPTION
	echo $MASTER_BRANCH_SYSTEM
	echo ===============show_env====================
}

function save_log()
{
	echo ========$(date)======== >> /dev/shm/basesystem.txt
	echo step=$1 reason=$2 >> /dev/shm/basesystem.txt
	show_env >> /dev/shm/basesystem.txt
}

#step  description
function show_err_save_exit()
{
	save_log "$1" "$2"
	echo "!!!!!!!!!!!!!!!!step:$1!!!!!!!!!!!!!!!!!!!!"
	echo "!!           参数不正确              !!"
	echo "!!	   已停止此次运行            !!"
	echo "!!!!!!!!!!!!!!!!step:$1!!!!!!!!!!!!!!!!!!!!"
	echo [$(date)]========[$2]========
	exit 1
}


function basesystem_launch()
{
	SYSTEM_PLATFORM=$2
	SYSTEM_BUILD_TYPE=$3
	SYSTEM_DIR=$4
	SYSTEM_DIR_PASSWORD=$5
	BIN_DIR=$6
	BIN_DIR_PASSWORD=$7
	BASESYSTEM_DIR_IN_BIN_DIR=$8
	GIT_MASTER_BRANCH=$9
	OUT_BASESYSTEM_NAME=${10}
	MASTER_BRANCH_SYSTEM=${12}
	BIN_GIT_COMMIT_DESCRIPTION="${13} ${14} ${15} ${16} ${17} ${18} ${19} ${20} ${21} ${22} ${23} ${24} ${25} ${26} ${27} ${28} ${29} ${30}"
	show_env
	
	if [ $# -lt 13 ];then
		show_err_save_exit 1 "input num < 13 $#"
	fi
	
	if [ $# == '' ];then
		show_err_save_exit 2 "the last num = null"
	fi

	JDKPath=${11}
	export JAVA_HOME=$JDKPath
	export JRE_HOME=$JAVA_HOME/jre
	echo "=============[JAVA_HOME=$JDKPath]============"

	case $1 in
	1)
		cd $SYSTEM_DIR && git reset --hard origin/$MASTER_BRANCH_SYSTEM &&git_pull $SYSTEM_DIR $SYSTEM_DIR_PASSWORD $MASTER_BRANCH_SYSTEM && git_reset_hard $BIN_DIR $GIT_MASTER_BRANCH&& git_pull $BIN_DIR $BIN_DIR_PASSWORD $GIT_MASTER_BRANCH&&git_pull $BIN_DIR $BIN_DIR_PASSWORD $GIT_MASTER_BRANCH&&
		system_dir_build && git_pull $BIN_DIR $BIN_DIR_PASSWORD $GIT_MASTER_BRANCH&& 
		copy_basesystem_system_to_bin_dir && git_pull $BIN_DIR $BIN_DIR_PASSWORD $GIT_MASTER_BRANCH&& git_add_push $BIN_DIR "$BIN_DIR_PASSWORD" "$GIT_MASTER_BRANCH" "$BIN_GIT_COMMIT_DESCRIPTION";;
	*)
		show_err_save_exit 3 "not find case:$1" 
	esac
}

basesystem_launch $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14} ${15} ${16} ${17} ${18} ${19} ${20} ${21} ${22} ${23} ${24} ${25} ${26} ${27} ${28} ${29} ${30}

