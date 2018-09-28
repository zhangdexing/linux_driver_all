# ----------------------------------------------
#				mipsel version
# ----------------------------------------------
#CC = /opt/buildroot-gcc342/bin/mipsel-linux-uclibc-g++

#--- Denug Version ---
#DEFINES=-D__LINUX__ -D_DEBUG

#--- Release Version ---
#DEFINES=-D__LINUX__ -D_REINIT_

#--- Release Version (ReInit Ver) ---
#DEFINES=-D__LINUX__ -D_REINIT_

#LIB = ./libusb.a ./BurnerApLib.a

# ----------------------------------------------
#				x86 version
# ----------------------------------------------
CC = g++

#--- Denug Version ---
#DEFINES=-D__LINUX__ -D_DEBUG

#--- Release Version ---
DEFINES=-D__LINUX__

#--- Release Version (ReInit Ver) ---
#DEFINES=-D__LINUX__ -D_REINIT_

LIB = /usr/lib/libusb.a \
     ../BurnerApLib/BurnerApLib.a

# ----------------------------------------------
#				ARM-based version
# ----------------------------------------------
#ARM = arm-none-linux-gnueabi-
#CC = $(ARM)g++

#--- Denug Version ---
#DEFINES=-D__LINUX__ -D_DEBUG

#--- Release Version ---
#DEFINES=-D__LINUX__

#--- Release Version (ReInit Ver) ---
#DEFINES=-D__LINUX__ -D_REINIT_

#LIB = libusb.a \
#      BurnerApLib.a

# ----------------------------------------------
CFLAGS=-O2 -g -Wall -static $(DEFINES)

SOURCE_COMMON = ../common/debug.cpp \
				../common/CamEnum.cpp \
		        ../common/misc.cpp 

SOURCE_BURNMGR = ../BurnMgr/FW_File.cpp \
				 ../BurnMgr/BurnMgr.cpp

SOURCE = $(SOURCE_COMMON) \
		 $(SOURCE_BURNMGR) \
		 main.cpp

TARGET = SONiX_BurnerAP/fw_update

all:
	$(CC) $(CFLAGS) $(SOURCE) $(LIB) -o $(TARGET)

clean:
	rm -f *.o $(TARGET) *~

romfs:
	$(ROMFSINST) /bin/SONiX_BurnerAP
