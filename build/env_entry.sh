#=======================================================================================
#	FileName    : 
#	Description : 
#       Date:         2010/04/27
#=======================================================================================



export DBG_ROOT_PATH=`cd ../ && pwd`
source $DBG_ROOT_PATH/build/users_conf.sh
export DBG_BUILD_PATH=$DBG_ROOT_PATH/build
export DBG_TOOLS_PATH=$DBG_ROOT_PATH/tools
export DBG_OUT_PATH=$DBG_ROOT_PATH/out
export DBG_CORE_PATH=$DBG_ROOT_PATH/core
export DBG_SOC_PATH=$DBG_ROOT_PATH/soc
export DBG_DRIVERS_PATH=$DBG_ROOT_PATH/drivers
export DBG_HAL_PATH=$DBG_ROOT_PATH/app
export DBG_PLATFORM_DIR=$DBG_SOC_DIR/$DBG_PLATFORM
export LINUX_android_PATH=$BP_SOURCE_PATH/LINUX/android
export MPSS_BUILD_PATH=$BP_SOURCE_PATH/modem_proc/build/ms
export BOOT_IMAGE_PATH=$BP_SOURCE_PATH/boot_images/build/ms
export ADSP_PATH=$BP_SOURCE_PATH/adsp_proc/build
export RPM_PATH=$BP_SOURCE_PATH/rpm_proc/build
export WCNSS_PATH=$BP_SOURCE_PATH/wcnss_proc/Pronto/bsp/build
export TZ_PATH=$BP_SOURCE_PATH/trustzone_images/build/ms
export UPDATE_INFO=$BP_SOURCE_PATH/common/build
export DEBUG_IMAGE_PATH=$BP_SOURCE_PATH/debug_image/build/ms

#export USE_CCACHE=1

case "$DBG_PLATFORM_ID" in
    	0)
	export DBG_PLATFORM=msm7627a
	export BOARD_VERSION=V2
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	export SYSTEM_BUILD_TYPE=eng
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export DBG_VENDOR=VENDOR_QCOM
	export DBG_SOC=msm8x25;;
    	1)
	export DBG_PLATFORM=msm8625
	export BOARD_VERSION=V4
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
	export SYSTEM_BUILD_TYPE=userdebug
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	export UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8625-ota-eng.*_mmc.zip
	export DBG_SOC=msm8x25;;
    	2)
	export DBG_PLATFORM=msm8226
	export BOARD_VERSION=V3
	export ANDROID_VERSION=442
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P6
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=for_new_lk
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8226-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8x28/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=8x28-release
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=1
	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	#以下命令用于bp v1.0.3
	#MPSS_BUILD_CMD='./build.sh 8626.gen BUILD_ID=AAAAANAZ'
	export MPSS_BUILD_CMD='./build.sh 8626.gen.prod 8626.gps.prod BUILD_VER=01780 -k'
	export BOOTLOADER_BUILD_CMD='./build.sh --prod TARGET_FAMILY=8x26 -j1'
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8x26.sh -j1'
	export DEBUG_IMAGE_CMD='./b8x26.sh TARGET_FAMILY=8x26 sdi BUILD_ID=AAAAANAZ BUILD_VER=54 BUILD_MIN=1 -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8x26 tz -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';; 
	
    	3)
	export DBG_PLATFORM=msm8226
	export BOARD_VERSION=V4
	export ANDROID_VERSION=442
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P6
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
    	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8928
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8226-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8928/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=10
	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
        export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	export MPSS_BUILD_CMD='./build.sh 8926.gen.prod BUILD_ID=DAAAANAZ'
	export BOOTLOADER_BUILD_CMD='./build.sh TARGET_FAMILY=8x26 BUILD_ID=FAAAANAZ -j1'
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8x26.sh -j1'
	export DEBUG_IMAGE_CMD='./b8x26.sh TARGET_FAMILY=8x26 sdi BUILD_ID=AAAAANAZ BUILD_VER=5 BUILD_MIN=1 -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8x26 tz sampleapp tzbsp_no_xpu playready widevine isdbtmm securitytest keymaster commonlib -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';;

    	4)
	export DBG_PLATFORM=msm8974
	export BOARD_VERSION=V2
	export ANDROID_VERSION=444
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P6
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=master
    	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8974
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8974/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=6
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA
	#for bp
	export MPSS_BUILD_CMD='./build.sh 8974.gen.prod -k'
	export BOOTLOADER_BUILD_CMD='./build.sh --prod TARGET_FAMILY=8974 -j1'
	#boot_image can also use command : ./build.sh TARGET_FAMILY=8974 BUILD_ID=AAAAANAZ;
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8974.sh -j1'
	export WCNSS_BUILD_CMD='./wcnss_build.sh 8974 pronto BUILD_ID=SCAQBAZ -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8974 tz sampleapp tzbsp_no_xpu -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';; 
	
   	5)
	export DBG_PLATFORM=mt3360
	export BOARD_VERSION=V1
	export ANDROID_VERSION=422
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
	export SYSTEM_BUILD_TYPE=eng
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/kernel
	export UPDATA_BIN_DIR=$RELEASE_REPOSITORY/driver
	export UPDATA_BASESYSTEM_DIR=$RELEASE_REPOSITORY/basesystem
	export DBG_PASSWORD=gitac8317
	export DBG_REPO_PASSWORD=gitac8317
	export DBG_VENDOR=VENDOR_MTK
	export DBG_SOC=mt3360
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=1;;

    	6)
	export DBG_PLATFORM=msm8226
	export BOARD_VERSION=V3
	export ANDROID_VERSION=502
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P7
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8228_5.0
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8226-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8x28/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=11

	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	export MPSS_BUILD_CMD='./build.sh 8626.gen.prod BUILD_VER=01780 -k'
	export BOOTLOADER_BUILD_CMD='./build.sh TARGET_FAMILY=8x26 BUILD_ID=FAAAANAZ -j1'
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8x26.sh -j1'
	export DEBUG_IMAGE_CMD='./b8x26.sh TARGET_FAMILY=8x26 sdi BUILD_ID=AAAAANAZ BUILD_VER=54 BUILD_MIN=1 -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8x26 tz -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';; 
        7)
        export DBG_PLATFORM=msm8974
        export BOARD_VERSION=V2
	export ANDROID_VERSION=511
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=5.1_new_lk
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8974_5.1
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_QCOM
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=msm8x26
        export TEST_PACKAGE_PATH=//172.168.1.240/8974/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=master
        export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=15
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA
        #for bp
        export MPSS_BUILD_CMD='./build.sh 8974.gen.prod BUILD_VER=00161 -k'
        export BOOTLOADER_BUILD_CMD=' ./build.sh TARGET_FAMILY=8974 BUILD_ID=AAAAANAZ -j1'
        export ADSP_BUILD_CMD='python build.py'
        export RPM_BUILD_CMD='./rpm_proc/build/build_8974.sh -j1'
        export WCNSS_BUILD_CMD='./wcnss_build.sh 8974 pronto BUILD_ID=SCAQBAZ -j1'
        export TZ_BUILD_CMD='./build.sh CHIPSET=msm8974 tz sampleapp tzbsp_no_xpu playready widevine isdbtmm securitytest keymaster commonlib mobicore sse -j1'
	export DEBUG_IMAGE_CMD='./build.sh TARGET_FAMILY=8974 sdi BUILD_ID=AAAAANAZ BUILD_VER=8 BUILD_MIN=1 -j1'
        export UPDATE_BUILD_CMD='python update_common_info.py';;

        8)
        export DBG_PLATFORM=radxa_rock_pro
        export BOARD_VERSION=V1
	export ANDROID_VERSION=442
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
        export JAVA_HOME=$PATHJAVA1P6
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=eng
	export KERNEL_DEFCONFIG=rk3188_box_radxa_rock_pro_hdmi_defconfig
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/kernel
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/px3
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_ROCKCHIP
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=rk3x88
        export TEST_PACKAGE_PATH=//172.168.1.240/px3/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=master
        export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=6;;

        9)
        export DBG_PLATFORM=rkpx3
        export BOARD_VERSION=V2
	export ANDROID_VERSION=444
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
        export JAVA_HOME=$PATHJAVA1P6
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
	export KERNEL_DEFCONFIG=rkpx3_sdk_android-4.4_defconfig
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/uboot
	export DBG_BOOTLOADER_TYPE=UBOOT
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/kernel
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/PX3
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_ROCKCHIP
        export OTA_PACKAGE_NAME=rkpx3-ota-eng.*.zip
        export DBG_SOC=rk3x88
        export TEST_PACKAGE_PATH=//172.168.1.240/px3/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=PX3
        export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=17;;

    	10)
	export DBG_PLATFORM=msm8226
	export BOARD_VERSION=V3
	export ANDROID_VERSION=511
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P7
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8228_5.0
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8226-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8x28/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=11

	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	export MPSS_BUILD_CMD='./build.sh 8626.gen.prod BUILD_VER=01780 -k'
	export BOOTLOADER_BUILD_CMD='./build.sh TARGET_FAMILY=8x26 BUILD_ID=FAAAANAZ -j1'
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8x26.sh -j1'
	export DEBUG_IMAGE_CMD='./b8x26.sh TARGET_FAMILY=8x26 sdi BUILD_ID=AAAAANAZ BUILD_VER=54 BUILD_MIN=1 -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8x26 tz -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';;

    	11)
	export DBG_PLATFORM=msm8909
	export BOARD_VERSION=V2
	export ANDROID_VERSION=511
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P7
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=for_new_bp
	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_BOOTLOADER_TYPE=LK
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8909-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/share/8909/升级包/测试升级包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=msm8909-master
	export RECOVERY_BUILD_BRANCH=msm8909-ota-recovery
        export MAKE_PAKG_NUM=21

	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	export MPSS_BUILD_CMD='./build.sh 8909.genns.prod -k'
	export BOOTLOADER_BUILD_CMD='./build.sh TARGET_FAMILY=8909 --prod -j1'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8909.sh -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8909 tz sampleapp tzbsp_no_xpu playready widevine keymaster commonlib -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';;

    	12)
	export DBG_PLATFORM=msm8226
	export BOARD_VERSION=V3
	export ANDROID_VERSION=444
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P6
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8228_4.4.4
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8226-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/8x28/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=android4.4.4
        export MAKE_PAKG_NUM=17
	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	#以下命令用于bp v1.0.3
	#MPSS_BUILD_CMD='./build.sh 8626.gen BUILD_ID=AAAAANAZ'
	export MPSS_BUILD_CMD='./build.sh 8626.gen.prod 8626.gps.prod BUILD_VER=01780 -k'
	export BOOTLOADER_BUILD_CMD='./build.sh --prod TARGET_FAMILY=8x26 -j1'
	export ADSP_BUILD_CMD='python build.py'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8x26.sh -j1'
	export DEBUG_IMAGE_CMD='./b8x26.sh TARGET_FAMILY=8x26 sdi BUILD_ID=AAAAANAZ BUILD_VER=54 BUILD_MIN=1 -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8x26 tz -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py';;
    13)
	export DBG_PLATFORM=kylin_hummingbird
	export BOARD_VERSION=V1
	export ANDROID_VERSION=444
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P6
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=eng
	export KERNEL_DEFCONFIG=sun7i_defconfig
	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/../lichee/brandy/u-boot-2011.09
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/../lichee/linux-3.4
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/../lichee/linux-3.4
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/tx
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_WINNER
	export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
	export DBG_SOC=tx
	export TEST_PACKAGE_PATH=//172.168.1.240/tx/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
	export MAKE_PAKG_NUM= ;;
    14)
        export DBG_PLATFORM=msm8974
        export BOARD_VERSION=V2
        export ANDROID_VERSION=600
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=msm8974-6.0
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_BOOTLOADER_TYPE=LK
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY/others/8974_6.0
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_QCOM
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=msm8x26
        export TEST_PACKAGE_PATH=//172.168.1.240/8974/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=dev-8974-6.0
        export SYSTEM_WORK_BRANCH=msm8974-6.0
	export RECOVERY_BUILD_BRANCH=msm8974-6.0
        export MAKE_PAKG_NUM=22
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA
        #for bp
        export MPSS_BUILD_CMD='./build.sh 8974.gen.prod BUILD_VER=00161 –k'
        export BOOTLOADER_BUILD_CMD=' ./build.sh TARGET_FAMILY=8974 BUILD_ID=AAAAANAZ -j1'
        export ADSP_BUILD_CMD='python build.py'
        export RPM_BUILD_CMD='./rpm_proc/build/build_8974.sh -j1'
        export WCNSS_BUILD_CMD='./wcnss_build.sh 8974 pronto BUILD_ID=SCAQBAZ -j1'
        export TZ_BUILD_CMD='./build.sh CHIPSET=msm8974 tz sampleapp tzbsp_no_xpu playready widevine isdbtmm securitytest keymaster commonlib mobicore sse -j1'
	export DEBUG_IMAGE_CMD='./build.sh TARGET_FAMILY=8974 sdi BUILD_ID=AAAAANAZ BUILD_VER=8 BUILD_MIN=1 -j1'
        export UPDATE_BUILD_CMD='python update_common_info.py';;
     15)
    	export DBG_PLATFORM=sabresd_6dq
	export BOARD_VERSION=V1
	export ANDROID_VERSION=511
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
	export JAVA_HOME=$PATHJAVA1P7
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=eng
	export KERNEL_DEFCONFIG=imx_v7_android_defconfig
	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/uboot-imx
	export DBG_BOOTLOADER_TYPE=UBOOT
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel_imx
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/kernel_imx
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_NXP
	export OTA_PACKAGE_NAME=sabresd_6dq-ota-20151218-imx6q.zip
	export DBG_SOC=imx6q
	export TEST_PACKAGE_PATH=//172.168.1.240/tx/升级包发布/专项测试包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=master
	export MAKE_PAKG_NUM= ;;

    16)
        export DBG_PLATFORM=evb3561t_66_m0
	export SOC_ARCH=arm64
        export BOARD_VERSION=V1
        export ANDROID_VERSION=601
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/vendor/mediatek/proprietary/bootable/bootloader/lk
	export DBG_BOOTLOADER_TYPE=LK
        export DBG_BOOTLOADER_IMG=lk.bin
        export DBG_BOOTLOADER_PARTITION_NAME=lk
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel-3.18
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_MTK
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=mt35x
        export TEST_PACKAGE_PATH=//172.168.1.240/8974/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=master
        export SYSTEM_WORK_BRANCH=master
        export MAKE_PAKG_NUM=19
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA ;;

    17)
	export DBG_PLATFORM=msm8996
	export SOC_ARCH=arm64
	export BOARD_VERSION=V2
	export ANDROID_VERSION=601
	export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
	export JAVA_HOME=$PATHJAVA1P7
	export JRE_HOME=$JAVA_HOME/jre
	export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
	export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
	export SYSTEM_BUILD_TYPE=userdebug
	export BOOTLOADER_BUILD_BRANCH=msm8996-master
	export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/bootable/bootloader/lk
        export DBG_BOOTLOADER_IMG=emmc_appsboot.mbn
        export DBG_BOOTLOADER_PARTITION_NAME=aboot
	export DBG_BOOTLOADER_TYPE=LK
	export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel
	export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
	export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
	export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
	export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
	export DBG_PASSWORD=git
	export DBG_REPO_PASSWORD=git
	export DBG_VENDOR=VENDOR_QCOM
	export OTA_PACKAGE_NAME=msm8996-ota-eng.*.zip
	export DBG_SOC=msm8x26
	export TEST_PACKAGE_PATH=//172.168.1.240/share/8996/升级包/测试升级包
	export REPOSITORY_WORK_BRANCH=master
	export SYSTEM_WORK_BRANCH=msm8996-master
	export RECOVERY_BUILD_BRANCH=msm8996-master
        export MAKE_PAKG_NUM=21

	export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
	export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
	export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8x26
	export TZ_FILE_PATH=$TZ_PATH/bin/FARAANBA
	export MPSS_BUILD_CMD='./build.sh 8996.genns.prod -k'
	export BOOTLOADER_BUILD_CMD='./build.sh TARGET_FAMILY=8996 --prod -j1'
	export RPM_BUILD_CMD='./rpm_proc/build/build_8996.sh -j1'
	export TZ_BUILD_CMD='./build.sh CHIPSET=msm8996 tz sampleapp tzbsp_no_xpu playready widevine keymaster commonlib -j1'
	export UPDATE_BUILD_CMD='python update_common_info.py' ;;


    18)
        export DBG_PLATFORM=evb3561s_65_m0
	export SOC_ARCH=arm64
        export BOARD_VERSION=V2
        export ANDROID_VERSION=601
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/vendor/mediatek/proprietary/bootable/bootloader/lk
	export DBG_BOOTLOADER_TYPE=LK
        export DBG_BOOTLOADER_IMG=lk.bin
        export DBG_BOOTLOADER_PARTITION_NAME=lk
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel-3.18
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_MTK
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=mt35x
        export TEST_PACKAGE_PATH=//172.168.1.240/8974/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=v2
        export SYSTEM_WORK_BRANCH=mt3561s
        export MAKE_PAKG_NUM=19
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA ;;

    19)
        export DBG_PLATFORM=evb3561sv_w_65_m0
	export SOC_ARCH=arm64
        export BOARD_VERSION=V2
        export ANDROID_VERSION=601
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/vendor/mediatek/proprietary/bootable/bootloader/lk
	export DBG_BOOTLOADER_TYPE=LK
        export DBG_BOOTLOADER_IMG=lk.bin
        export DBG_BOOTLOADER_PARTITION_NAME=lk
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel-3.18
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_MTK
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=mt35x
        export TEST_PACKAGE_PATH=//172.168.1.240/3561/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=mt3561s_release
        export SYSTEM_WORK_BRANCH=mt3561s_release
        export MAKE_PAKG_NUM=19
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
        export TZ_FILE_PATH=$TZ_PATH/bin/AAAAANAA ;;

    20)
        export DBG_PLATFORM=evb3562v_c_66_m0
	export SOC_ARCH=arm64
        export BOARD_VERSION=V2
        export ANDROID_VERSION=601
        export DBG_CROSS_COMPILE=$DBG_SYSTEM_DIR/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
        export JAVA_HOME=$PATHJAVA1P7
        export JRE_HOME=$JAVA_HOME/jre
        export CLASSPATH=$JAVA_HOME/lib:$JRE_HOME/lib:$CLASSPATH
        export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH
        export SYSTEM_BUILD_TYPE=userdebug
        export DBG_BOOTLOADER_DIR=$DBG_SYSTEM_DIR/vendor/mediatek/proprietary/bootable/bootloader/lk
	export DBG_BOOTLOADER_TYPE=LK
        export DBG_BOOTLOADER_IMG=lk.bin
        export DBG_BOOTLOADER_PARTITION_NAME=lk
        export DBG_KERNEL_SRC_DIR=$DBG_SYSTEM_DIR/kernel-3.18
        export DBG_KERNEL_OBJ_DIR=$DBG_SYSTEM_DIR/out/target/product/$DBG_PLATFORM/obj/KERNEL_OBJ
        export UPDATA_BIN_PLATFORM_DIR=$RELEASE_REPOSITORY
        export UPDATA_BIN_DIR=$UPDATA_BIN_PLATFORM_DIR/driver
        export UPDATA_BASESYSTEM_DIR=$UPDATA_BIN_PLATFORM_DIR/basesystem
        export DBG_PASSWORD=git
        export DBG_REPO_PASSWORD=git
        export DBG_VENDOR=VENDOR_MTK
        export OTA_PACKAGE_NAME=msm8974-ota-eng.*.zip
        export DBG_SOC=mt35x
        export TEST_PACKAGE_PATH=//172.168.1.240/3561/升级包发布/专项测试包
        export REPOSITORY_WORK_BRANCH=mt3562_release
        export SYSTEM_WORK_BRANCH=mt3562_release
        export MAKE_PAKG_NUM=22
        export NON_HLOS_FILE_PATH=$BP_SOURCE_PATH/common/build/bin/asic
        export RPM_FILE_PATH=$RPM_PATH/ms/bin/AAAAANAAR
        export SBL_FILE_PATH=$BOOT_IMAGE_PATH/bin/8974
esac 




