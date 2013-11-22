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


int kernel_main()
{
	disp_str("------kernel main begin------\n");
	
	int i, j, eflags, prio;
	u8 rpl;
	u8 priv;
	TASK *t;
	PROCESS *p = proc_table;
	char *stk = task_stack + STACK_SIZE_TOTAL;
	
	for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++, t++)
	{
		if (i >= NR_TASKS + NR_NATIVE_PROCS)
		{
			p->flags = FREE_SLOT;
			continue;
		}
		
		if (i < NR_TASKS)
		{
			t = task_table + i;
			priv = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202; // IF=1, IOPL=1, bit 2 always 1
		}
		else
		{
			t = user_process_table + i - NR_TASKS;
			priv = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x0202; // IF=1, bit 2 always 1
		}
		
		strcpy(p->name, t->name);
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0)
		{
			p->ldts[INDEX_LDT_CODE] = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_DATA] = gdt[SELECTOR_KERNEL_DS >> 3];

			p->ldts[INDEX_LDT_CODE].attr1 = DA_C | priv << 5;
			p->ldts[INDEX_LDT_DATA].attr1 = DA_DRW | priv << 5;
		}
		else
		{
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);

			init_desc(&p->ldts[INDEX_LDT_CODE],
				0,
				(k_base + k_limit) >> LIMIT_4K_SHITF,
				DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_DATA],
				0,
				(k_base + k_limit) >> LIMIT_4K_SHITF,
				DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}
		
		p->regs.cs = INDEX_LDT_CODE << 3 | SA_TIL | rpl;
		p->regs.ds = 
		p->regs.es = 
		p->regs.fs = 
		p->regs.ss = INDEX_LDT_DATA << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip = (u32)t->initial_eip;
		p->regs.esp = (u32)stk;
		p->regs.eflags = eflags;

		p->flags = 0;
		p->msg = 0;
		p->recvfrom = NO_TASK;
		p->sendto = NO_TASK;
		p->has_int_msg = 0;
		p->sending_queue = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
		{
			p->filp[j] = 0;
		}
		
		stk -= t->stacksize;
	}
	
	ticks = 0;
	k_reenter = 0;
	p_proc_ready = proc_table;
	
	disp_str("init_clock()\n");
	init_clock();
	init_keyboard();
/*	
	disp_pos = 0;
	for (i = 0; i < 80 * 25; i++)
		disp_str(" ");
	disp_pos = 0;
*/
	disp_str("call restart()\n");
	__asm__ __volatile__ ("xchg %bx, %bx");
	restart();
	
	while (1) {}
	
	return 0;
}

int get_ticks()
{
/*	return ticks;*/
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	
	return msg.RETVAL;	
}

void Init()
{
	int fd_stdin = open("/dev_tty0", O_RDWR);
	assert(fd_stdin == 0);

	int fd_stdout = open("dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	int pid = fork();
	if (pid != 0)
	{
		printf("parent is running, child pid=%d.\n", pid);
		spin("parent");
	}
	else
	{
		printf("child is running, pid=%d.\n", getpid());
		spin("child");
	}
}

void TestA()
{
	for (;;)
	{
	}
}

void TestB()
{
	for (;;)
	{
	}
}

void TestC()
{
	for (;;)
	{
	}
}

void panic(const char *format, ...)
{
	int i;
	char buf[256];
	
	va_list args = (va_list)((char*)&format + 4);
	i = vsprintf(buf, format, args);
	
	printl("%c !!panic!! %s", MAGIC_CHAR_PANIC, buf);
	
	// should never arrive here
	__asm__ __volatile__("ud2");
}

