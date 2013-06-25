#=======================================================================================
#	FileName    : 
#	Description : Make ALL Clean
#       Date:         2012/02/03
#=======================================================================================
source ./env_entry.sh

if [ "$BOARD_VERSION" = "" ]; then
echo "exit:  BOARD_VERSION=null"
exit
fi

./drv_clean.sh
./work_clean.sh
cd $DBG_OUT_PATH && rm *.ko
cd $DBG_OUT_RELEASE_PATH && rm -rf $BOARD_VERSION
cd $DBG_ANDROID_PATH && rm -rf out

