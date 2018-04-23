#=======================================================================================
#	FileName    : 
#	Description : copy images to specified directory when changed
#       Date:         2010/04/27
#=======================================================================================
source ./env_entry.sh

src_dir=$DBG_SOC_PATH/$DBG_SOC
dest_dir=$DBG_OUT_PATH

cp -u $src_dir/*.ko	$dest_dir/	&> /dev/null
cp -u $src_dir/bin/*.ko	$dest_dir/	&> /dev/null
cp $src_dir/conf/*.conf	$dest_dir/	&> /dev/null
cp $DBG_ROOT_PATH/conf/*.conf	$dest_dir/	&> /dev/null
cp $src_dir/conf/*.sh	$dest_dir/	&> /dev/null
cp $DBG_ROOT_PATH/conf/*.sh	$dest_dir/	&> /dev/null

src_dir=$DBG_CORE_PATH
cp -u $src_dir/*.ko	$dest_dir/	&> /dev/null
cp $src_dir/*.conf	$dest_dir/	&> /dev/null

cd $DBG_HAL_PATH
cp -u ./halAndJni/inc/hal_lidbg_commen.h $dest_dir/hal_lidbg_commen.h

cd $DBG_DRIVERS_PATH
cp -u ./inc/lidbg_interface.h $dest_dir/lidbg_interface.conf
cp -ru ./touchscreen/ts_config $dest_dir/  &> /dev/null
cp -ru ./touchscreen/cyttsp4/*.ko $dest_dir/  &> /dev/null
rm $dest_dir/drivers.conf  &> /dev/null
rm $dest_dir/state.conf    &> /dev/null
mkdir $dest_dir/fm1388 &> /dev/null
cp -r ./fm1388/fm1388_config/* $dest_dir/fm1388  &> /dev/null


for each_dir in `ls -l | grep "^d" | awk '{print $NF}'`
	do
	src_dir=$DBG_DRIVERS_PATH/$each_dir
	cp -u $src_dir/*.ko     $dest_dir/	&> /dev/null
	cp $src_dir/*.conf     $dest_dir/	&> /dev/null

if [ -s $src_dir/dbg.confi ]; then
	cat $src_dir/dbg.confi  >> $dest_dir/drivers.conf
fi
if [ -s $src_dir/state.confi ]; then
	cat $src_dir/state.confi  >> $dest_dir/state.conf
fi
done

cd $dest_dir
ls > $dest_dir/release

