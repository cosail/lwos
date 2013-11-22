#ifndef _PROTO_H_
#define _PROTO_H_

// klib.asm
void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);
void disp_str(char *string);
void disp_color_str(char *string, int color);
void disable_irq(int irq);
void enable_irq(int irq);
void disable_int();
void enable_int();
void port_read(u16 port, void *buf, int n);
void port_write(u16 port, void *buf, int n);
void glitter(int row, int col);

// string.c
//char* strcpy(char *dst, char *src); defined in string.h

// protect.c
void init_protect();
u32 seg2linear(u16 seg);
void init_desc(struct descriptor_t *p_desc, u32 base, u32 limit, u16 attribute);

// klib.c
void get_boot_params(struct boot_params *pbp);
int get_kernel_map(unsigned int *b, unsigned int *l);
void delay(int time);
void disp_int(int input);
char* itoa(char *str, int num);

// kernel.asm
void restart();

// main.c
void Init();
int get_ticks();
void panic(const char *format, ...);
void TestA();
void TestB();
void TestC();

// i8259.c
void init_8259A();
void put_irq_handler(int irq, irq_handler handler);
void spurious_irq(int irq);

// clock.c
void clock_handler(int irq);
void init_clock();
void milli_delay(int milli_sec);

// hd.c
void task_hd();
void hd_handler(int irq);

// keryboard.c
void init_keyboard();
void keyboard_read(TTY *tty);

// tty.c
void task_tty();
void in_process(TTY *tty, u32 key);
void dump_tty_buf();

// systask.c
void task_sys();

// fs/main.c
void task_fs();
int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
struct inode* get_inode(int dev, int num);
void put_inode(struct inode *p);
void sync_inode(struct inode *p);
struct super_block* get_super_block(int dev);

// fs/open.c
int do_open();
int do_close();

// fs/read_write.c
int do_rdwt();

// fs/link.c
int do_unlink();

// fs/misc.c
int do_stat();
int strip_path(char *filename, const char *pathname, struct inode **ppinode);
int search_file(char *path);

// fs/disklog.c
int do_disklog();
int disklog(char *logtr);
void dump_fd_graph(const char *fmt, ...);

// mm/main.c
void task_mm();
int alloc_mem(int pid, int memsize);
int free_mem(int pid);

// mm/forkexit.c
int do_fork();
void do_exit(int status);
void do_wait();

// console.c
void init_screen(TTY *tty);
void select_console(int nr_console);
void scroll_screen(CONSOLE *console, int direction);
void out_char(CONSOLE *console, char ch);
int is_current_console(CONSOLE *console);


// printf.c
int printf(const char *format, ...);
int printl(const char *format, ...);

// vsprintf.c
int vsprintf(char *buf, const char *format, va_list args);
int sprintf(char *buf, const char *format, ...);

// proc.c
void inform_init(int task_nr);

// lib/misc.c
void spin(char *func_name);

// syscall.asm
void sys_call();
int sendrec(int function, int src_dest, MESSAGE *msg);
int printx(char *str);

#endif //_PROTO_H_
