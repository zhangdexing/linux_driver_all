#include <debug.h>
#include <string.h>
#include "../soc.h"

extern part_t *mt_part_get_partition(char *name);
extern u64 emmc_read(u32 part_id, u64 offset, void *data, u64 size);

int ptn_read(char *ptn_name, unsigned int offset, unsigned long len, unsigned char *buf)
{
	part_t *part;
	u64 addr;
	if(!strcmp(ptn_name, "logo") || !strcmp(ptn_name, "flyparameter"))
	{
		part = mt_part_get_partition(ptn_name);
		if(part == NULL){
			printk("get partition failed! \n");
			return -1;
		}
		addr = (u64)part->start_sect * BLK_SIZE;
		dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);
		if(emmc_read(part->part_id, addr, buf, len) < 0){
			dprintf(INFO, "ERROR: Cannot read flylogo  logodata:0x%x\n", len);
			return -1;
		}
		return 0;
	}else
		dprintf(INFO, "Please ensure partition %s should be read\n", ptn_name);
	return -1;
}

