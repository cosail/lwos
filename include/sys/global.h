#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#ifdef GLOBAL_VARIABLES_HERE
#ifdef EXTERN
#undef EXTERN
#define EXTERN
#endif // EXTERN
#endif // GLOBAL_VARIABLES_HERE

EXTERN int ticks;
EXTERN int disp_pos;

EXTERN u8 gdt_ptr[6];
EXTERN DESCRIPTOR gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6];
EXTERN GATE idt[IDT_SIZE];

EXTERN u32 k_reenter;
EXTERN int nr_current_console;

EXTERN int key_pressed;

EXTERN TSS tss;
EXTERN PROCESS *p_proc_ready;
extern char		task_stack[];
extern PROCESS	proc_table[];
extern TASK		task_table[];
extern TASK		user_process_table[];
extern irq_handler irq_table[];
extern TTY tty_table[];
extern CONSOLE console_table[];

/* FS */
extern u8 *fsbuf;
extern const int FSBUF_SIZE;
extern struct dev_drv_map dd_map[];
EXTERN struct file_desc f_desc_table[NR_FILE_DESC];
EXTERN struct inode inode_table[NR_INODE];
EXTERN struct super_block super_block[NR_SUPER_BLOCK];
EXTERN MESSAGE fs_msg;
EXTERN PROCESS *pcaller;
EXTERN struct inode *root_inode;

// MM
EXTERN MESSAGE mm_msg;
extern u8* mmbuf;
extern const int MMBUF_SIZE;
EXTERN int memory_size;

// for test only
extern char *logbuf;
extern const int LOGBUF_SIZE;
extern char* logdiskbuf;
extern const int LOGDISKBUF_SIZE;

#endif //_GLOBAL_H_

