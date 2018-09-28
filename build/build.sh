
source ./env_entry.sh
./build_cfg.sh $DBG_SOC $BOARD_VERSION $DBG_PLATFORM $DBG_VENDOR
mkdir -pm 777 $DBG_OUT_PATH

find ../ -name "Module.symvers" -exec rm -rf {} \;
./make_all.sh  && . $DBG_BUILD_PATH/copy2out.sh 


build_log=$DBG_OUT_PATH/build_time.conf
hostname > $build_log
date >> $build_log
echo $DBG_PLATFORM_ID >> $build_log
echo $USERS_ID >> $build_log
echo $DBG_PLATFORM >> $build_log
echo $BOARD_VERSION >> $build_log

git log --oneline | sed -n '1,10p' >> $build_log
#git log --pretty=format:"|%s" | sed -n '1,10p' >> $build_log

#. $DBG_BUILD_PATH/sign_modules.sh

