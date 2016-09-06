source ../dbg_cfg.sh
source ../soc/$DBG_SOC/conf/soc_select_hal
LOCATE_PATH=`pwd`
clear
cd ../build && source env_entry.sh && ./build_cfg.sh  $DBG_SOC $BOARD_VERSION $DBG_PLATFORM $DBG_VENDOR
if [ $DBG_PLATFORM = mt3360 ];then
cd $DBG_SYSTEM_DIR/&&source ./selfenv&&lunch 5  && mmm $DBG_HAL_PATH -B
elif [ $DBG_PLATFORM = evb3561t_66_m0 ];then
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release full_$DBG_PLATFORM $SYSTEM_BUILD_TYPE && mmm $DBG_HAL_PATH -B
else
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE && mmm $DBG_HAL_PATH -B
if [ $? = 0 ]; then
	echo
else
	echo -e "\033[41;37mmake $1 error!!\033[0m"
	read get_key
fi
fi

