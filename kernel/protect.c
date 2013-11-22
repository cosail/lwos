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


static void init_idt_desc(
	u8 vector,
	u8 desc_type,
	int_handler handler,
	u8 privilege
	);

void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();
void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

void init_protect()
{
	init_8259A();
	
	init_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, divide_error, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, single_step_exception, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception, PRIVILEGE_USER);
	init_idt_desc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, PRIVILEGE_USER);
	init_idt_desc(INT_VECTOR_BOUNDS, DA_386IGate, bounds_check, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, inval_opcode, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, copr_not_available, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, inval_tss, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, stack_exception, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_PROTECTION, DA_386IGate, general_protection, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, copr_error, PRIVILEGE_KERNEL);
	
	init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, PRIVILEGE_KERNEL);
	init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, PRIVILEGE_KERNEL);
	
	init_idt_desc(INT_VECTOR_SYS_CALL, DA_386IGate, sys_call, PRIVILEGE_USER);
	
	// initialize TSS in GDT
	memset(&tss, 0, sizeof(tss));
	tss.ss0 = SELECTOR_KERNEL_DS;
	init_desc(&gdt[INDEX_TSS],
		makelinear(SELECTOR_KERNEL_DS, &tss),
		sizeof(tss) - 1,
		DA_386TSS);
	tss.iobase = sizeof(tss); // no I/O permission
	
	// initialize LDT in GDT
	int i = 0;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		memset(&proc_table[i], 0, sizeof(PROCESS));

		proc_table[i].ldt_sel = SELECTOR_LDT_FIRST + (i << 3);
		assert(INDEX_LDT_FIRST + i < GDT_SIZE);

		init_desc(&gdt[INDEX_LDT_FIRST + i],
			makelinear(SELECTOR_KERNEL_DS, proc_table[i].ldts),
			LDT_SIZE * sizeof(DESCRIPTOR) - 1,
			DA_LDT);
	}
}

static void init_idt_desc(
	u8 vector,
	u8 desc_type,
	int_handler handler,
	u8 privilege
	)
{
	GATE *gate = &idt[vector];
	u32 base = (u32)handler;
	gate->offset_low = base & 0xffff;
	gate->selector = SELECTOR_KERNEL_CS;
	gate->dcount = 0;
	gate->attribute = desc_type | (privilege << 5);
	gate->offset_high = (base >> 16) & 0xffff;
}

u32 seg2linear(u16 seg)
{
	DESCRIPTOR *dest = &gdt[seg >> 3];
	return (dest->base_high << 24) | (dest->base_mid << 16) | dest->base_low;
}

void init_desc(
	DESCRIPTOR *desc,
	u32 base,
	u32 limit,
	u16 attribute
	)
{
	desc->limit_low = limit & 0xffff;
	desc->base_low = base & 0xffff;
	desc->base_mid = (base >> 16) & 0xff;
	desc->attr1 = attribute & 0xff;
	desc->limit_high_attr2 = ((limit >> 16) & 0xf) | ((attribute >> 8) & 0xf0);
	desc->base_high = (base >> 24) & 0xff;
}

void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	int i = 0;
	int text_color = 0x74;
	
	char *err_msg[] = {
		"#DE Divide Error",
		"#DB RESERVED",
		"--  NMI Interrupt",
		"#BP Breakpoint",
		"#OF Overflow",
		"#BR BOUND Range Exceeded",
		"#UD Invalid Opcode (Undefined Opcode)",
		"#NM Device Not Available (No Math Coprocessor)",
		"#DF Double Fault",
		"    Coprocessor Segment Overrun (reserved)",
		"#TS Invalid TSS",
		"#NP Segment Not Present",
		"#SS Stack-Segment Fault",
		"#GP General Protection",
		"#PF Page Fault",
		"--  (Intel reserved. Do not use)",
		"#MF x87 FPU Floating-Point Error (Math Fault)",
		"#AC Alignment Check",
		"#MC Machine Check",
		"#XF SIMD Floating-Point Exception"
	};
	
	disp_pos = 0;
	for (i = 0; i < 80 * 5; i++)
	{
		disp_str(" ");
	}
	
	disp_pos = 0;
	
	disp_color_str("Exception! --> ", text_color);
	disp_color_str(err_msg[vec_no], text_color);
	disp_color_str("\n\n", text_color);
	disp_color_str("EFLAGS:", text_color);
	disp_int(eflags);
	disp_color_str("CS:", text_color);
	disp_int(cs);
	disp_color_str("EIP:", text_color);
	disp_int(eip);
	
	if (err_code != 0xffffffff)
	{
		disp_color_str("Error code:", text_color);
		disp_int(err_code);
	}
}
