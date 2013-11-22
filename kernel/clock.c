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


void clock_handler(int irq)
{
	if (++ticks >= MAX_TICKS)
		ticks = 0;

	if (p_proc_ready->ticks)
		p_proc_ready->ticks--;
		
	if (key_pressed)
	{
		inform_init(TASK_TTY);
	}
	
	if (k_reenter != 0)
		return;
	
	if (p_proc_ready->ticks > 0)
	{
		//disp_str(p_proc_ready->name);
		return;
	}

	schedule();	
}

void milli_delay(int milli_sec)
{
	int t = get_ticks();
	while (((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

void init_clock()
{
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ/HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ/HZ) >> 8));
	
	put_irq_handler(CLOCK_IRQ, clock_handler);
	enable_irq(CLOCK_IRQ);
}
