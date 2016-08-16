#include "soc.h"
#include "fly_platform.h"
#include "fly_target.h"

#define DBG_UART_PORT g_bootloader_hw.dbg_uart_port
unsigned int flyrecovery_load_addr = NULL;

int boot_flyrecovery_from_mmc()
{
	int ret;
	ret = mboot_android_load_recoveryimg_hdr("flyrecovery", NULL);
	if(ret < 0){
	        printk("Load FlyRecovery hdr failed\n");
	        return -1;
	}
	flyrecovery_load_addr = (unsigned int)target_get_scratch_address();
	ret = mboot_android_load_recoveryimg("flyrecovery", flyrecovery_load_addr);
	if(ret < 0){
	        printk("Load FlyRecovery Image failed\n");
	        flyrecovery_load_addr = NULL;
	}
	if(flyrecovery_load_addr == NULL)
	        return -1;
	g_boot_mode = FLYRECOVERY_BOOT;

	return 0;
}

char *dbg_msg_en(const char *system_cmd, int dbg_msg_en)
{
	char *cmdline;
	int cmd_size = 0;

	if(dbg_msg_en == 1){
		dprintf(INFO,"System print is enabled !\n");

		cmd_size = strlen(DBG_UART_PORT) + strlen(system_cmd);

		cmdline = malloc(cmd_size);
		if(!cmdline){
			dprintf(INFO, " malloc space for cmdline failed, use system cmdline \n");
			return system_cmd;
		}

		cmdline = DBG_UART_PORT;
		strcat(cmdline, system_cmd);
	}else{
		dprintf(INFO,"System print is disabled !\n");
		cmdline = system_cmd;
	}

	return cmdline;
}
