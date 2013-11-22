#ifndef _PROTECT_H_
#define _PROTECT_H_

typedef struct descriptor_t
{
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  attr1;
    u8  limit_high_attr2;
    u8  base_high;
    
} DESCRIPTOR;

#define reassembly(high, high_shift, mid, mid_shift, low) \
	(((high) << (high_shift)) + \
	((mid) << (mid_shift)) + \
	(low))

typedef struct gate_t
{
	u16 offset_low;
	u16 selector;
	u8  dcount;
	u8  attribute;
	u16 offset_high;

} GATE;

typedef struct tss_t
{
	u32 backlink;
	u32 esp0;
	u32 ss0;
	u32 esp1;
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldt;
	u16 trap;
	u16 iobase;
	
} TSS;

#define INDEX_DUMMY		0
#define INDEX_FLAT_CODE	1
#define INDEX_FLAT_DATA	2
#define INDEX_VIDEO		3
#define INDEX_TSS		4
#define INDEX_LDT_FIRST	5

#define SELECTOR_DUMMY		0
#define SELECTOR_CODE		0x08
#define SELECTOR_DATA		0x10
#define SELECTOR_VIDEO		(0x18+3) // RPL=3
#define SELECTOR_TSS		0x20
#define SELECTOR_LDT_FIRST	0x28

#define SELECTOR_KERNEL_CS	SELECTOR_CODE
#define SELECTOR_KERNEL_DS	SELECTOR_DATA
#define SELECTOR_KERNEL_GS	SELECTOR_VIDEO

#define LDT_SIZE 2
#define INDEX_LDT_CODE	0
#define INDEX_LDT_DATA	1

#define SA_RPL_MASK	0xfffc
#define SA_RPL0		0
#define SA_RPL1		1
#define SA_RPL2		2
#define SA_RPL3		3

#define SA_TI_MASK	0xfffb
#define SA_TIG		0
#define SA_TIL		4

#define DA_32		0x4000
#define DA_LIMIT_4K	0x8000
#define LIMIT_4K_SHITF 12

#define DA_DPL0		0x00
#define DA_DPL1		0x20
#define DA_DPL2		0x40
#define DA_DPL3		0x60

#define DA_DR		0x90
#define DA_DRW		0x92
#define DA_DRWA		0x93
#define DA_C		0x98
#define DA_CR		0x9a
#define DA_CCO		0x9c
#define DA_CCOR		0x9e

#define DA_LDT		0x82
#define DA_TaskGate	0x85
#define DA_386TSS	0x89
#define DA_386CGate	0x8c
#define DA_386IGate	0x8e
#define DA_386TGate	0x8f

#define INT_VECTOR_DIVIDE		0x0
#define INT_VECTOR_DEBUG		0x1
#define INT_VECTOR_NMI			0x2
#define INT_VECTOR_BREAKPOINT	0x3
#define INT_VECTOR_OVERFLOW		0x4
#define INT_VECTOR_BOUNDS		0x5
#define INT_VECTOR_INVAL_OP		0x6
#define INT_VECTOR_COPROC_NOT	0x7
#define INT_VECTOR_DOUBLE_FAULT	0x8
#define INT_VECTOR_COPROC_SEG	0x9
#define INT_VECTOR_INVAL_TSS	0xa
#define INT_VECTOR_SEG_NOT		0xb
#define INT_VECTOR_STACK_FAULT	0xc
#define INT_VECTOR_PROTECTION	0xd
#define INT_VECTOR_PAGE_FAULT	0xe
#define INT_VECTOR_COPROC_ERR	0x10

#define INT_VECTOR_IRQ0		0x20
#define INT_VECTOR_IRQ8		0x28

#define INT_VECTOR_SYS_CALL	0x90

#define vir2phys(seg_base, vir) (u32)(((u32)seg_base) + (u32)(vir))

#define makelinear(seg, off) (u32)(((u32)(seg2linear(seg))) + (u32)(off))

#endif //_PROTECT_H_

