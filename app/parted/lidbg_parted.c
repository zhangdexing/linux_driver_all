
#include "lidbg_servicer.h"
#include "lidbg_insmod.h"

typedef enum
{
    GPT_SIEZ_ORIGIN,
    GPT_SIEZ_1024M,
    GPT_SIEZ_1536M,
    GPT_SIEZ_2048M,
    GPT_SIEZ_UNUSED,
} GPT_PARTITION_SIZE;

int main(int argc, char **argv)
{
	int emmc_size = 0;
	int parted_size = GPT_SIEZ_ORIGIN;

	if(argc < 2)
	{
		printf("usage:lidbg_parted emmc_size parted_size\n");
		return -1;
	}

	emmc_size = strtoul(argv[1], 0, 0);
	parted_size = strtoul(argv[2], 0, 0);

	if(parted_size != GPT_SIEZ_ORIGIN)
	{
		lidbg("######### gpt start: emmc_size=%d, parted_size=%d, #########\n",emmc_size, parted_size);
		//system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 print");
		system("umount /cache");
		//rm mmcblk0p29 and mmcblk0p30
		system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 rm 29");
		system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 rm 30");

		if(parted_size == 1024){
			lidbg("######### gpt set userdata to 1G #########\n");
			//create userdata from 1879MB ~ 2953MB
			system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs userdata ext2 1879MB 2953MB");
			if(emmc_size == 16)
				//create flash from 2953MB ~ 15.8GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 2953MB 15.8GB");
			else if(emmc_size == 32)
				//create flash from 2953MB ~ 31.3GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 2953MB 31.3GB");
			else{
				lidbg("######### Unsupported emmc size:%d #########\n",emmc_size);
				return -1;
			}
		}else if(parted_size == 1536){
			lidbg("######### gpt set userdata to 1.5G #########\n");
			//create userdata from 1879MB ~ 3465MB
			system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs userdata ext2 1879MB 3465MB");
			if(emmc_size == 16)
				//create flash from 3465MB ~ 15.8GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 3465MB 15.8GB");
			else if(emmc_size == 32)
				//create flash from 3465MB ~ 31.3GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 3465MB 31.3GB");
			else{
				lidbg("######### Unsupported emmc size:%d #########\n",emmc_size);
				return -1;
			}
		}else if(parted_size == 2048){
			lidbg("######### gpt set userdata to 2G #########\n");
			//create userdata from 1879MB ~ 3977MB
			system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs userdata ext2 1879MB 3977MB");
			if(emmc_size == 16)
				//create flash from 3977MB ~ 15.8GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 3977MB 15.8GB");
			else if(emmc_size == 32)
				//create flash from 3977MB ~ 31.3GB
				system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 mkpartfs flash ext2 3977MB 31.3GB");
			else{
				lidbg("######### Unsupported emmc size:%d #########\n",emmc_size);
				return -1;
			}
		}else{
			lidbg("######### Undefined gpt userdata size:%d #########\n",parted_size);
			return -1;
		}

		system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 name 29 userdata");
		system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 name 30 flash");

		emmc_size = 0;
		parted_size = GPT_SIEZ_ORIGIN;
		//system("/system/lib/modules/out/parted -s /dev/block/mmcblk0 print");
		lidbg("######### gpt set end #########\n");
	}
	return 0;
}
