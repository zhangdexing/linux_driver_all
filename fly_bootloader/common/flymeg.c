#define __LIB_FS_H

#include <config.h>
#include "flymeg.h"

#include "flyaboot.h"
#include "soc.h"

static const int EXTRA_PAGE = 0;

recovery_meg_t RecoveryMeg =
{
    .bootParam.autoUp.val = 0,
    .verParam.lpc.ver = {0, 0, 0, 0},
};

#if WITH_DEBUG_GLOBAL_RAM
//extern char print_buf[];
extern unsigned int print_idx;
#define MISC_SKIP_BYTE 10240
#endif

#if WITH_DEBUG_GLOBAL_RAM
#define RESERVE_LOG_MAGIC 0x474F4C52  //"RLOG" in ascii
#define PRINT_BUFF_SIZE	(128 * 1024)
char print_buf[PRINT_BUFF_SIZE];
//unsigned int print_idx = 12;
#endif
static char fly_buf[4096];


#if 0
int FlyGetRecoveryMessage(struct recovery_message *out, char *parname)
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned pagesize = flash_page_size();

    ptable = flash_get_ptable();

    if (ptable == NULL)
    {
        dprintf(CRITICAL, "ERROR: Partition table not found\n");
        return -1;
    }
    ptn = ptable_find(ptable, parname);

    if (ptn == NULL)
    {
        dprintf(CRITICAL, "ERROR: No misc partition found\n");
        return -1;
    }

    offset += (pagesize * FLY_MISC_COMMAND_PAGE);
    if (flash_read(ptn, offset, (void *) fly_buf, pagesize))
    {
        dprintf(CRITICAL, "ERROR: Cannot read recovery_header\n");
        return -1;
    }
    memcpy(out, fly_buf, sizeof(*out));
    return 0;
}

int SaveDebugMessage(void)
{

#if WITH_DEBUG_GLOBAL_RAM
    unsigned n = 0;
    struct save_log_message save_log_cookie;

    if (target_is_emmc_boot())
    {
        char *ptn_name = "misc";
        unsigned long long ptn_offset = 0;
        unsigned int index;

        index = partition_get_index((unsigned char *) ptn_name);

        if(INVALID_PTN == index)
        {
            dprintf(CRITICAL, "ERROR: Partition index not found\n");
            return -1;
        }

        ptn_offset = partition_get_offset(index);

        dprintf(INFO, "INFO: save the lk log to misc\n");
        save_log_cookie.flags[0] = 0x6e616670;
        save_log_cookie.flags[1] = 0x676f6c67;
        save_log_cookie.length = print_idx;

        memset((void *)SCRATCH_ADDR, 0, 512);
        memcpy((void *)SCRATCH_ADDR, (void *)&save_log_cookie, sizeof(struct save_log_message));

        /*Save cookie*/
        if (mmc_write(ptn_offset + MISC_SKIP_BYTE, 512, (void *)SCRATCH_ADDR))
        {
            dprintf(CRITICAL, "ERROR: flash write fail!\n");
            return -1;
        }
        memcpy((void *)SCRATCH_ADDR , (void *)print_buf, save_log_cookie.length);
        arch_disable_cache(UCACHE);
        n = ( save_log_cookie.length / 512 + 1) * 512;

        if (mmc_write(ptn_offset + MISC_SKIP_BYTE + 512, n, (void *)SCRATCH_ADDR))
        {
            dprintf(CRITICAL, "ERROR: flash write fail!\n");
            return -1;
        }
    }
    else
    {
        /*for the innopath change the FOTA ecc type, now we disable the nand lk log temporary*/
#if 0
        struct ptentry *ptn;
        struct ptable *ptable;
        unsigned offset = 0;
        unsigned pagesize = flash_page_size();
        unsigned pagemask = pagesize - 1;

        ptable = flash_get_ptable();

        if (ptable == NULL)
        {
            dprintf(CRITICAL, "ERROR: Partition table not found\n");
            return -1;
        }
        ptn = ptable_find(ptable, "FOTA");

        if (ptn == NULL)
        {
            dprintf(CRITICAL, "ERROR: No misc partition found\n");
            return -1;
        }
        dprintf(INFO, "INFO: save the lk log to misc\n");
        save_log_cookie.flags[0] = 0x6e616670;
        save_log_cookie.flags[1] = 0x676f6c67;
        save_log_cookie.length = print_idx;

        memcpy((void *)SCRATCH_ADDR, (void *)&save_log_cookie, sizeof(struct save_log_message));
        memcpy((void *)SCRATCH_ADDR + sizeof(struct save_log_message), (void *)print_buf, save_log_cookie.length);
        arch_disable_cache(UCACHE);
        n = ((sizeof(struct save_log_message) + save_log_cookie.length ) / pagesize + 1) * pagesize;
        if (flash_write(ptn, 0, (void *)SCRATCH_ADDR, n))
        {
            dprintf(CRITICAL, "ERROR: flash write fail!\n");
            return -1;
        }
#endif
    }
#endif
    return 0;
}

int SetRecoveryMessage(const struct recovery_message *in, char *parname)
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned pagesize = flash_page_size();
    unsigned n = 0;

    ptable = flash_get_ptable();

    if (ptable == NULL)
    {
        dprintf(CRITICAL, "ERROR: Partition table not found\n");
        return -1;
    }
    ptn = ptable_find(ptable, parname);

    if (ptn == NULL)
    {
        dprintf(CRITICAL, "ERROR: No misc partition found\n");
        return -1;
    }

    n = pagesize * (FLY_MISC_COMMAND_PAGE + 1);

    if (flash_read(ptn, offset, (void *) SCRATCH_ADDR, n))
    {
        dprintf(CRITICAL, "ERROR: Cannot read recovery_header\n");
        return -1;
    }

    offset += (pagesize * FLY_MISC_COMMAND_PAGE);
    offset += SCRATCH_ADDR;
    memcpy((void *) offset, in, sizeof(*in));
    if (flash_write(ptn, 0, (void *)SCRATCH_ADDR, n))
    {
        dprintf(CRITICAL, "ERROR: flash write fail!\n");
        return -1;
    }
    return 0;
}

int  FlyCheckRecoveryMessage(char *parname)
{
    struct recovery_message msg;
    char partition_name[32];
    unsigned valid_command = 0;
    int update_status = 0;
    dprintf(INFO, "start  get recovery meg for %s \n", parname);

    if (FlyGetRecoveryMessage(&msg, parname))
    {
        dprintf(INFO, "get recovery meg failed for %s \n", parname);
        return -1;
    }
    else
    {
        dprintf(INFO, "get recovery meg success for %s \n", parname);
    }

    msg.command[sizeof(msg.command) - 1] = '\0'; //Ensure termination

    dprintf(INFO, "get recovery meg is:%s\n", msg.command);

    if (!strcmp("reboot recovery", msg.command))
    {
        dprintf(INFO, "get recovery meg success\n");
        strlcpy(msg.status, "OKAY", sizeof(msg.status));

        strlcpy(msg.command, "", sizeof(msg.command));	// clearing recovery command
        set_recovery_message(&msg);	// send recovery message

        boot_into_recovery = 1;		// Boot in recovery mode
        return 0;
    }
    else
    {
        dprintf(INFO, "get recovery meg failed\n");
        return -1;
    }

}

#endif

int get_extra_recovery_message(recovery_meg_t *out)
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned pagesize = flash_page_size();

    //dprintf(INFO, "this is func:get_extra_recovery_message \n");
    ptable = flash_get_ptable();

    if (ptable == NULL)
    {
        dprintf(INFO, "ERROR: Partition table not found\n");
        return -1;
    }
    ptn = ptable_find(ptable, "flyparameter");

    if (ptn == NULL)
    {
        dprintf(INFO, "ERROR: No flyparameter partition found\n");
        return -1;
    }
    offset += (pagesize * EXTRA_PAGE);
    if (flash_read(ptn, offset, fly_buf, pagesize))
    {
        dprintf(INFO, "ERROR: Cannot read flyparameter partition\n");
        return -1;
    }

    memcpy(out, fly_buf, sizeof(*out));
    dprintf(INFO, "Sucess to read recovery_header \n");
    dprintf(INFO, "out:%d\n", out->bootParam.autoUp.val);

    return 0;
}


void fly_erase( char *parname)
{
    struct ptentry *ptn;
    struct ptable *ptable;

    ptable = flash_get_ptable();
    if (ptable == NULL)
    {
        dprintf(INFO, "partition table doesn't exist");
        return;
    }

    ptn = ptable_find(ptable, parname);
    if (ptn == NULL)
    {
        dprintf(INFO, "unknown %s name", parname);
        return;
    }

    if (flash_erase(ptn))
    {
        dprintf(INFO, "failed to erase partition");
        return;
    }
    dprintf(INFO, "erase %s is access!!", parname);
}



int get_extra_persist_message(char *pname, char *out, int num)
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned pagesize = flash_page_size();

    //	dprintf(INFO	, "this is func:get_extra_recovery_message \n");
    ptable = flash_get_ptable();

    if (ptable == NULL)
    {
        dprintf(INFO, "ERROR: Partition table not found\n");
        return -1;
    }
    ptn = ptable_find(ptable, pname);

    if (ptn == NULL)
    {
        dprintf(INFO, "ERROR: No %s partition found\n", pname);
        return -1;
    }
    //offset += (pagesize * EXTRA_PAGE);
    offset += (pagesize * num);

    if (flash_read(ptn, offset, fly_buf, pagesize))
    {
        dprintf(INFO, "ERROR: Cannot read %s partition\n", pname);
        return -1;
    }

    memcpy(out, (char *)fly_buf, pagesize);
    dprintf(INFO, "Sucess to read %s \n", pname);
    //dprintf(INFO, "out:%d\n",out->bootParam.autoUp.val);

    return 0;
}


int set_extra_recovery_message(const recovery_meg_t *in)
{
    struct ptentry *ptn;
    struct ptable *ptable;
    unsigned offset = 0;
    unsigned pagesize = flash_page_size();
    unsigned n = 0;
    ptable = flash_get_ptable();

    if (ptable == NULL)
    {
        dprintf(CRITICAL, "ERROR: Partition table not found\n");
        return -1;
    }
    ptn = ptable_find(ptable, "flyparameter");

    if (ptn == NULL)
    {
        dprintf(CRITICAL, "ERROR: No flyparameter partition found\n");
        return -1;
    }

    n = pagesize * (EXTRA_PAGE + 1);
#if 0
    if (flash_read(ptn, offset, SCRATCH_ADDR, n))
    {
        dprintf(CRITICAL, "ERROR: Cannot read recovery_header\n");
        return -1;
    }

    offset += (pagesize * EXTRA_PAGE);
    offset += SCRATCH_ADDR;
    memcpy(offset, in, sizeof(*in));
    if (flash_write(ptn, 0, (void *)SCRATCH_ADDR, n))
    {
        dprintf(CRITICAL, "ERROR: flash write fail!\n");
        return -1;
    }
#endif
}
/*******************************************/
/**EMMC***/

int emmc_get_extra_recovery_msg(char *ptn_name, unsigned  long len, unsigned char *in)
{
    //char *ptn_name = "misc";
    unsigned long long ptn = 0;
    unsigned int size =  2048;//ROUND_TO_PAGE(sizeof(*in),511);
    int index = INVALID_PTN;
    dprintf(INFO, "partition %sv(1) \n", ptn_name);
    index = partition_get_index((unsigned char *) ptn_name);
    dprintf(INFO, "partition %s index = %08x\n", ptn_name, index);
    ptn = partition_get_offset(index);
    dprintf(INFO, "partition %s ptn = %d\r\n", ptn_name, ptn);
    if(ptn == 0)
    {
        dprintf(CRITICAL, "partition %s doesn't exist\n", ptn_name);
        return -1;
    }
    dprintf(INFO, "partition %sv(2) \n", ptn_name);

    int n = len / 2048;
    n = n + 1;
    int i = 0;
    dprintf(INFO, "partition %s pages num = %d\n", ptn_name, n);
    unsigned char *data = malloc(n * 2048);
    dprintf(INFO, "partition %sv(3) \n", ptn_name);
    while(n--)
    {
        if (mmc_read((ptn + i * 2048), (unsigned int *)(data + i * 2048), 2048))
        {
            dprintf(CRITICAL, "mmc read failure %s %d\n", ptn_name, len);
            free(data);
            return -1;
        }
        i++;
    }
    dprintf(INFO, "partition %sv(4) \n", ptn_name);
    memcpy(in, data, len);
    free(data);
    return 0;
}
int emmc_get_extra_recovery_msg2(char *ptn_name, unsigned  long len, unsigned char *in)

{
    //char *ptn_name = "misc";

    unsigned long long ptn = 0;
    unsigned int size =  2048;//ROUND_TO_PAGE(sizeof(*in),511);
    int index = INVALID_PTN;
    dprintf(INFO, "partition %sv(1) \n", ptn_name);
    index = partition_get_index((unsigned char *) ptn_name);
    dprintf(INFO, "partition %s index = %08x\n", ptn_name, index);
    ptn = partition_get_offset(index);
    dprintf(INFO, "partition %s ptn = %d\r\n", ptn_name, ptn);
    if(ptn == 0)
    {
        dprintf(CRITICAL, "partition %s doesn't exist\n", ptn_name);
        return -1;
    }
    dprintf(INFO, "partition %sv(2) \n", ptn_name);

    int n = len / 2048;
    n = n + 1;
    int i = 0;
    dprintf(INFO, "partition %s pages num = %d\n", ptn_name, n);
    unsigned char *data = malloc(n * 2048);
    dprintf(INFO, "partition %sv(3) \n", ptn_name);
    while(n--)
    {
        if (mmc_read((ptn + i * 2048 + 512 * 1024), (unsigned int *)(data + i * 2048), 2048))
        {
            dprintf(CRITICAL, "mmc read failure %s %d\n", ptn_name, len);
            free(data);
            return -1;
        }
        i++;
    }
    dprintf(INFO, "partition %sv(4) \n", ptn_name);
    memcpy(in, data, len);
    free(data);
    return 0;
}

int emmc_set_extra_recovery_msg(char *ptn_name, unsigned  long len, unsigned char *out)
{
    //char *ptn_name = "misc";
    unsigned long long ptn = 0;
    //unsigned int size =  2048;//ROUND_TO_PAGE(sizeof(*out),511);
    unsigned char data[len];
    int index = INVALID_PTN;

    index = partition_get_index((unsigned char *) ptn_name);
    ptn = partition_get_offset(index);
    if(ptn == 0)
    {
        dprintf(CRITICAL, "partition %s doesn't exist\n", ptn_name);
        return -1;
    }
    memcpy(data, out, len);
    if (mmc_write(ptn , len, (unsigned int *)data))
    {
        dprintf(CRITICAL, "mmc write failure %s %d\n", ptn_name, len);
        return -1;
    }
    return 0;

}


#define RecoveryModel 0xf0
int  boot_compare_lpc_version(void)
{
    //Get LPC of the Version Number
    int i, j;
    unsigned char *Ver = CheckLpcVersions();

    if(Ver == NULL)
    {
        dprintf(INFO, "failed:read the LPC version failed!!!");
        return -1;
    }

    for(i = 0; i < 4; i++)
        dprintf(INFO, "RecoveryMeg.verParam.lpc.ver[%d]:0x%x\n",
                i, RecoveryMeg.verParam.lpc.ver[i]);
    //compare LPC of the Version Number
    Ver += 6;
    for(i = 6, j = 0; j < 4; i++, j++)
    {
        dprintf(INFO, "ver[%d]:0x%x\n", j, Ver[j]);
    }

    if(memcmp(RecoveryMeg.verParam.lpc.ver, Ver, 4))
    {
        if(RecoveryMeg.bootParam.autoUp.val != 55)
        {
            RecoveryMeg.bootParam.autoUp.val = 55;
            set_extra_recovery_message(&RecoveryMeg);
            mdelay(30);
            get_extra_recovery_message(&RecoveryMeg);
            dprintf(INFO, "@@@@@@@@RecoveryMeg.bootParam.autoUp.val= %d\n", RecoveryMeg.bootParam.autoUp.val);
        }
        dprintf(INFO, "ERROR:====>>the LPC version is error!!\n");
        return RecoveryModel;
    }
    //}

    return 0;

}

//extern int fs_mount_type(const char *path, const char *device, const char *name);
//extern int fs_create_file(const char *path, filecookie *fcookie);
//extern int fs_make_dir(const char *path);
//extern int fs_write_file(filecookie fcookie, const void *buf, off_t offset, size_t len);
//extern int fs_mount1212(const char *path, const char *device);
#if 0
int check_persist_partition()
{
    int count = 0;
    int num = 0 ;
    int checknum = 1;
    int i = 0, j = 0;
    int  size = flash_page_size();
    unsigned char *temp = malloc(size + 10);
    // char temp[size];
    memset(temp, 0, (size));
    while(num < 100)
    {

        //	for(i = 0;i<10;i++)
        //{
        /*
        	if(get_extra_persist_message("persist",temp,(num+i)))
        		break;;

        	for(j = 0;j<size;j++)
        	{
        		dprintf(INFO,"temp[%d]:%x  ",j,temp[j]);
        		if(!(j%14))dprintf(INFO,"\n");
        	}

        	*/
        fs_init();
        checknum = fs_mount("/persist", NULL);
        if(!checknum) dprintf(INFO, "*FLYAUDIO: persist have date!!\n");
        else dprintf(INFO, "*FLYAUDIO: persist is not date!!\n");
        num++;
        mdelay(50);
        /*
        if(temp != NULL )
        {
        	for(count = 0;count<size;count++)
        	{
        		if(!memcmp((temp + count),"persist.prop",strlen("persist.prop")))//persist.prop
        		{
        			dprintf(INFO,"success to read persist info num = %d!!\n",num);
        			free(temp);
        			return 0;
        		}
        	}
        }

        }
        num += 64;
        */

    }
    free(temp);
    return 0xffff;
}
#endif
