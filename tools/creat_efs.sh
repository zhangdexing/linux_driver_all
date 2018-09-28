function creat_efs()
{
	read -p	"Where is your modem_proc (the whole PATH)  " modem_root_path
	read -p "Where is your fs_image.tar.gz (the whole PATH) " fs_image_path
	read -p "What't the modem BUILD_ID " build_id 

	cp $fs_image_path/fs_image.tar.gz $modem_root_path/core/storage/tools/qdst/ &&
	cd $modem_root_path/core/storage/tools/qdst/ && python QDSTMBN.py fs_image.tar.gz &&
	echo "==================Generate fs_image.tar.gz.mbn successful !=====================" 
	cp $modem_root_path/core/storage/tools/qdst/fs_image.tar.gz.mbn $modem_root_path/core/bsp/efs_image_header/tools &&
	cp $modem_root_path/core/bsp/efs_image_header/build/efs_image_header/qdsp6/$build_id/efs_image_meta.bin $modem_root_path/core/bsp/efs_image_header/tools &&
	cd $modem_root_path/core/bsp/efs_image_header/tools &&
	python efs_image_create.py efs_image_meta.bin fs_image.tar.gz.mbn &&
	cp $modem_root_path/core/bsp/efs_image_header/tools/fs_image.tar.gz.mbn.img $fs_image_path/ &&
	echo " " &&
	echo "====================Generate fs_image.tar.gz.mbn.img successful !====================" &&
	echo " " &&
	ls -l $fs_image_path/fs_image.tar.gz.mbn.img
	echo " "
}
