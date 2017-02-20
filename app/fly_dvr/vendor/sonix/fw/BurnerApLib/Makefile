# ----------------------------------------------
#				mipsel version
# ----------------------------------------------
#CC = mipsel-linux-uclibc-gcc

#--- Denug Version ---
#DEFINES=-D__LINUX__ -D_DEBUG

#--- Release Version ---
#DEFINES=-D__LINUX__

#--- Release Version (ReInit Ver) ---
#DEFINES=-D__LINUX__ -D_REINIT_

#LIB = ./libusb.a 

#AR = mipsel-linux-ar
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

LIB = /usr/local/lib/libusb.a 

AR = $(ARM)ar
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

#LIB = libusb.a 

#AR = $(ARM)ar
# ----------------------------------------------

CFLAGS=-O2 -g -Wall -c $(DEFINES)

ARFLAGS = rcs

SOURCE = BurnerApLib.cpp \
        ..//common//debug.cpp \
        ..//common//misc.cpp

OBJ = BurnerApLib.o \
     debug.o \
     misc.o
      
TARGET = BurnerApLib.a

all : 
	$(CC) $(CFLAGS) $(SOURCE)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJ)

clean:
	rm -f *.o $(TARGET)
