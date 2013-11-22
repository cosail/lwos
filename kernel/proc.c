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

static void block(PROCESS *p);
static void unblock(PROCESS *p);
static int msg_send(PROCESS *current, int dest, MESSAGE *msg);
static int msg_receive(PROCESS *current, int src, MESSAGE *msg);
static int deadlock(int src, int dest);

/*
  <Ring 0> choose one process to run.
*/
void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;
	
	while (!greatest_ticks)
	{
		for (p = &FIRST_PROC; p <= &LAST_PROC; p++)
		{
			if (p->flags == 0)
			{
				if (p->ticks > greatest_ticks)
				{
					greatest_ticks = p->ticks;
					p_proc_ready = p;
					//printf("choose process[%s] to run\n", p->name);
				}				
			}
		}
		
		if (!greatest_ticks)
		{
			for (p = &FIRST_PROC; p <= &LAST_PROC; p++)
			{
				p->ticks = p->priority;
			}
		}
	}
}

/*
  <Ring 0> the core routine of system call 'sendrec()'
  
  @param function SEND or RECEIVE
  @param src_dest To/From whom the message is transferred
  @param m Pointer to the MESSAGE body
  @param p The caller process_t
  
  @return Zero if success.
*/
int sys_sendrec(int function, int src_dest, MESSAGE *msg, PROCESS *p)
{
	assert(k_reenter == 0);
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_NATIVE_PROCS) ||
		src_dest == ANY || src_dest == INTERRUPT);
	
	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE *mla = (MESSAGE*)va2la(caller, msg);
	mla->source = caller;
	
	assert(mla->source != src_dest);
	
	/*
	  Actually we have the third message type: BOTH. However, it is not
	  allowed to be passed to the kernel directly. Kernel doesn't know
	  it at all. It is transformed into SEND followed by a RECEIVE by
	  'send_recv()'
	*/
	if (function == SEND)
	{
		//printf("%s call msg_send, src=%d\n", p->name, src_dest);
		ret = msg_send(p, src_dest, msg);
		//printf("msg_send returned %d\n", ret);
		if (ret != 0)
			return ret;
	}
	else if (function == RECEIVE)
	{
		//printf("call msg_receive, src=%d\n", p->name, src_dest);
		ret = msg_receive(p, src_dest, msg);
		//printf("msg_receive returned %d\n", ret);
		if (ret != 0)
			return ret;
	}
	else
	{
		panic("{sys_sendrec} invalid function: %d (SEND: %d, RECEIVE: %d).",
			function, SEND, RECEIVE);
	}
	
	return 0;
}

/*
  <Ring 1~3> IPC syscall
  
  It is an encapsulation of 'sendrec', invoking 'sendrec' should be avoided
  
  @param function SEND, RECEIVE or BOTH
  @param src_dest The caller's process number
  @param msg Pointer to the MESSAGE structure
  
  @return always 0.
*/
int send_recv(int function, int src_dest, MESSAGE *msg)
{
	int ret = 0;
	
	if (function == RECEIVE)
		memset(msg, 0, sizeof(MESSAGE));
		
	switch (function)
	{
	case BOTH:
		//printf("send message\n");
		ret = sendrec(SEND, src_dest, msg);
		if (ret == 0)
			ret = sendrec(RECEIVE, src_dest, msg);
		break;
	case SEND:
	case RECEIVE:
		ret = sendrec(function, src_dest, msg);
		break;
	default:
		assert(function == BOTH || function == SEND || function == RECEIVE);
		break;
	}
	
	return ret;
}

/*
  <Ring 0~1> Calculate the linear address of a certain segment of a given process.
  
  @param p whose (the process pointer)
  @param index which (one process has more than one segments)
  
  @return the required linear address
*/
int ldt_seg_linear(PROCESS *p, int index)
{
	DESCRIPTOR *desc = &p->ldts[index];
	return (desc->base_high << 24) | (desc->base_mid << 16) | desc->base_low;
}

/*
  <Ring 0~1> Virtual address --> Linear address
  
  @param pid PID of the process whose address is to be calculated.
  @param va Virtual address
  
  @return The linear address for the given virtual address.
*/
void* va2la(int pid, void *va)
{
	PROCESS *p = &proc_table[pid];
	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_DATA);
	u32 la = seg_base + (u32)va;
	
	if (pid < NR_TASKS + NR_NATIVE_PROCS)
		assert(la == (u32)va);
		
	return (void*)la;
}

/*
  <Ring 0~3> Clear up a MESSAGE by setting each byte to 0.
  
  @param msg The message to be cleared.
*/
void reset_msg(MESSAGE *msg)
{
	memset(msg, 0, sizeof(MESSAGE));
}

/*
  <Ring 0> This routine is called after 'flags' has benn set (!= 0), it
  calls 'schedule()' to choose another process as the 'p_proc_ready'.
  
  @attention This routine does not chage 'flags'. Make sure the 'flags'
  of the process to be blocked has been set properly.
  
  @param p The process to be blocked.
*/
static void block(PROCESS *p)
{
	assert(p->flags);
	//printf("block process: %s\n", p->name);
	schedule();
}

/*
  <Ring 0> This is dummy routine. It does noting actually. When it is
  called, the 'flags' should have been cleared (== 0).
  
  @param p The unblocked process
*/
static void unblock(PROCESS *p)
{
	assert(p->flags == 0);
}

/*
  <Ring 0> Check whether it is safe to send a message from src to dest.
  The routine will detect if the messaging graph contains a cycle. For
  instance, if we have procs trying to send message like this:
  A -> B -> C -> A, then a deadlock occurs, because all of them will
  wait forever. If no cycles detected, it is considered as safe.
  
  @param src Who wants to send message.
  @param dest To whom the message is sent.
  
  @return Zero if success.
*/
static int deadlock(int src, int dest)
{
	PROCESS *p = proc_table + dest;
	while (true)
	{
		if (p->flags & SENDING)
		{
			if (p->sendto == src)
			{
				p = proc_table + dest;
				printl("=_= %s", p->name);
				do {
					assert(p->msg);
					p = proc_table + p->sendto;
					printl(" -> %s", p->name);
				} while (p != proc_table + src);
				
				printl("=_=");
				
				return 1;
			}
			p = proc_table + p->sendto;
		}
		else
		{
			break;
		}
	}
	
	return 0;
}

/*
  <Ring 0> Send a message to the dest process. If dest is blocked waiting
  for the message, copy the message to it and unblock dest. Otherwise the
  caller will be blocked and appended to the dest's sending queue.
  
  @param curret The caller, the sender.
  @param dest To whom the message is sent.
  @param msg The message.
  
  @return Zero if success.
*/
static int msg_send(PROCESS *current, int dest, MESSAGE *msg)
{
	PROCESS *sender = current;
	PROCESS *pdest = proc_table + dest;
	
	assert(proc2pid(sender) != dest);
	
	if (deadlock(proc2pid(sender), dest))
	{
		panic(">>DEADLOCK<< %s->%s", sender->name, pdest->name);
	}
	
	if ((pdest->flags & RECEIVING) && 
		(pdest->recvfrom == proc2pid(sender) ||
		pdest->recvfrom == ANY))
	{
		assert(pdest->msg);
		assert(msg);
		
		phys_copy(va2la(dest, pdest->msg),
			va2la(proc2pid(sender), msg),
			sizeof(MESSAGE));
		pdest->msg = 0;
		pdest->flags &= ~RECEIVING;
		pdest->recvfrom = NO_TASK;
		unblock(pdest);
		
		assert(pdest->flags == 0);
		assert(pdest->msg == 0);
		assert(pdest->recvfrom == NO_TASK);
		assert(pdest->sendto == NO_TASK);
		
		assert(sender->flags == 0);
		assert(sender->msg == 0);
		assert(sender->recvfrom == NO_TASK);
		assert(sender->sendto == NO_TASK);
	}
	else
	{
		sender->flags |= SENDING;
		assert(sender->flags == SENDING);
		sender->sendto = dest;
		sender->msg = msg;
		
		PROCESS *p;
		if (pdest->sending_queue)
		{
			p = pdest->sending_queue;
			while (p->next_sending)
				p = p->next_sending;
			p->next_sending = sender;
		}
		else
		{
			pdest->sending_queue = sender;
		}
		
		sender->next_sending = 0;
		
		block(sender);
		
		assert(sender->flags == SENDING);
		assert(sender->msg != 0);
		assert(sender->recvfrom == NO_TASK);
		assert(sender->sendto == dest);
	}
	
	return 0;
}

/*
  <Ring 0> Try to get a message from the src process. If src is blocked
  sending the message, copy the message form it and unblock src. Otherwise
  the caller will be blocked.
  
  @param current The caller, the process who wanna receive.
  @param src From whom the message will be received.
  @param msg The meaage pointer to accept the message.
  
  @return Zero if success.
*/
static int msg_receive(PROCESS *current, int src, MESSAGE *msg)
{
	PROCESS *p_who_wanna_recv = current;
	PROCESS *from = 0;
	PROCESS *prev = 0;
	int copyok = 0;
	
	assert(proc2pid(p_who_wanna_recv) != src);
	//printf("process receiving message ...\n");
	
	if ((p_who_wanna_recv->has_int_msg) && ((src == ANY) || (src == INTERRUPT)))
	{
		MESSAGE m;
		reset_msg(&m);
		m.source = INTERRUPT;
		m.type = HARDWARE_INT;
		assert(msg);
		phys_copy(va2la(proc2pid(p_who_wanna_recv), msg), &m , sizeof(MESSAGE));
		
		p_who_wanna_recv->has_int_msg = 0;

		assert(p_who_wanna_recv->flags == 0);
		assert(p_who_wanna_recv->msg == 0);
		assert(p_who_wanna_recv->sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);
		
		return 0;
	}
	
	if (src == ANY)
	{
		//printf("any\n");
		if (p_who_wanna_recv->sending_queue)
		{
			from = p_who_wanna_recv->sending_queue;
			copyok = 1;
			
			assert(p_who_wanna_recv->flags == 0);
			assert(p_who_wanna_recv->msg == 0);
			assert(p_who_wanna_recv->recvfrom == NO_TASK);
			assert(p_who_wanna_recv->sendto == NO_TASK);
			assert(p_who_wanna_recv->sending_queue != 0);
			
			assert(from->flags == SENDING);
			assert(from->msg != 0);
			assert(from->recvfrom == NO_TASK);
			assert(from->sendto == proc2pid(p_who_wanna_recv));
		}
	}
	else
	{
		from = &proc_table[src];
		
		if ((from->flags & SENDING) &&
			(from->sendto == proc2pid(p_who_wanna_recv)))
		{
			copyok = 1;
			
			PROCESS *p = p_who_wanna_recv->sending_queue;
			assert(p);
			
			while (p)
			{
				assert(from->flags & SENDING);
				if (proc2pid(p) == src)
				{
					from = p;
					break;
				}
				
				prev = p;
				p = p->next_sending;
			}
			
			assert(p_who_wanna_recv->flags == 0);
			assert(p_who_wanna_recv->msg == 0);
			assert(p_who_wanna_recv->recvfrom == NO_TASK);
			assert(p_who_wanna_recv->sendto == NO_TASK);
			assert(p_who_wanna_recv->sending_queue != 0);
			
			assert(from->flags == SENDING);
			assert(from->msg != 0);
			assert(from->recvfrom == NO_TASK);
			assert(from->sendto == proc2pid(p_who_wanna_recv));
		}
	}
	
	if (copyok)
	{
		if (from == p_who_wanna_recv->sending_queue)
		{
			assert(prev == 0);
			p_who_wanna_recv->sending_queue = from->next_sending;
			from->next_sending = 0;
		}
		else
		{
			assert(prev);
			prev->next_sending = from->next_sending;
			from->next_sending = 0;
		}
		
		assert(msg);
		assert(from->msg);
		
		phys_copy(va2la(proc2pid(p_who_wanna_recv), msg),
			va2la(proc2pid(from), from->msg),
			sizeof(MESSAGE));
		
		from->msg = 0;
		from->sendto = NO_TASK;
		from->flags &= ~SENDING;
		unblock(from);
	}
	else
	{
		p_who_wanna_recv->flags |= RECEIVING;
		p_who_wanna_recv->msg = msg;
		
		if (src == ANY)
		{
			p_who_wanna_recv->recvfrom = ANY;
		}
		else
		{
			p_who_wanna_recv->recvfrom = proc2pid(from);
		}
		
		block(p_who_wanna_recv);
		
		assert(p_who_wanna_recv->flags == RECEIVING);
		assert(p_who_wanna_recv->msg != 0);
		assert(p_who_wanna_recv->recvfrom != NO_TASK);
		assert(p_who_wanna_recv->sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);
	}
	
	return 0;
}

/*
 *	<ring 0> inform a process that an interrupt has occured.
 */
void inform_init(int task_nr)
{
	PROCESS *p = proc_table + task_nr;

	if ((p->flags & RECEIVING) &&
		((p->recvfrom == INTERRUPT) || (p->recvfrom == ANY)))
	{
		p->msg->source = INTERRUPT;
		p->msg->type = HARDWARE_INT;
		p->msg = 0;
		p->has_int_msg = 0;
		p->flags &= ~RECEIVING;
		p->recvfrom = NO_TASK;
		assert(p->flags == 0);
		unblock(p);

		assert(p->flags == 0);
		assert(p->msg == 0);
		assert(p->recvfrom == NO_TASK);
		assert(p->sendto == NO_TASK);
	}
	else
	{
		p->has_int_msg = 1;
	}
}

void dump_proc(PROCESS *p)
{
	char info[STR_DEFAULT_LEN];
	int i;
	int text_color = MAKE_COLOR(GREEN, RED);
	int dump_len = sizeof(PROCESS);
	
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, 0);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, 0);
	
	sprintf(info, "byte dump of proc_table[%d]:\n", p - proc_table);
	disp_color_str(info, text_color);
	
	for (i = 0; i < dump_len; i++)
	{
		sprintf(info, "%x.", ((u8*)p)[i]);
		disp_color_str(info, text_color);
	}
	
	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY);
	disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK);
	disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel);
	disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks);
	disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority);
	disp_color_str(info, text_color);
	sprintf(info, "pid: 0x%x.  ", p->pid);
	disp_color_str(info, text_color);
	sprintf(info, "name: %s.  ", p->name);
	disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "p_flags: 0x%x.  ", p->flags);
	disp_color_str(info, text_color);
	sprintf(info, "p_recvfrom: 0x%x.  ", p->recvfrom);
	disp_color_str(info, text_color);
	sprintf(info, "p_sendto: 0x%x.  ", p->sendto);
	disp_color_str(info, text_color);
	//sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty);
	disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg);
	disp_color_str(info, text_color);
}

void dump_msg(const char *title, MESSAGE *msg)
{
	int packed = 0;
	printl("{%s}\n<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)msg,
	       packed ? "" : "\n        ",
	       proc_table[msg->source].name,
	       msg->source,
	       packed ? " " : "\n        ",
	       msg->type,
	       packed ? " " : "\n        ",
	       msg->u.m3.m3i1,
	       msg->u.m3.m3i2,
	       msg->u.m3.m3i3,
	       msg->u.m3.m3i4,
	       (int)msg->u.m3.m3p1,
	       (int)msg->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}
