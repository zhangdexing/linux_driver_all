#=======================================================================================
#	FileName    : 
#	Description : 
#       Date:         2010/04/27
#=======================================================================================

source ../dbg_cfg.sh

DBG_ROOT_PATH=`cd ../ && pwd`
DBG_BUILD_PATH=$DBG_ROOT_PATH/build
DBG_TOOLS_PATH=$DBG_ROOT_PATH/tools
DBG_OUT_PATH=$DBG_ROOT_PATH/out
DBG_CORE_PATH=$DBG_ROOT_PATH/core
DBG_SOC_PATH=$DBG_ROOT_PATH/soc
DBG_DRIVERS_PATH=$DBG_ROOT_PATH/drivers
DBG_HAL_PATH=$DBG_ROOT_PATH/hal
DBG_PLATFORM_DIR=$DBG_SOC_DIR/$DBG_PLATFORM


case "$DBG_PLATFORM_ID" in
    	0)
	DBG_PLATFORM=msm7627a
	BOARD_VERSION=V2
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=eng
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	DBG_VENDOR=VENDOR_QCOM
	DBG_SOC=msm8x25;;
    	1)
	DBG_PLATFORM=msm8625
	BOARD_VERSION=V4
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=userdebug
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	DBG_PASSWORD=git
	DBG_REPO_PASSWORD=git
	DBG_VENDOR=VENDOR_QCOM
	OTA_PACKAGE_NAME=msm8625-ota-eng.root_mmc.zip
	DBG_SOC=msm8x25;;
    	2)
	DBG_PLATFORM=msm8226
	BOARD_VERSION=V3
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=userdebug
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	DBG_PASSWORD=git
	DBG_REPO_PASSWORD=git
	DBG_VENDOR=VENDOR_QCOM
	OTA_PACKAGE_NAME=msm8226-ota-eng.root.zip
	DBG_SOC=msm8x26
	TEST_PACKAGE_PATH=//192.168.128.128/8x28/升级包发布/专项测试包
	REPOSITORY_WORK_BRANCH=master
        MAKE_PAKG_NUM=1;;
    	3)
	DBG_PLATFORM=msm8226
	BOARD_VERSION=V4
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=userdebug
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/others/8928/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/others/8928/basesystem
	DBG_PASSWORD=git
	DBG_REPO_PASSWORD=git
	DBG_VENDOR=VENDOR_QCOM
	OTA_PACKAGE_NAME=msm8226-ota-eng.root.zip
	DBG_SOC=msm8x26
	TEST_PACKAGE_PATH=//192.168.128.128/8928/升级包发布/专项测试包
	REPOSITORY_WORK_BRANCH=dev-8928
        MAKE_PAKG_NUM=8;;
    	4)
	DBG_PLATFORM=msm8974
	BOARD_VERSION=V2
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=userdebug
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/others/8974/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/others/8974/basesystem
	DBG_PASSWORD=git
	DBG_REPO_PASSWORD=git
	DBG_VENDOR=VENDOR_QCOM
	OTA_PACKAGE_NAME=msm8974-ota-eng.root.zip
	DBG_SOC=msm8x26
	TEST_PACKAGE_PATH=//192.168.128.128/8974/升级包发布/专项测试包
	REPOSITORY_WORK_BRANCH=master
        MAKE_PAKG_NUM=6;;
   	5)
	DBG_PLATFORM=mt3360
	BOARD_VERSION=V1
	DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
	SYSTEM_BUILD_TYPE=eng
	DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/kernel
	UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	DBG_PASSWORD=gitac8317
	DBG_REPO_PASSWORD=gitac8317
	DBG_VENDOR=VENDOR_MTK
	DBG_SOC=mt3360;;
esac

export REPOSITORY_WORK_BRANCH
export TEST_PACKAGE_PATH
export OTA_PACKAGE_NAME
export DBG_VENDOR
export DBG_ROOT_PATH
export DBG_TOOLS_PATH
export DBG_BUILD_PATH
export DBG_OUT_PATH
export DBG_CORE_PATH
export DBG_SOC_PATH
export DBG_DRIVERS_PATH
export DBG_PLATFORM_DIR
export RELEASE_REPOSITORY

export DBG_SOC
export DBG_PLATFORM
export DBG_SYSTEM_DIR
export UPDATA_BIN_DIR
export UPDATA_BASESYSTEM_DIR
export DBG_KERNEL_SRC_DIR
export DBG_KERNEL_OBJ_DIR
export DBG_CROSS_COMPILE
export BOARD_VERSION
export BUILD_VERSION
export DBG_REPO_PASSWORD
export DBG_PASSWORD
export MAKE_PAKG_NUM
