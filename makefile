# Makefile for oss

# entry point of kernel
# it must be the same value with 'PhyKernelEntryPoint' in load.inc
ENTRYPOINT = 0x30400

# offset of entry point in kernel file
# it depends on ENTRYPOINT
ENTRYOFFSET = 0x400

# programs, flags, etc.
ASM = nasm
DASM = ndisasm
CC = gcc
LD = ld
ASM_BOOT_FLAGS = -I boot/include/
ASM_KERNEL_FLAGS = -I include/ -I include/sys/ -f elf
CFLAGS = -I include/ -I include/sys/ -c -m32 -fno-builtin -fno-stack-protector
LDFLAGS = -s -m elf_i386 -Ttext $(ENTRYPOINT)
DASMFLAGS = -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# this program
BOOTLOADER = boot/boot.bin boot/loader.bin
KERNEL = kernel.bin
OBJS = kernel/kernel.o kernel/start.o kernel/main.o kernel/global.o \
		kernel/protect.o kernel/i8259.o kernel/clock.o kernel/keyboard.o \
		kernel/proc.o kernel/hd.o kernel/tty.o kernel/console.o \
		lib/syscall.o lib/printf.o lib/vsprintf.o kernel/systask.o \
		lib/misc.o lib/klib.o lib/kliba.o lib/string.o lib/open.o lib/read.o \
		lib/write.o lib/close.o lib/getpid.o lib/syslog.o lib/unlink.o \
		lib/fork.o lib/exit.o lib/wait.o \
		fs/main.o fs/open.o fs/misc.o fs/read_write.o fs/disklog.o fs/link.o \
		mm/main.o mm/forkexit.o
DASMOUTPUT = kernel.bin.asm

# all phony targets
.PHONY : everything final image clean realclean disasm all buildimg

# default starting position
everything : $(BOOTLOADER) $(KERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean : 
	rm -f $(OBJS)

realclean :
	rm -f $(OBJS) $(BOOTLOADER) $(KERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(KERNEL) > $(DASMOUTPUT)

buildimg :
	dd if=boot/boot.bin of=bochs/a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop bochs/a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc \
						boot/include/fat12.inc boot/include/lib16.inc
	$(ASM) $(ASM_BOOT_FLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc \
						boot/include/fat12.inc boot/include/lib16.inc \
						boot/include/lib32.inc boot/include/structs.inc
	$(ASM) $(ASM_BOOT_FLAGS) -o $@ $<

$(KERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm include/sys/sconst.inc
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<

kernel/start.o : kernel/start.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o : kernel/i8259.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/main.o : kernel/main.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/global.o : kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<
		
kernel/protect.o : kernel/protect.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/clock.o : kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/keyboard.o : kernel/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/tty.o : kernel/tty.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/console.o : kernel/console.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/proc.o : kernel/proc.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/systask.o : kernel/systask.c
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/hd.o : kernel/hd.c
	$(CC) $(CFLAGS) -o $@ $<
	
fs/main.o : fs/main.c
	$(CC) $(CFLAGS) -o $@ $<

fs/open.o : fs/open.c
	$(CC) $(CFLAGS) -o $@ $<

fs/read_write.o : fs/read_write.c
	$(CC) $(CFLAGS) -o $@ $<

fs/link.o : fs/link.c
	$(CC) $(CFLAGS) -o $@ $<

fs/misc.o : fs/misc.c
	$(CC) $(CFLAGS) -o $@ $<
	
fs/disklog.o : fs/disklog.c
	$(CC) $(CFLAGS) -o $@ $<
	
lib/misc.o : lib/misc.c
	$(CC) $(CFLAGS) -o $@ $<
	
lib/klib.o : lib/klib.c
	$(CC) $(CFLAGS) -o $@ $<

lib/read.o : lib/read.c
	$(CC) $(CFLAGS) -o $@ $<

lib/write.o : lib/write.c
	$(CC) $(CFLAGS) -o $@ $<

lib/unlink.o : lib/unlink.c
	$(CC) $(CFLAGS) -o $@ $<

lib/getpid.o : lib/getpid.c
	$(CC) $(CFLAGS) -o $@ $<

lib/syslog.o : lib/syslog.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fork.o : lib/fork.c
	$(CC) $(CFLAGS) -o $@ $<

lib/exit.o : lib/exit.c
	$(CC) $(CFLAGS) -o $@ $<

lib/wait.o : lib/wait.c
	$(CC) $(CFLAGS) -o $@ $<

mm/main.o : mm/main.c
	$(CC) $(CFLAGS) -o $@ $<

mm/forkexit.o : mm/forkexit.c
	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<
	
lib/printf.o : lib/printf.c
	$(CC) $(CFLAGS) -o $@ $<
	
lib/vsprintf.o : lib/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/syscall.o : lib/syscall.asm include/sys/sconst.inc
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<
