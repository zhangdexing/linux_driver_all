#include "../soc.h"

unsigned page_mask = 0;

int ptn_read(char *ptn_name, unsigned int offset, unsigned long len, unsigned char *buf)
{
    fbt_partition_t *ptn;

    ptn = fastboot_find_ptn(ptn_name);
    printf("Read partition %s, ptn = 0x%x \n", ptn_name, ptn->offset);

    if(ptn)
    {
        if(CopyFlash2Memory(buf, ptn->offset + offset, DIV_ROUND_UP(len, RK_BLK_SIZE)) < 0)
        {
            printf("read %s failed\n", ptn_name);
            return -1;
        }
    }
    else
    {
        printf("Find %s ptn failed\n", ptn_name);
        return -1;
    }

    return  0;
}

