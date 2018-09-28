#include <debug.h>
#include <string.h>
#include <partition_parser.h>
#include "../soc.h"
#include "../../common/include/lidbg_bare_para.h"

extern unsigned page_mask;

static int read_misc(unsigned page_offset, void *buf, unsigned size)
{
	const char *ptn_name = "misc";
	uint32_t pagesize = get_page_size();
	unsigned offset;

	if (size == 0 || buf == NULL)
		return -1;

	offset = page_offset * pagesize;

	if (target_is_emmc_boot())
	{
		int index;
		unsigned long long ptn;
		unsigned long long ptn_size;

		index = partition_get_index(ptn_name);
		if (index == INVALID_PTN)
		{
			dprintf(CRITICAL, "fmisc:No '%s' partition found\n", ptn_name);
			return -1;
		}

		ptn = partition_get_offset(index);
		ptn_size = partition_get_size(index);

		mmc_set_lun(partition_get_lun(index));

		if (ptn_size < offset + size)
		{
			dprintf(CRITICAL, "fmisc:Read request out of '%s' boundaries\n",
					ptn_name);
			return -1;
		}

		dprintf(INFO, "fmisc:read_misc.in ptn:%llu,ptn_size:%llu offset:%d pagesize:%d\n",ptn,ptn_size,offset,pagesize);
		dprintf(INFO, "fmisc:read_misc.real read\n");
		if (mmc_read(ptn + offset, (unsigned int *)buf, size))
		{
			dprintf(CRITICAL, "fmisc:Reading MMC failed\n");
			return -1;
		}
	}
	else
	{
		dprintf(CRITICAL, "fmisc:Misc partition not supported for NAND targets.\n");
		return -1;
	}

	return 0;
}

int ptn_read(char *ptn_name, unsigned int offset, unsigned long len, unsigned char *buf)
{
    unsigned n = 0;
    int index = INVALID_PTN;
    unsigned long long ptn = 0;
#ifdef BOOTLOADER_MSM8996
    unsigned long long size = 0;
    unsigned char lun = 0;
    index = partition_get_index(ptn_name);
    lun = partition_get_lun(index);
    mmc_set_lun(lun);
    size = partition_get_size(index);
    ptn = partition_get_offset(index);
    if(ptn == 0)
    {
	dprintf(CRITICAL, "partition %s doesn't exist\n", ptn_name);
	return -1;
    }
    dprintf(INFO, "Partition %s index[0x%x],len[%lu],size[%llu]offset:%d\n", ptn_name, index,len,size,offset);

    if(!strcmp(ptn_name, "flyparameter"))
    {
	unsigned char *data = memalign(CACHE_LINE, ROUNDUP(size, CACHE_LINE));
	if(mmc_read((ptn+offset), (unsigned int *)data, size)) {
		dprintf(CRITICAL, "mmc read failure %s %d\n", ptn_name, len);
		return -1;
	}
	memcpy(buf, data, len);
	free(data);
	return 0;
    }
    else if(!strcmp(ptn_name, "logo"))
    {
        n = ROUND_TO_PAGE(len, page_mask);
	if(mmc_read(ptn+offset, (unsigned int *)buf,n)) {
		dprintf(CRITICAL, "mmc read failure %s %d\n", ptn_name, len);
		return -1;
	}
	return 0;
    }
#else
    if(!strcmp(ptn_name, "flyparameter"))
    {
        unsigned int size =  2048;//ROUND_TO_PAGE(sizeof(*in),511);

        index = partition_get_index((unsigned char *) ptn_name);
        ptn = partition_get_offset(index);

        if(ptn == 0)
        {
            dprintf(CRITICAL, "partition %s doesn't exist\n", ptn_name);
            return -1;
        }

        n = len / 2048;
        n = n + 1;
        int i = 0;
        dprintf(INFO, "Partition %s index[0x%x], ptn_offset[%u], page_num[%u]\n", ptn_name, index, ptn, n);
        unsigned char *data = malloc(n * 2048);
        while(n--)
        {
            if (mmc_read((ptn + offset + i * 2048), (unsigned int *)(data + i * 2048), 2048))
            {
                dprintf(CRITICAL, "mmc read failure %s %d\n", ptn_name, len);
                free(data);
                return -1;
            }
            i++;
        }
        memcpy(buf, data, len);
        free(data);
        return 0;
    }
    else if(!strcmp(ptn_name, "logo"))
    {
        unsigned long long emmc_ptn = 0;

        index = partition_get_index(ptn_name);
        ptn = partition_get_offset(index);

        if(ptn == 0)
        {
            dprintf(INFO, "partition logo doesn't exist\n");
            return -1;
        }

        n = ROUND_TO_PAGE(len, page_mask);
        dprintf(INFO, "Partition %s index[0x%x], ptn_offset[%u], page_num[%u]\n", ptn_name, index, ptn, n);

        if (mmc_read(ptn + offset, (unsigned int *)buf, n))
        {
            dprintf(INFO, "ERROR: Cannot read flylogo  logodata:0x%x\n", n);
            return -1;
        }

        return 0;
    }
    else
        dprintf(INFO, "Please ensure partition %s should be read\n", ptn_name);
#endif
    if(!strcmp(ptn_name, "misc"))
    {
		if (read_misc(offset, buf, len))
		{
			dprintf(CRITICAL, "fmisc:read_misc.error\n");
			return -1;
		}
		dprintf(CRITICAL, "fmisc:read_misc.success\n");
		return 0;
    }
}

