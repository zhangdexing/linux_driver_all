
unsigned char* ubootlogobak = NULL;

#define bootloader_fb_addr  0x16100000
#define SIZE 1024*600

int thread_show_logo(void *lcd_virt_addr)
{

	int i = 0, j = 0, k = 0;
	unsigned char* p565 = (unsigned char*)ubootlogobak;
	unsigned char* p888 = (unsigned char*)lcd_virt_addr;
	unsigned short pixelRGB565;

    for (i = 0; i < SIZE; i++)
    {
        pixelRGB565 = p565[k + 1];
        pixelRGB565 = (pixelRGB565 << 8) + p565[k];

		p888[j + 3] = 0;
        p888[j + 2] = (pixelRGB565 << 3) & 0x00F8;
        p888[j + 1] = (pixelRGB565 >> 3) & 0x00FC;
        p888[j + 0] = (pixelRGB565 >> 8) & 0x00F8;

        j = j + 4;
        k = k + 2;
    }

	printk("show kernel logo done !\n");

	/*unsigned char* androidlogo = (unsigned char *)kmalloc(SIZE * 4, GFP_KERNEL);
	memcpy((unsigned char*)androidlogo, (unsigned char*)lcd_virt_addr, SIZE * 4);

	*(unsigned char *)lcd_virt_addr = 0xff;
	while((*(unsigned char *)(lcd_virt_addr)) != 0)
	{
		msleep(1);
	}
	memcpy((unsigned char *)lcd_virt_addr, (unsigned char*)androidlogo, SIZE * 4);

	printk("show kernel logo done2 !\n");*/
	return 0;
}

int show_uboot_image(char *lcd_virt_addr)
{

	struct task_struct *task;
	task = kthread_create(thread_show_logo, lcd_virt_addr, "thread_show_logo");
	if(IS_ERR(task))
	{
		printk("Unable to start thread.\n");
	}
	else 
		wake_up_process(task);

	return 0;
}

void uboot_logo_bakup(void)
{
	ubootlogobak = (unsigned char *)kmalloc(SIZE * 2, GFP_KERNEL);
	memcpy((unsigned char*)ubootlogobak, (unsigned char*)phys_to_virt(bootloader_fb_addr), SIZE*2);
	return;
}


EXPORT_SYMBOL_GPL(show_uboot_image);

