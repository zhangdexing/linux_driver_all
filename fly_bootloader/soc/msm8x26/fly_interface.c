#include "soc.h"
#include "fly_platform.h"
#include "fly_target.h"

#define DBG_UART_PORT g_bootloader_hw.dbg_uart_port

#if DEVICE_TREE
BUF_DMA_ALIGN(dt_buf, 4096);
#endif

static unsigned char buf[4096]; //Equal to max-supported pagesize

int boot_flyrecovery_from_mmc()
{
#if (defined BOOTLOADER_MSM8909)
char *cmdline;
struct boot_img_hdr *hdr = (void*) buf;
	struct boot_img_hdr *uhdr;
	unsigned offset = 0;
	int rcode;
	unsigned long long ptn = 0;
	int index = INVALID_PTN;

	unsigned char *image_addr = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual;
	unsigned second_actual = 0;

#if DEVICE_TREE
	struct dt_table *table;
	struct dt_entry dt_entry;
	unsigned dt_table_offset;
	uint32_t dt_actual;
	uint32_t dt_hdr_size;
#endif
	BUF_DMA_ALIGN(kbuf, BOOT_IMG_MAX_PAGE_SIZE);
	struct kernel64_hdr *kptr = (void*) kbuf;

	if (check_format_bit())
		boot_into_recovery = 1;

	if (!boot_into_recovery) {
		memset(ffbm_mode_string, '\0', sizeof(ffbm_mode_string));
		rcode = get_ffbm(ffbm_mode_string, sizeof(ffbm_mode_string));
		if (rcode <= 0) {
			boot_into_ffbm = false;
			if (rcode < 0)
				dprintf(CRITICAL,"failed to get ffbm cookie");
		} else
			boot_into_ffbm = true;
	} else
		boot_into_ffbm = false;
	uhdr = (struct boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
	if (!memcmp(uhdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(INFO, "Unified boot method!\n");
		hdr = uhdr;
		goto unified_boot;
	}

	index = partition_get_index("flyrecovery");
	dprintf(INFO,"partition flyrecovery index = %d \n",index);
	ptn = partition_get_offset(index);
	dprintf(INFO,"partition flyrecovery ptn = %d \n",ptn);

	if (mmc_read(ptn + offset, (unsigned int *) buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
                return -1;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {

		if (hdr->page_size > BOOT_IMG_MAX_PAGE_SIZE) {
			dprintf(CRITICAL, "ERROR: Invalid page size\n");
			return -1;
		}
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	/* Read the next page to get kernel Image header
	 * which lives in the second page for arm64 targets.
	 */

	if (mmc_read(ptn + page_size, (unsigned int *) kbuf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr, IS_ARM64(kptr));

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses overlap with aboot addresses.\n");
		return -1;
	}

#ifndef DEVICE_TREE
	if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
	{
		dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
		return -1;
	}
#endif

	/* Authenticate Kernel */
	dprintf(INFO, "use_signed_kernel=%d, is_unlocked=%d, is_tampered=%d.\n",
		(int) target_use_signed_kernel(),
		device.is_unlocked,
		device.is_tampered);

#if VERIFIED_BOOT
	boot_verifier_init();
#endif

	if(target_use_signed_kernel() && (!device.is_unlocked))
	{
		offset = 0;

		image_addr = (unsigned char *)target_get_scratch_address();

#if DEVICE_TREE
		dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

		if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_actual))
		{
			dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
			return -1;
		}
#else
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual);

#endif

		dprintf(INFO, "Loading boot image (%d): start\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		if (check_aboot_addr_range_overlap(image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read image without signature */
		if (mmc_read(ptn + offset, (void *)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
				return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		offset = imagesize_actual;

		if (check_aboot_addr_range_overlap(image_addr + offset, page_size))
		{
			dprintf(CRITICAL, "Signature read buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read signature */
		if(mmc_read(ptn + offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		verify_signed_bootimg(image_addr, imagesize_actual);

		/* Move kernel, ramdisk and device tree to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

		#if DEVICE_TREE
		if(hdr->dt_size) {
			dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
			table = (struct dt_table*) dt_table_offset;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Device Tree Blob cannot be found\n");
				return -1;
			}

			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void *)hdr->tags_addr, (char *)dt_table_offset + dt_entry.offset, dt_entry.size);
		} else {
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						hdr->kernel_size,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
				return -1;
			}
		}
		#endif
	}
	else
	{
		second_actual  = ROUND_TO_PAGE(hdr->second_size,  page_mask);

		image_addr = (unsigned char *)target_get_scratch_address();
#if DEVICE_TREE
		dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

		if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_actual))
		{
			dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
			return -1;
		}
#else
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual);

#endif
		if (check_aboot_addr_range_overlap(image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		dprintf(INFO, "Loading boot image (%d): start\n",
				imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		offset = 0;

		/* Load the entire boot image */
		if (mmc_read(ptn + offset, (void *)image_addr, imagesize_actual)) {
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
					return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n",
				imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		#ifdef TZ_SAVE_KERNEL_HASH
		aboot_save_boot_hash_mmc(image_addr, imagesize_actual);
		#endif /* TZ_SAVE_KERNEL_HASH */

		/* Move kernel, ramdisk and device tree to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

		#if DEVICE_TREE
		if(hdr->dt_size) {
			dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
			table = (struct dt_table*) dt_table_offset;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
				return -1;
			}

			/* Validate and Read device device tree in the tags_addr */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void *)hdr->tags_addr, (char *)dt_table_offset + dt_entry.offset, dt_entry.size);
		} else {
			/* Validate the tags_addr */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, kernel_actual))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						kernel_actual,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
				return -1;
			}
		}
		#endif
	}

	if (boot_into_recovery && !device.is_unlocked && !device.is_tampered)
		target_load_ssd_keystore();

#else
	unsigned *boot_into_recovery = 0;
	struct boot_img_hdr *hdr = (void*) buf;
	struct boot_img_hdr *uhdr;
	unsigned offset = 0;
	int rcode;
	unsigned long long ptn = 0;
	int index = INVALID_PTN;
	char *cmdline;

	unsigned char *image_addr = 0;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned imagesize_actual;
	unsigned second_actual = 0;
	const char *open_system_print_redScreen  = "console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x37 update_hw_info=1";

#if DEVICE_TREE
	struct dt_table *table;
	struct dt_entry dt_entry;
	unsigned dt_table_offset;
	uint32_t dt_actual;
	uint32_t dt_hdr_size;
#endif

	dprintf(CRITICAL,"****** Boot into flyrecovery model ******\n");

	if (!boot_into_recovery) {
		memset(ffbm_mode_string, '\0', sizeof(ffbm_mode_string));
		rcode = get_ffbm(ffbm_mode_string, sizeof(ffbm_mode_string));
		if (rcode <= 0) {
			boot_into_ffbm = false;
			if (rcode < 0)
				dprintf(CRITICAL,"failed to get ffbm cookie");
		} else
			boot_into_ffbm = true;
	} else
		boot_into_ffbm = false;
	uhdr = (struct boot_img_hdr *)EMMC_BOOT_IMG_HEADER_ADDR;
	if (!memcmp(uhdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(INFO, "Unified boot method!\n");
		hdr = uhdr;
		goto unified_boot;
	}

	index = partition_get_index("flyrecovery");
	dprintf(INFO,"partition flyrecovery index = %d \n",index);
	ptn = partition_get_offset(index);
	dprintf(INFO,"partition flyrecovery ptn = %d \n",ptn);

	if(ptn == 0) {
		dprintf(CRITICAL, "ERROR: No flyrecovery partition found\n");
                return -1;
	}

	if (mmc_read(ptn + offset, (unsigned int *) buf, page_size)) {
		dprintf(CRITICAL, "ERROR: Cannot read boot image header\n");
                return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		dprintf(CRITICAL, "ERROR: Invalid boot image header\n");
                return -1;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	/*
	 * Update the kernel/ramdisk/tags address if the boot image header
	 * has default values, these default values come from mkbootimg when
	 * the boot image is flashed using fastboot flash:raw
	 */
	update_ker_tags_rdisk_addr(hdr);

	/* Get virtual addresses since the hdr saves physical addresses. */
	hdr->kernel_addr = VA((addr_t)(hdr->kernel_addr));
	hdr->ramdisk_addr = VA((addr_t)(hdr->ramdisk_addr));
	hdr->tags_addr = VA((addr_t)(hdr->tags_addr));

	kernel_actual  = ROUND_TO_PAGE(hdr->kernel_size,  page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

	/* Check if the addresses in the header are valid. */
	if (check_aboot_addr_range_overlap(hdr->kernel_addr, kernel_actual) ||
		check_aboot_addr_range_overlap(hdr->ramdisk_addr, ramdisk_actual))
	{
		dprintf(CRITICAL, "kernel/ramdisk addresses overlap with aboot addresses.\n");
		return -1;
	}

#ifndef DEVICE_TREE
	if (check_aboot_addr_range_overlap(hdr->tags_addr, MAX_TAGS_SIZE))
	{
		dprintf(CRITICAL, "Tags addresses overlap with aboot addresses.\n");
		return -1;
	}
#endif

	/* Authenticate Kernel */
	dprintf(INFO, "use_signed_kernel=%d, is_unlocked=%d, is_tampered=%d.\n",
		(int) target_use_signed_kernel(),
		device.is_unlocked,
		device.is_tampered);

	if(target_use_signed_kernel() && (!device.is_unlocked))
	{
		offset = 0;

		image_addr = (unsigned char *)target_get_scratch_address();

#if DEVICE_TREE
		dt_actual = ROUND_TO_PAGE(hdr->dt_size, page_mask);
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual + dt_actual);

		if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_actual))
		{
			dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
			return -1;
		}
#else
		imagesize_actual = (page_size + kernel_actual + ramdisk_actual);

#endif

		dprintf(INFO, "Loading boot image (%d): start\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		if (check_aboot_addr_range_overlap(image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "Boot image buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read image without signature */
		if (mmc_read(ptn + offset, (void *)image_addr, imagesize_actual))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image\n");
				return -1;
		}

		dprintf(INFO, "Loading boot image (%d): done\n", imagesize_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		offset = imagesize_actual;

		if (check_aboot_addr_range_overlap(image_addr + offset, page_size))
		{
			dprintf(CRITICAL, "Signature read buffer address overlaps with aboot addresses.\n");
			return -1;
		}

		/* Read signature */
		if(mmc_read(ptn + offset, (void *)(image_addr + offset), page_size))
		{
			dprintf(CRITICAL, "ERROR: Cannot read boot image signature\n");
			return -1;
		}

		verify_signed_bootimg(image_addr, imagesize_actual);

		/* Move kernel, ramdisk and device tree to correct address */
		memmove((void*) hdr->kernel_addr, (char *)(image_addr + page_size), hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, (char *)(image_addr + page_size + kernel_actual), hdr->ramdisk_size);

		#if DEVICE_TREE
		if(hdr->dt_size) {
			dt_table_offset = ((uint32_t)image_addr + page_size + kernel_actual + ramdisk_actual + second_actual);
			table = (struct dt_table*) dt_table_offset;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Device Tree Blob cannot be found\n");
				return -1;
			}

			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			memmove((void *)hdr->tags_addr, (char *)dt_table_offset + dt_entry.offset, dt_entry.size);
		} else {
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						hdr->kernel_size,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
				return -1;
			}
		}
		#endif
	}
	else
	{
		second_actual  = ROUND_TO_PAGE(hdr->second_size,  page_mask);

		dprintf(INFO, "Loading boot image (%d): start\n",
				kernel_actual + ramdisk_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_START);

		offset = page_size;

		/* Load kernel */
		if (mmc_read(ptn + offset, (void *)hdr->kernel_addr, kernel_actual)) {
			dprintf(CRITICAL, "ERROR: Cannot read kernel image\n");
					return -1;
		}
		offset += kernel_actual;

		/* Load ramdisk */
		if(ramdisk_actual != 0)
		{
			if (mmc_read(ptn + offset, (void *)hdr->ramdisk_addr, ramdisk_actual)) {
				dprintf(CRITICAL, "ERROR: Cannot read ramdisk image\n");
				return -1;
			}
		}
		offset += ramdisk_actual;

		dprintf(INFO, "Loading boot image (%d): done\n",
				kernel_actual + ramdisk_actual);
		bs_set_timestamp(BS_KERNEL_LOAD_DONE);

		if(hdr->second_size != 0) {
			offset += second_actual;
			/* Second image loading not implemented. */
			ASSERT(0);
		}

		#if DEVICE_TREE
		if(hdr->dt_size != 0) {
			/* Read the first page of device tree table into buffer */
			if(mmc_read(ptn + offset,(unsigned int *) dt_buf, page_size)) {
				dprintf(CRITICAL, "ERROR: Cannot read the Device Tree Table\n");
				return -1;
			}
			table = (struct dt_table*) dt_buf;

			if (dev_tree_validate(table, hdr->page_size, &dt_hdr_size) != 0) {
				dprintf(CRITICAL, "ERROR: Cannot validate Device Tree Table \n");
				return -1;
			}

			table = (struct dt_table*) memalign(CACHE_LINE, dt_hdr_size);
			if (!table)
				return -1;

			/* Read the entire device tree table into buffer */
			if(mmc_read(ptn + offset,(unsigned int *) table, dt_hdr_size)) {
				dprintf(CRITICAL, "ERROR: Cannot read the Device Tree Table\n");
				return -1;
			}

			/* Find index of device tree within device tree table */
			if(dev_tree_get_entry_info(table, &dt_entry) != 0){
				dprintf(CRITICAL, "ERROR: Getting device tree address failed\n");
				return -1;
			}

			/* Validate and Read device device tree in the "tags_add */
			if (check_aboot_addr_range_overlap(hdr->tags_addr, dt_entry.size))
			{
				dprintf(CRITICAL, "Device tree addresses overlap with aboot addresses.\n");
				return -1;
			}

			if(mmc_read(ptn + offset + dt_entry.offset,
						 (void *)hdr->tags_addr, dt_entry.size)) {
				dprintf(CRITICAL, "ERROR: Cannot read device tree\n");
				return -1;
			}
			#ifdef TZ_SAVE_KERNEL_HASH
			aboot_save_boot_hash_mmc(hdr->kernel_addr, kernel_actual,
				       hdr->ramdisk_addr, ramdisk_actual,
				       ptn, offset, hdr->dt_size);
			#endif /* TZ_SAVE_KERNEL_HASH */

		} else {
			/*
			 * If appended dev tree is found, update the atags with
			 * memory address to the DTB appended location on RAM.
			 * Else update with the atags address in the kernel header
			 */
			void *dtb;
			dtb = dev_tree_appended((void*) hdr->kernel_addr,
						kernel_actual,
						(void *)hdr->tags_addr);
			if (!dtb) {
				dprintf(CRITICAL, "ERROR: Appended Device Tree Blob not found\n");
				return -1;
			}
		}
		#endif
	}

	if (boot_into_recovery && !device.is_unlocked && !device.is_tampered)
		target_load_ssd_keystore();
#endif
unified_boot:

	cmdline = dbg_msg_en(hdr->cmdline, bp_meg);

	boot_linux((void *)hdr->kernel_addr, (void *)hdr->tags_addr,
		   (const char *)cmdline, board_machtype(),
		   (void *)hdr->ramdisk_addr, hdr->ramdisk_size);

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
		sprintf(cmdline, "%s", DBG_UART_PORT);
		strcat(cmdline, system_cmd);
	}else{
		dprintf(INFO,"System print is disabled !\n");
		cmdline = system_cmd;
	}

#ifdef BOOTLOADER_ID_14
{
        char *cmdline2;
	cmd_size = strlen("lk_id=14 ") + strlen(cmdline);
	cmdline2 = malloc(cmd_size);
	if(!cmdline2){
		dprintf(INFO, " malloc space for cmdline failed, use system cmdline \n");
		return system_cmd;
	}
	sprintf(cmdline2, "%s", "lk_id=14 ");
	strcat(cmdline2, cmdline);
        return cmdline2;
}
#endif
	return cmdline;
}
