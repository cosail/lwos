#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

static int read_register(char reg_addr);
static u32 get_rtc_time(struct time *t);

/*
  <ring 1> the main loop of task sys.
*/
void task_sys()
{
	MESSAGE msg;
	struct time t;
	//disp_str("task sys running ...\n");
	//assert(0);
	
	while (true)
	{
		//disp_str("task sys waiting for incoming message.\n");
		send_recv(RECEIVE, ANY, &msg);
		//disp_str("task sys received message\n");
		int src = msg.source;
		
		switch (msg.type)
		{
		case GET_TICKS:
			msg.RETVAL = ticks;
			send_recv(SEND, src, &msg);
			break;

		case GET_PID:
			msg.type = SYSCALL_RET;
			msg.PID = src;
			send_recv(SEND, src, &msg);
			break;

		case GET_RTC_TIME:
			msg.type = SYSCALL_RET;
			get_rtc_time(&t);
			phys_copy(va2la(src, msg.BUF),
				va2la(TASK_SYS, &t),
				sizeof(struct time));
			send_recv(SEND, src, &msg);
			break;

		default:
			panic("unknown msg type");
			break;
		}
	}
}

static u32 get_rtc_time(struct time *t)
{
	t->year = read_register(YEAR);
	t->month = read_register(MONTH);
	t->day = read_register(DAY);
	t->hour = read_register(HOUR);
	t->minute = read_register(MINUTE);
	t->second = read_register(SECOND);

	if ((read_register(CLK_STATUS) & 0x04) == 0)
	{
		t->year = BCD_TO_DEC(t->year);
		t->month = BCD_TO_DEC(t->month);
		t->day = BCD_TO_DEC(t->day);
		t->hour = BCD_TO_DEC(t->hour);
		t->minute = BCD_TO_DEC(t->minute);
		t->second = BCD_TO_DEC(t->second);
	}
	
	t->year += 2000;

	return 0;
}

static int read_register(char reg_addr)
{
	out_byte(CLK_ELE, reg_addr);
	return in_byte(CLK_IO);
}
