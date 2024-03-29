#ifndef _CONSTANT_H_
#define _CONSTANT_H_

// max() and min()
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

// boolean
#define TRUE  1
#define FALSE 0

#define true  1
#define false 0

// color of video output
#define BLACK	0x00
#define BLUE	0x01
#define GREEN	0x02
#define CYAN	0x03
#define RED		0x04
#define CARMINE	0x05
#define BROWN	0x06
#define WHITE	0x07
#define FLASH	0x80
#define BRIGHT	0x08
#define MAKE_COLOR(x,y) ((x << 4) | y) // MAKE_COLOR(background, foreground)

// count of GDT & IDT
#define GDT_SIZE 128
#define IDT_SIZE 256

// privilege
#define PRIVILEGE_KERNEL 0
#define PRIVILEGE_TASK   1
#define PRIVILEGE_USER   3

// RPL
#define RPL_KERNEL	SA_RPL0
#define RPL_TASK	SA_RPL1
#define RPL_USER	SA_RPL3

// 8259A interrupt controller ports
#define INT_M_CTL		0x20
#define INT_M_CTLMASK	0x21
#define INT_S_CTL		0xa0
#define INT_S_CTLMASK	0xa1

// 8253/8254 PIT(Programmable Interval Timer)
#define TIMER0			0x40
#define TIMER_MODE		0x43
#define RATE_GENERATOR	0x34
#define TIMER_FREQ		1193182L
#define HZ				100

// AT or PS/2 keyboard, 8042 ports
#define KB_DATA		0x60
#define KB_CMD		0x64
#define LED_CODE	0xed
#define KB_ACK		0xfa

// VGA(Video Graphics Array)
#define VIDEO_MEM_BASE	0xb8000
#define VIDEO_MEM_SIZE	0x8000
#define CRTC_ADDR_REG	0x3d4
#define CRTC_DATA_REG	0x3d5
#define START_ADDR_H	0xc
#define START_ADDR_L	0xd
#define CURSOR_H		0xe
#define CURSOR_L		0xf

// CMOS
#define CLK_ELE		0x70
#define CLK_IO		0x71

#define YEAR	9
#define MONTH	8
#define DAY		7
#define HOUR	4
#define MINUTE	2
#define SECOND	0

#define CLK_STATUS	0x0b
#define CLK_HEALTH	0x0e

// tty
#define NR_CONSOLES 3

// hardware interrupts
#define NR_IRQ			16
#define CLOCK_IRQ		0
#define KEYBOARD_IRQ	1
#define CASCADE_IRQ		2
#define ETHER_IRQ		3
#define RSS232_IRQ		4
#define XT_WINI_IRQ		5
#define FLOPPY_IRQ		6
#define PRINTER_IRQ		7
#define AT_WINI_IRQ		14

// process
#define SENDING		0x02
#define RECEIVING	0x04
#define WAITING		0x08
#define HANGING		0x10
#define FREE_SLOT	0x20

// tasks
#define INVALID_DRIVER	-20
#define INTERRUPT		-10
#define TASK_TTY	0
#define TASK_SYS	1
#define TASK_HD		2
#define TASK_FS		3
#define TASK_MM		4
#define INIT		5
#define ANY				(NR_TASKS + NR_PROCS + 10)
#define NO_TASK			(NR_TASKS + NR_PROCS + 20)

#define MAX_TICKS		0x7fffabcd

// system call
#define NR_SYS_CALL	3

// IPC
#define SEND	1
#define RECEIVE	2
#define BOTH	3

// magic chars used by 'printx'
#define MAGIC_CHAR_PANIC	'\002'
#define MAGIC_CHAR_ASSERT	'\003'

enum msgtype
{
	HARDWARE_INT = 1,

	// SYS task
	GET_TICKS, GET_PID, GET_RTC_TIME,

	// FS
	OPEN, CLOSE, READ, WRITE, LSEEK, STAT, UNLINK,

	// FS & TTY
	SUSPEND_PROC, RESUME_PROC,

	// MM
	EXEC, WAIT,

	// FS & MM
	FORK, EXIT,

	// TTY, SYS, FS, MM, etc
	SYSCALL_RET,

	// message type for drivers
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL,

	// for debug
	DISK_LOG
};

// macros for messages
#define FD			u.m3.m3i1
#define PATHNAME	u.m3.m3p1
#define FLAGS		u.m3.m3i1
#define NAME_LEN	u.m3.m3i2
#define CNT			u.m3.m3i2
#define REQUEST		u.m3.m3i2
#define PROC_NR		u.m3.m3i3
#define DEVICE		u.m3.m3i4
#define POSITION	u.m3.m3l1
#define BUF			u.m3.m3p2
#define OFFSET		u.m3.m3i2
#define WHENCE		u.m3.m3i3
#define PID			u.m3.m3i2
#define STATUS		u.m3.m3i1
#define RETVAL		u.m3.m3i1

#define DIOCTL_GET_GEO	1

// hard drive
#define SECTOR_SIZE			512
#define SECTOR_BITS			(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9

// major device number (corresponding to kernel/global.c::dd_map[])
#define NO_DEV			0
#define DEV_FLOPPY		1
#define DEV_CDROM		2
#define DEV_HD			3
#define DEV_CHAR_TTY	4
#define DEV_SCSI		5

// make device number from major and minor numbers
#define MAJOR_SHIFT		8
#define MAKE_DEV(a,b)	((a << MAJOR_SHIFT) | b)

// sepatate major and minor numbers from device number
#define MAJOR(x)		((x >> MAJOR_SHIFT) & 0xff)
#define MINOR(x)		(x & 0xff)

#define INVALID_INODE	0
#define ROOT_INODE		1

#define MAX_DRIVES			2
#define NR_PART_PER_DRIVE	4
#define NR_SUB_PER_PART		16
#define NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1)

#define MAX_PRIM			(MAX_DRIVES * NR_PRIM_PER_DRIVE)
#define MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES)

// device numbers of hard disk
#define MINOR_hd1a		0x10
#define MINOR_hd2a		(MINOR_hd1a + NR_SUB_PER_PART)

#define ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)

#define P_PRIMARY	0
#define P_EXTENDED	1

#define ORANGES_PART	0x99
#define NO_PART			0x00
#define EXT_PART		0x05

#define NR_FILES		64
#define NR_FILE_DESC	64
#define NR_INODE		64
#define NR_SUPER_BLOCK	8

// INODE::i_mode(octal, lower 12 bits reserved)
#define I_TYPE_MASK		0170000
#define I_REGULAR		0100000
#define I_BLOCK_SPECIAL	0060000
#define I_DIRECTORY		0040000
#define I_CHAR_SPECIAL	0020000
#define I_NAMED_PIPE	0010000

#define is_special(m) ((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) || \
	(((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

#define NR_DEFAULT_FILE_SECTS	2048

#endif //_CONSTANT_H_

