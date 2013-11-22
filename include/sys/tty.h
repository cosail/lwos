#ifndef _TTY_H_
#define _TTY_H_

#define TTY_IN_BYTES	256
#define TTY_OUT_BUF_LEN	2

struct tty_t;
struct console_t;

typedef struct tty_t
{
	u32 inbuf[TTY_IN_BYTES];
	u32 *inbuf_head;
	u32 *inbuf_tail;
	int inbuf_count;

	int tty_caller;
	int tty_procnr;
	void *tty_req_buf;
	int tty_left_cnt;
	int tty_trans_cnt;
	
	struct console_t *console;
	
} TTY;

#endif // _TTY_H_
