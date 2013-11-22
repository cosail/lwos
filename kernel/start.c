#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include "fs.h"
#include "global.h"

void cstart()
{
    disp_str("\n\n\n------cstart begin------\n");

	// move the gdt
    u16 *gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *gdt_base = (u32*)(&gdt_ptr[2]);
    
    memcpy(gdt, (void*)(*gdt_base), *gdt_limit + 1);
    
    *gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *gdt_base = (u32)gdt;
    
    // initialize idt
    u16 *idt_limit = (u16*)(&idt_ptr[0]);
    u32 *idt_base = (u32*)(&idt_ptr[2]);
    
    *idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *idt_base = (u32)idt;
    
    // initialize port
    init_protect();
    
    disp_str("------cstart end------\n");
}

