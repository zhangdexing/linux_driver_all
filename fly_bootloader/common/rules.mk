INCLUDES += -I$(DBG_BOOTLOADER_DIR)/flyaudio/common/include -I$(DBG_BOOTLOADER_DIR)/app/aboot -I$(DBG_BOOTLOADER_DIR)/dev/ssbi/include
INCLUDES += -I$(DBG_BOOTLOADER_DIR)/flyaudio/soc

include $(DBG_BOOTLOADER_DIR)/flyaudio/common/build_cfg.mk

OBJS +=\
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/dsi83.o \
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/ctp_func.o \
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/flyaboot.o \
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/flykey.o	\
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/flymeg.o	\
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/showlogo.o \
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/lpc_i2c.o \
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/flyfbcon.o\
	$(DBG_BOOTLOADER_DIR)/flyaudio/common/i2c_gpio.o
