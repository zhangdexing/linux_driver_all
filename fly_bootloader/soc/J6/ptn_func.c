#include "../soc.h"
#include <fsl_fastboot.h>
#include <android_image.h>
#include <fsl_fastboot.h>
#include <mmc.h>
#include <android_image.h>
#include <asm/bootm.h>
#include <nand.h>
#include <aboot.h>


unsigned page_mask = 0;
unsigned ptn_buf[15000] __aligned(ARCH_DMA_MINALIGN);

int ptn_read(char *ptn_name, unsigned int offset, unsigned long len, unsigned char *buf)
{
	struct fastboot_ptentry *ptn;
	struct mmc *mmc;
	int mmcc = 2;
	int size;
	mmc = find_mmc_device(mmcc);
	if (!mmc) {
			printf("boota: cannot find '%d' mmc device\n", mmcc);
			goto fail;
	}
	ptn = fastboot_flash_find_ptn(ptn_name);
	if (!ptn)
	{
			printf("cannot find '%s' partition\n", ptn_name);
			goto fail;
	}	
    else
    {
		size=len/512+1;
		if (mmc->block_dev.block_read(mmcc, ptn->start,size, (void *)ptn_buf) < 0)
		{
            printf("read %s failed\n", ptn_name);
            goto fail;
        }
		else
		{
			memcpy(buf,ptn_buf,len);
		}
    }
   
    return  0;
fail:
	return -1;

}

