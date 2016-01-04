PREFIX = /Users/haru/SDKs/gcc-arm-none-eabi-4_9-2015q1
ARCH = arm-none-eabi
BINDIR = $(PREFIX)/bin
ADDNAME = $(ARCH)-

AR = $(BINDIR)/$(ADDNAME)ar
AS = $(BINDIR)/$(ADDNAME)as
CC = $(BINDIR)/$(ADDNAME)gcc
LD = $(BINDIR)/$(ADDNAME)ld
NM = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB = $(BINDIR)/$(ADDNAME)ranlib
STRIP = $(BINDIR)/$(ADDNAME)strip

OBJS = vector.o startup.o main.o
OBJS += lib.o serial.o xmodem.o elf.o

TARGET = kzload

CFLAGS = -Wall -std=c99 -mcpu=cortex-m4 -mthumb -nostdinc -nostdlib -fno-builtin -fleading-underscore
CFLAGS += -I.
CFLAGS += -Os
CFLAGS += -DKZLOAD

LFLAGS = -static -T ld.scr -L.

.SUFFIXES: .elf .hex
.SUFFIXES: .c .o
.SUFFIXES: .S .o

all :		$(TARGET).hex

.elf.hex :	$<
			$(OBJCOPY) -O binary $< $@
#			$(OBJCOPY) -O ihex $< $@

$(TARGET).elf :	$(OBJS)
	        $(CC) $(OBJS) -o $@ $(CFLAGS) $(LFLAGS)

.c.o :	    $<
	        $(CC) -c $(CFLAGS) $<

.S.o :	    $<
	        $(CC) -c $(CFLAGS) $<

clean :
	        rm -f $(OBJS) $(TARGET).elf $(TARGET).hex
