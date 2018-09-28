
#ifndef __FLYLK_H__
#define __FLYLK_H__

#include "flyaboot.h"
#include "flymeg.h"
#include "i2c_gpio.h"
#include "i2c_screen.h"
#include "soc.h"
#include "fly_target.h"

#define DEFAULT_CMDLINE "mem=100M console=null";
#define FLY_SCREEN_SIZE_1024 1
#define      flyblk_a  20
#define      flyblk_b  100


#define CLEAN_SCREEN_WRITE 0
#define DEVICE_MAGIC_SIZE 13

#define FBCON_WIDTH 1024
#define FBCON_HEIGHT 600
#if (LOGO_FORMAT== RGB888)
#define FBCON_BPP 24
#elif(LOGO_FORMAT == ARGB8888)
#define FBCON_BPP 32
#else
#define FBCON_BPP 16
#endif

/*ctp reset*/
extern unsigned char ctp_read(unsigned char chip_addr, unsigned char *sub_addr, char *buf, unsigned int size);
extern unsigned char ctp_write(unsigned char chip_addr, char *buf, unsigned int size);
extern int ctp_type_get(void);
extern int ctp_points_get(void);

extern int show_logo();
void backlight_enable();
void backlight_disable();
void lpc_reboot();
extern void fly_setBcol(unsigned long int backcolor);
extern void fly_text_lk(int x, int y, const char *s, unsigned long fontcol);
extern int judge_key_state();
extern int  get_boot_mode(void);
extern void fbcon_flush(void);
#if (defined BOOTLOADER_MSM8909)
#define IS_ARM64(ptr) (ptr->magic_64 == KERNEL64_HDR_MAGIC) ? true : false
extern void update_ker_tags_rdisk_addr(struct boot_img_hdr *hdr, bool is_arm64);
extern bool check_format_bit();
#else
extern void update_ker_tags_rdisk_addr(struct boot_img_hdr *hdr);
#endif
extern int check_aboot_addr_range_overlap(uint32_t start, uint32_t size);
extern void verify_signed_bootimg(uint32_t bootimg_addr, uint32_t bootimg_size);
extern void boot_linux(void *kernel, unsigned *tags,
                       const char *cmdline, unsigned machtype,
                       void *ramdisk, unsigned ramdisk_size);

void fly_fbcon_clear(void);
void fly_setBcol(unsigned long int backcolor);
void fly_text_lk(int x, int y, const char *s, unsigned long fontcol);
void fly_version(int x, int y, const char *s, unsigned long fontcol, ...);
void drawRect(int x0, int y0, int wide, int hide, unsigned long linecolor, unsigned long backcolor);
extern void *fb_base_get(void);
#ifndef BOOTLOADER_MT3561
extern u8 I2C_Byte_Read(u8 device_ID, u16 address);
extern u8 I2C_Byte_Write(u8 device_ID, u8 address, u8 bytedata);

#endif
extern int fly_screen_w;
extern int fly_screen_h;
extern recovery_meg_t RecoveryMeg;
extern const char *model_message;
extern const char *NO_SYS_MEG1;
extern const char *NO_SYS_MEG2;
extern const char *FASTBOOT_MEG;
extern const char *INTO_FASTBOT;
extern const char *INTO_REC;
extern const char *INTO_FLYREC;

extern const char *open_system_print_message;
extern const char *android_system_unprintf;
#endif
