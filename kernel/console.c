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

static void set_cursor(u32 position);
static void set_video_start_addr(u32 addr);
static void flush(CONSOLE *console);
static void w_copy(unsigned int dst, const unsigned int src, int size);
static void clear_screen(int pos, int len);

void init_screen(TTY *tty)
{
	int nr_tty = tty - tty_table;
	tty->console = console_table + nr_tty;
	
	int video_mem_word_size = VIDEO_MEM_SIZE >> 1;
	int size_per_con = video_mem_word_size / NR_CONSOLES;
	tty->console->orig = size_per_con * nr_tty;
	tty->console->con_size = size_per_con / SCREEN_WIDTH * SCREEN_WIDTH;
	tty->console->crtc_start = tty->console->orig;
	tty->console->cursor = tty->console->orig;
	tty->console->is_full = false;
	
	if (nr_tty == 0)
	{
		tty->console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		const char prompt[] = "[TTY #?]\n";
		const char *p = prompt;

		for (; *p; p++)
		{
			out_char(tty->console, *p == '?' ? nr_tty + '0' : *p);
		}
	}
	
	set_cursor(tty->console->cursor);
}

int is_current_console(CONSOLE *console)
{
	return (console == &console_table[nr_current_console]);
}

void out_char(CONSOLE *console, char ch)
{
	u8 *video_mem = (u8*)(VIDEO_MEM_BASE + console->cursor * 2);
	assert(console->cursor - console->orig < console->con_size);
	
	int cursor_x = (console->cursor - console->orig) % SCREEN_WIDTH;
	int cursor_y = (console->cursor - console->orig) / SCREEN_WIDTH;

	switch (ch)
	{
	case '\n':
		console->cursor = console->orig + SCREEN_WIDTH * (cursor_y + 1);
		break;
	case '\b':
		if (console->cursor > console->orig)
		{
			console->cursor--;
			*(video_mem - 2) = ' ';
			*(video_mem - 1) = DEFAULT_CHAR_COLOR;
		}
		break;
	default:
		*video_mem++ = ch;
		*video_mem++ = DEFAULT_CHAR_COLOR;
		console->cursor++;			
		break;
	}

	if (console->cursor - console->orig >= console->con_size)
	{
		cursor_x = (console->cursor - console->orig) % SCREEN_WIDTH;
		cursor_y = (console->cursor - console->orig) / SCREEN_WIDTH;

		int cp_orig = console->orig + (cursor_y + 1) * SCREEN_WIDTH - SCREEN_SIZE;
		w_copy(console->orig, cp_orig, SCREEN_SIZE - SCREEN_WIDTH);
		console->crtc_start = console->orig;
		console->cursor = console->orig + (SCREEN_SIZE - SCREEN_WIDTH) + cursor_x;
		clear_screen(console->cursor, SCREEN_WIDTH);

		if (!console->is_full)
		{
			console->is_full = true;
		}
	}

	assert(console->cursor - console->orig < console->con_size);

	while (console->cursor >= console->crtc_start + SCREEN_SIZE ||
		console->cursor < console->crtc_start)
	{
		scroll_screen(console, SCROLL_UP);
		clear_screen(console->cursor, SCREEN_WIDTH);
	}
	
	flush(console);
}

static void clear_screen(int pos, int len)
{
	u8 *pch = (u8*)(VIDEO_MEM_BASE + pos * 2);

	while (--len >= 0)
	{
		*pch++ = ' ';
		*pch++ = DEFAULT_CHAR_COLOR;
	}
}

static void flush(CONSOLE *console)
{
	if (is_current_console(console))
	{
		set_cursor(console->cursor);
		set_video_start_addr(console->crtc_start);
	}

#ifdef __TTY_DEBUG__
	int lineno = 0;
	for (lineno = 0; lineno < console->con_size / SCREEN_WIDTH; lineno++)
	{
		u8 * pch = (u8*)(VIDEO_MEM_BASE + (console->orig + (lineno + 1) * SCREEN_WIDTH) * 2 - 4);
		*pch++ = lineno / 10 + '0';
		*pch++ = RED_CHAR;
		*pch++ = lineno % 10 + '0';
		*pch++ = RED_CHAR;
	}
	
#endif //__TTY_DEBUG__
}

static void set_cursor(u32 position)
{
	disable_int();
	
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xff);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xff);
	
	enable_int();
}

static void set_video_start_addr(u32 addr)
{
	disable_int();
	
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xff);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xff);
	
	enable_int();	
}

void select_console(int nr_console)
{
	if (nr_console < 0 || nr_console >= NR_CONSOLES)
		return;
	
	nr_current_console = nr_console;
	
	//set_cursor(console_table[nr_console].cursor);
	//set_video_start_addr(console_table[nr_console].current_start_addr);
	flush(&console_table[nr_console]);
}

void scroll_screen(CONSOLE *console, int direction)
{
	int oldest;
	int newest;
	int scr_top;

	newest = (console->cursor - console->orig) / SCREEN_WIDTH * SCREEN_WIDTH;
	oldest = console->is_full ? (newest + SCREEN_WIDTH) % console->con_size : 0;
	scr_top = console->crtc_start - console->orig;

	if (direction == SCROLL_DOWN)
	{
		if (!console->is_full && scr_top > 0)
		{
			console->crtc_start -= SCREEN_WIDTH;
		}
		else if (console->is_full && scr_top != oldest)
		{
			if (console->cursor - console->orig >= console->con_size - SCREEN_SIZE)
			{
				if (console->crtc_start != console->orig)
				{
					console->crtc_start -= SCREEN_WIDTH;
				}
			}
			else if (console->crtc_start == console->orig)
			{
				scr_top = console->con_size - SCREEN_SIZE;
				console->crtc_start = console->orig + scr_top;
			}
			else
			{
				console->crtc_start -= SCREEN_WIDTH;
			}
		}
	}
	else if (direction == SCROLL_UP)
	{
		if (!console->is_full && newest >= scr_top + SCREEN_SIZE)
		{
			console->crtc_start += SCREEN_WIDTH;
		}
		else if (console->is_full && scr_top + SCREEN_SIZE - SCREEN_WIDTH != newest)
		{
			if (scr_top + SCREEN_SIZE == console->con_size)
			{
				console->crtc_start = console->orig;
			}
			else
			{
				console->crtc_start += SCREEN_WIDTH;
			}
		}
	}
	else
	{
		assert(direction == SCROLL_DOWN || direction == SCROLL_UP);
	}
	
	//set_video_start_addr(console->current_start_addr);
	//set_cursor(console->cursor);
	flush(console);
}

static void w_copy(unsigned int dst, const unsigned int src, int size)
{
	phys_copy((void*)(VIDEO_MEM_BASE + (dst << 1)),
		(void*)(VIDEO_MEM_BASE + (src << 1)),
		size << 1);
}
