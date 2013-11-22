#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "fs.h"
#include "global.h"

PROCESS proc_table[NR_TASKS + NR_NATIVE_PROCS];

char task_stack[STACK_SIZE_TOTAL];

TASK task_table[NR_TASKS] = {
				{task_tty, STACK_SIZE_TTY, "tty", 20},
				{task_sys, STACK_SIZE_SYS, "sys", 20},
				{task_hd, STACK_SIZE_HD, "hd", 20},
				{task_fs, STACK_SIZE_FS, "fs", 20},
				{task_mm, STACK_SIZE_MM, "mm", 20}
			};
			
TASK user_process_table[NR_NATIVE_PROCS] = {
				{Init, STACK_SIZE_INIT, "INIT", 25},
				{TestA, STACK_SIZE_TESTA, "TestA", 5},
				{TestB, STACK_SIZE_TESTB, "TestB", 10},
				{TestC, STACK_SIZE_TESTC, "TestC", 15}
			};

irq_handler irq_table[NR_IRQ];

system_call sys_call_table[NR_SYS_CALL] = {
				sys_printx,
				sys_sendrec
			};

TTY tty_table[NR_CONSOLES];

CONSOLE console_table[NR_CONSOLES];

struct dev_drv_map dd_map[] = {
	{INVALID_DRIVER},
	{INVALID_DRIVER},
	{INVALID_DRIVER},
	{TASK_HD},
	{TASK_TTY},
	{INVALID_DRIVER}
};

// 6MB~7MB: buffer for FS
u8 *fsbuf = (u8*)0x600000;
const int FSBUF_SIZE = 0x100000;

// 7MB~8MB: buffer for MM
u8 *mmbuf = (u8*)0x700000;
const int MMBUF_SIZE = 0x100000;

// 8MB~10MB: buffer for log (debug)
char *logbuf = (char*)0x800000;
const int LOGBUF_SIZE = 0x100000;

char *logdiskbuf = (char*)0x900000;
const int LOGDISKBUF_SIZE = 0x100000;