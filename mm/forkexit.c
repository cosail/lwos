#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include "proc.h"
#include "string.h"
#include "fs.h"
#include "global.h"

static void cleanup(PROCESS *proc);

int do_fork()
{
	PROCESS *p = proc_table;
	int i;
	
	// find a free slot in proc_table
	for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++)
	{
		if (p->flags == FREE_SLOT)
		{
			break;
		}
	}

	int child_pid = i;
	assert(p == &proc_table[child_pid]);
	assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);

	if (i == NR_TASKS + NR_PROCS)
	{
		return -1;
	}

	assert(i < NR_TASKS + NR_PROCS);

	// duplicate the process table
	int pid = mm_msg.source;
	u16 child_ldt_sel = p->ldt_sel;

	// name
	*p = proc_table[pid];
	p->ldt_sel = child_ldt_sel;
	p->p_parent = pid;
	sprintf(p->name, "%s_%d", proc_table[pid].name, child_pid);

	// code segment
	struct descriptor_t *ppd;
	ppd = &proc_table[pid].ldts[INDEX_LDT_CODE];

	int caller_T_base = reassembly(ppd->base_high, 24,
		ppd->base_mid, 16,
		ppd->base_low);

	int caller_T_limit = reassembly(0, 0,
		(ppd->limit_high_attr2 & 0xf), 16,
		ppd->limit_low);

	int caller_T_size = ((caller_T_limit + 1) *
		((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8) ? 4096 : 1)));

	// data and stack segment
	ppd = &proc_table[pid].ldts[INDEX_LDT_DATA];

	int caller_D_S_base = reassembly(ppd->base_high, 24,
		ppd->base_mid, 16,
		ppd->base_low);
	
	int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xf), 16,
		0, 0,
		ppd->limit_low);

	int caller_D_S_size = ((caller_T_limit + 1) *
		((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8) ? 4096 : 1)));

	assert((caller_T_base == caller_D_S_base) &&
		(caller_T_limit == caller_D_S_limit) &&
		(caller_T_size == caller_D_S_size));

	// copy
	int child_base = alloc_mem(child_pid, caller_T_size);
	printl("MM::0x%x <- 0x%x (0x%x bytes)\n",
		child_base, caller_T_base, caller_T_size);

	phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);
	
	init_desc(&p->ldts[INDEX_LDT_CODE],
		child_base,
		(PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHITF,
		DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);

	init_desc(&p->ldts[INDEX_LDT_DATA],
		child_base,
		(PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHITF,
		DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	// tell FS, see fs_fork()
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;

	send_recv(BOTH, TASK_FS, &msg2fs);

	mm_msg.PID = child_pid;

	MESSAGE m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
	m.PID = 0;
	
	send_recv(SEND, child_pid, &m);

	return 0;
}

void do_exit(int status)
{
	int i;
	int pid = mm_msg.source;
	int parent_pid = proc_table[pid].p_parent;
	PROCESS *p = &proc_table[pid];

	MESSAGE msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = pid;

	send_recv(BOTH, TASK_FS, &msg2fs);

	free_mem(pid);

	p->exit_status = status;

	if (proc_table[parent_pid].flags & WAITING)
	{
		printl("MM::((--do_exit()::%s (%d) is WAITING, %s (%d) will be clean up.--))\n",
			proc_table[parent_pid].name, parent_pid,
			p->name, pid);

		proc_table[parent_pid].flags &= ~WAITING;
		cleanup(&proc_table[pid]);
	}
	else
	{
		printl("MM::((--do_exit()::%s (%d) is not WAITING, %s (%d) will be clean up.--))\n",
			proc_table[parent_pid].name, parent_pid,
			p->name, pid);

		proc_table[pid].flags |= HANGING;
	}

	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		if (proc_table[i].p_parent == pid)
		{
			proc_table[i].p_parent = INIT;

			if ((proc_table[INIT].flags & WAITING) &&
				(proc_table[i].flags & HANGING))
			{
				proc_table[INIT].flags &= ~WAITING;
				cleanup(&proc_table[i]);
				assert(0);
			}
			else
			{
			}
		}
	}
}

static void cleanup(PROCESS *proc)
{
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(proc);
	msg2parent.STATUS = proc->exit_status;

	send_recv(SEND, proc->p_parent, &msg2parent);

	proc->flags = FREE_SLOT;

	printl("MM::((--cleanup():: %s (%d) has been cleaned up.--))\n",
		proc->name, proc2pid(proc));
}

void do_wait()
{
	printl("MM::((--do_wait()--))\n");

	int pid = mm_msg.source;
	int i;
	int children = 0;
	PROCESS *p_proc = proc_table;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++, p_proc++)
	{
		if (p_proc->p_parent == pid)
		{
			children++;

			if (p_proc->flags == HANGING)
			{
				cleanup(p_proc);
				return;
			}
		}
	}

	if (children)
	{
		proc_table[pid].flags |= WAITING;
	}
	else
	{
		MESSAGE msg;
		msg.type = SYSCALL_RET;
		msg.PID = NO_TASK;

		send_recv(SEND, pid, &msg);
	}
}
