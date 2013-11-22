#ifndef _PROCESS_H_
#define _PROCESS_H_

#define PROCESS_NAME_LEN 32

typedef struct stackframe_t
{
	u32 gs;
	u32 fs;
	u32 es;
	u32 ds;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 kernel_esp;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
	u32 retaddr;
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
	
} STACK_FRAME;

typedef struct process_t
{
	STACK_FRAME regs;
	u16 ldt_sel;
	DESCRIPTOR ldts[LDT_SIZE];
	u32 pid;
	char name[PROCESS_NAME_LEN];
	int ticks;
	int priority;
	//int nr_tty;
	int flags;
	MESSAGE *msg;
	int recvfrom;
	int sendto;
	int has_int_msg;
	struct process_t *sending_queue;
	struct process_t *next_sending;
	struct file_desc *filp[NR_FILES];
	int p_parent;
	int exit_status;
	
} PROCESS;

typedef struct task_t
{ task_f initial_eip;
	int stacksize;
	char name[PROCESS_NAME_LEN];
	int priority;
	
} TASK;

#define proc2pid(x) (x - proc_table)

#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]
// number of tasks and process
#define NR_TASKS		5
#define NR_PROCS		32
#define NR_NATIVE_PROCS	4

#define PROCS_BASE				0xa00000
#define PROC_IMAGE_SIZE_DEFAULT	0x100000
#define PROC_ORIGIN_STACK		0x400

// stack size of tasks
#define STACK_SIZE_DEFAULT	0x4000

#define STACK_SIZE_TTY		STACK_SIZE_DEFAULT
#define STACK_SIZE_SYS		STACK_SIZE_DEFAULT
#define STACK_SIZE_HD		STACK_SIZE_DEFAULT
#define STACK_SIZE_FS		STACK_SIZE_DEFAULT
#define STACK_SIZE_MM		STACK_SIZE_DEFAULT
#define STACK_SIZE_INIT		STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTA	STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTB	STACK_SIZE_DEFAULT
#define STACK_SIZE_TESTC	STACK_SIZE_DEFAULT

#define STACK_SIZE_TOTAL ( \
	STACK_SIZE_TTY + \
	STACK_SIZE_SYS + \
	STACK_SIZE_HD + \
	STACK_SIZE_FS + \
	STACK_SIZE_MM + \
	STACK_SIZE_INIT + \
	STACK_SIZE_TESTA + \
	STACK_SIZE_TESTB + \
	STACK_SIZE_TESTC \
	)

void schedule();
void* va2la(int pid, void *va);
int ldt_seg_linear(PROCESS *p, int index);
void reset_msg(MESSAGE *msg);
void dump_msg(const char *title, MESSAGE *msg);
void dump_proc(PROCESS *p);
int send_recv(int function, int src_dest, MESSAGE *msg);
int sys_sendrec(int function, int src_dest, MESSAGE *msg, PROCESS *p);
int sys_printx(int _unused1, int _unused2, char *s, PROCESS *p);

#endif //_PROCESS_H_
