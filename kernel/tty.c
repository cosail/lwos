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
#include "keyboard.h"


#define TTY_FIRST	(tty_table)
#define TTY_LAST	(tty_table + NR_CONSOLES)

static void init_tty(TTY *tty);
static void tty_dev_read(TTY *tty);
static void tty_dev_write(TTY *tty);
static void tty_do_read(TTY *tty, MESSAGE *msg);
static void tty_do_write(TTY *tty, MESSAGE *msg);
static void put_key(TTY *tty, u32 key);

void task_tty()
{
	TTY *tty;
	MESSAGE msg;
	
	init_keyboard();
	
	for (tty = TTY_FIRST; tty < TTY_LAST; tty++)
	{
		init_tty(tty);
	}
	
	select_console(0);
	
	while (true)
	{
		for (tty = TTY_FIRST; tty < TTY_LAST; tty++)
		{
			do 
			{
				tty_dev_read(tty);
				tty_dev_write(tty);

			} while (tty->inbuf_count);
		}

		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		assert(src != TASK_TTY);

		TTY *ptty = &tty_table[msg.DEVICE];

		switch (msg.type)
		{
		case DEV_OPEN:
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;

		case DEV_READ:
			tty_do_read(ptty, &msg);
			break;

		case DEV_WRITE:
			tty_do_write(ptty, &msg);
			break;

		case HARDWARE_INT:
			key_pressed = 0;
			continue;

		default:
			dump_msg("TTY:unknown message.", &msg);
			break;
		}
	}
}

static void init_tty(TTY *tty)
{
	tty->inbuf_count = 0;
	tty->inbuf_head = tty->inbuf_tail = tty->inbuf;
	
	init_screen(tty);
}

void in_process(TTY *tty, u32 key)
{
	if (!(key & FLAG_EXT))
	{
		put_key(tty, key);
	}
	else
	{
		int raw_code = key & MASK_RAW;
		
		switch (raw_code)
		{
		case ENTER:
			put_key(tty, '\n');
			break;
		case BACKSPACE:
			put_key(tty, '\b');
			break;
		case UP:
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
			{
				scroll_screen(tty->console, SCROLL_DOWN);
			}
			break;
		case DOWN:
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
			{
				scroll_screen(tty->console, SCROLL_UP);
			}
			break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R))
			{
				select_console(raw_code - F1);
			}
			break;
		default:
			break;	
		}
	}
}

static void put_key(TTY* tty, u32 key)
{
	if (tty->inbuf_count < TTY_IN_BYTES)
	{
		*tty->inbuf_head = key;
		tty->inbuf_head++;
		if (tty->inbuf_head == tty->inbuf + TTY_IN_BYTES)
			tty->inbuf_head = tty->inbuf;
		tty->inbuf_count++;
	}
}

static void tty_dev_read(TTY *tty)
{
	if (is_current_console(tty->console))
	{
		keyboard_read(tty);
	}
}

static void tty_dev_write(TTY *tty)
{
	if (tty->inbuf_count > 0)
	{
		char ch = *(tty->inbuf_tail);
		tty->inbuf_tail++;
		if (tty->inbuf_tail == tty->inbuf + TTY_IN_BYTES)
			tty->inbuf_tail = tty->inbuf;
		tty->inbuf_count--;

		if (tty->tty_left_cnt)
		{
			if (ch >= ' ' && ch <= '~')
			{
				out_char(tty->console, ch);
				void *p = tty->tty_req_buf + tty->tty_trans_cnt;
				phys_copy(p, (void*)va2la(TASK_TTY, &ch), 1);
				tty->tty_trans_cnt++;
				tty->tty_left_cnt--;
			}
			else if (ch == '\b' && tty->tty_trans_cnt)
			{
				out_char(tty->console, ch);
				tty->tty_trans_cnt--;
				tty->tty_left_cnt++;
			}

			if (ch == '\n' || tty->tty_left_cnt == 0)
			{
				out_char(tty->console, '\n');
				MESSAGE msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = tty->tty_procnr;
				msg.CNT = tty->tty_trans_cnt;

				send_recv(SEND, tty->tty_caller, &msg);
				tty->tty_left_cnt = 0;
			}
		}
	}
}

static void tty_do_read(TTY *tty, MESSAGE *msg)
{
	tty->tty_caller = msg->source;
	tty->tty_procnr = msg->PROC_NR;
	tty->tty_req_buf = va2la(tty->tty_procnr, msg->BUF);
	tty->tty_left_cnt = msg->CNT;
	tty->tty_trans_cnt = 0;

	msg->type = SUSPEND_PROC;
	send_recv(SEND, tty->tty_caller, msg);
}

static void tty_do_write(TTY *tty, MESSAGE *msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char *p = (char*)va2la(msg->PROC_NR, msg->BUF);

	int i = msg->CNT;
	int j;

	while (i)
	{
		int bytes = min(TTY_OUT_BUF_LEN, i);
		phys_copy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for (j = 0; j < bytes; j++)
		{
			out_char(tty->console, buf[j]);
		}

		i -= bytes;
		p += bytes;
	}
	
	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}

int sys_printx(int _unused1, int _unused2, char *string, PROCESS *process)
{
	const char *p;
	char ch;
	
	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAGIC_CHAR_PANIC;
	
	/*
	  @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	  If this happens in Ring 0, no linear-physical address mapping
	  is needed.
	  
	  @attention The value of 'k_reenter' is tricky here. When
	  -# printx() is called in Ring 0
	  		- k_reenter > 0. When code in Ring 0 calls printx(),
	  		  an 'interrupt re-enter' will occur (printx() generates
	  		  a software interrupt). Thus 'k_reenter' will be increased
	  		  by 'kernel.asm::save' and be greater than 0.
	  -# printx() is called in Ring 1~3
	  		- k_reenter == 0
	*/
	if (k_reenter == 0)
		p = va2la(proc2pid(process), string);
	else if (k_reenter > 0)
		p = string;
	else
		p = reenter_err;
		
	if ((*p == MAGIC_CHAR_PANIC) ||
		(*p == MAGIC_CHAR_ASSERT && p_proc_ready < &proc_table[NR_TASKS]))
	{
		disable_int();
		char *v = (char*)VIDEO_MEM_BASE;
		const char *q = p + 1;
		
		while (v < (char*)(VIDEO_MEM_BASE + VIDEO_MEM_SIZE))
		{
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (*q == 0)
			{
				while (((int)v - VIDEO_MEM_BASE) % (SCREEN_WIDTH * 16))
				{
					v++;
					*v++ = GRAY_CHAR;
				}
				
				q = p + 1;
			}
		}
		
		__asm__ __volatile__("hlt");
	}
	
	while ((ch = *p++) != 0)
	{
		if (ch == MAGIC_CHAR_PANIC || ch == MAGIC_CHAR_ASSERT)
			continue;
			
		//out_char(tty_table[process->nr_tty].console, ch);
		out_char(TTY_FIRST->console, ch);
	}
	
	return 0;
}

void dump_tty_buf()
{
	TTY * tty = &tty_table[1];
	static char sep[] = "--------------------------------\n";

	printl(sep);

	printl("head: %d\n", tty->inbuf_head - tty->inbuf);
	printl("tail: %d\n", tty->inbuf_tail - tty->inbuf);
	printl("cnt: %d\n", tty->inbuf_count);

	int pid = tty->tty_caller;
	printl("caller: %s (%d)\n", proc_table[pid].name, pid);
	pid = tty->tty_procnr;
	printl("caller: %s (%d)\n", proc_table[pid].name, pid);

	printl("req_buf: %d\n", (int)tty->tty_req_buf);
	printl("left_cnt: %d\n", tty->tty_left_cnt);
	printl("trans_cnt: %d\n", tty->tty_trans_cnt);

	printl("--------------------------------\n");

	strcpy(sep, "\n");
}
