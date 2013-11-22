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


void init_8259A()
{
	// master 8259, ICW1
	out_byte(INT_M_CTL, 0x11);
	
	// slave 8259, ICW1
	out_byte(INT_S_CTL, 0x11);
	
	// master 8259, ICW2. set the interrupt entry address to 0x20
	out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0);
	
	// slave 8259, ICW2. set the interrupt entry address to 0x29
	out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8);
	
	// master 8259, ICW3. use slave 8259
	out_byte(INT_M_CTLMASK, 0x4);
	
	// slave 8259, ICW3. related to master 8259 IR2
	out_byte(INT_S_CTLMASK, 0x2);
	
	// master 8259, ICW4
	out_byte(INT_M_CTLMASK, 0x1);
	
	// slave 8259, ICW4
	out_byte(INT_S_CTLMASK, 0x1);
	
	// master 8259, OCW1
	out_byte(INT_M_CTLMASK, 0xff);
	
	// slave 8259, OCW1
	out_byte(INT_S_CTLMASK, 0xff);
	
	int i = 0;
	for (i = 0; i < NR_IRQ; i++)
	{
		irq_table[i] = spurious_irq;
	}
}

void spurious_irq(int irq)
{
	disp_str("spurious_irq: ");
	disp_int(irq);
	disp_str("\n");
}

void put_irq_handler(int irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}
