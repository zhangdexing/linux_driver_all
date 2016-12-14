LOCATE_PATH=`pwd`
echo $LOCATE_PATH
source ../../dbg_cfg.sh
cd ../../build && source env_entry.sh && ./build_cfg.sh $DBG_SOC $BOARD_VERSION $DBG_PLATFORM $DBG_VENDOR

if [ $DBG_PLATFORM = mt3360 ];then
cd $DBG_SYSTEM_DIR/&&source ./selfenv&&lunch 5
elif [ $DBG_PLATFORM = rkpx3 ];then
cd $DBG_SYSTEM_DIR/&&source ./build/envsetup.sh&&lunch rkpx3-$SYSTEM_BUILD_TYPE
elif [ $DBG_SOC = mt35x ];then
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release full_$DBG_PLATFORM $SYSTEM_BUILD_TYPE
else
cd $DBG_SYSTEM_DIR/&&source build/envsetup.sh&&choosecombo release $DBG_PLATFORM $SYSTEM_BUILD_TYPE
fi
mmm $LOCATE_PATH -B
while :;do		
	echo
	echo -e "\033[41;20m 任意键继续，ctrl+c退出 \033[0m"
	read Parameter0
	mmm $LOCATE_PATH -B
done
