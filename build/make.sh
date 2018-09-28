
#./make.sh drivers/gps/
source env_entry.sh && ./build_cfg.sh $DBG_SOC $BUILD_VERSION $DBG_PLATFORM $DBG_VENDOR
cd $DBG_ROOT_PATH/$1 && make modules
cp -u $DBG_ROOT_PATH/$1/*.ko $DBG_OUT_PATH/
