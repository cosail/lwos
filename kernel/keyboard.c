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
#include "keymap.h"


static KB_INPUT kb_in = {0};

static int code_with_e0;
static int shift_l;
static int shift_r;
static int alt_l;
static int alt_r;
static int ctrl_l;
static int ctrl_r;
static int caps_lock;
static int num_lock;
static int scroll_lock;
static int column;

static u8 get_byte_from_kbuf();
static void set_leds();
static void kb_wait();
static void kb_ack();

void keyboard_handler(int irq)
{
	u8 scan_code = in_byte(KB_DATA);
	//disp_str("keyboard interrupt handler\n");
	//disp_int(kb_in.count);
	
	if (kb_in.count < KB_IN_BYTES)
	{
		*(kb_in.head) = scan_code;
		kb_in.head++;
		
		if (kb_in.head == kb_in.buf + KB_IN_BYTES)
		{
			kb_in.head = kb_in.buf;
		}
		
		kb_in.count++;
		//disp_str("keyboard_handler: +++ ");
		//disp_int(kb_in.count);
		//disp_str("\n");
	}

	key_pressed = 1;
}

void init_keyboard()
{
	kb_in.count = 0;
	kb_in.head = kb_in.tail = kb_in.buf;
	
	code_with_e0 = false;

	shift_l = shift_r = false;
	alt_l = alt_r = false;
	ctrl_l = ctrl_r = false;
	
	caps_lock = false;
	num_lock = true;
	scroll_lock = false;
	
	column = 0;

	set_leds();
	
	put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
	enable_irq(KEYBOARD_IRQ);
}

void keyboard_read(TTY *tty)
{
	u8 scan_code = 0;
	int make;
	u32 key = 0;
	u32 *keyrow;
	
	while (kb_in.count > 0)
	{
		code_with_e0 = false;
		scan_code = get_byte_from_kbuf();
		
		// parse the scan code
		if (scan_code == 0xe1)
		{
			int i;
			u8 pausebreak_scode[] = {0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5};
			int is_pausebreak = true;
			for (i = 1; i < 6; i++)
			{
				if (get_byte_from_kbuf() != pausebreak_scode[i])
				{
					is_pausebreak = false;
					break;
				}
			}
			
			if (is_pausebreak)
				key = PAUSEBREAK;
		}
		else if (scan_code == 0xe0)
		{
			code_with_e0 = true;
			scan_code = get_byte_from_kbuf();
			
			// PrintScreen pressed
			if (scan_code == 0x2a)
			{
				code_with_e0 = false;
				if (scan_code = get_byte_from_kbuf() == 0xe0)
				{
					code_with_e0 = true;
					if (scan_code = get_byte_from_kbuf() == 0x37)
					{
						key = PRINTSCREEN;
						make = true;
					}
				}
			}
			
			// PrintScreen released
			else if (scan_code == 0xb7)
			{
				code_with_e0 = false;
				if (scan_code = get_byte_from_kbuf() == 0xe0)
				{
					code_with_e0 = true;
					if (scan_code = get_byte_from_kbuf() == 0xaa)
					{
						key = PRINTSCREEN;
						make = false;
					}
				}
			}
		}
		
		if ((key != PAUSEBREAK) && (key != PRINTSCREEN))
		{
			make = (scan_code & FLAG_BREAK ? false : true);
			keyrow = &keymap[(scan_code & 0x7f) * MAP_COLS];
			column = 0;
			
			int caps = shift_l || shift_r;
			if (caps_lock)
			{
				if (keyrow[0] >= 'a' && keyrow[0] <= 'z')
				{
					caps = !caps;
				}
			}
			
			if (caps)
				column = 1;
			
			if (code_with_e0)
			{
				//disp_int(scan_code);
				column = 2;
				//code_with_e0 = false;
			}
			
			key = keyrow[column];
			//disp_int(key);
			switch (key)
			{
			case SHIFT_L:
				shift_l = make;
				break;
			case SHIFT_R:
				shift_r = make;
				break;
			case CTRL_L:
				ctrl_l = make;
				break;
			case CTRL_R:
				ctrl_r = make;
				break;
			case ALT_L:
				alt_l = make;
				break;
			case ALT_R:
				alt_r = make;
				break;
			case CAPS_LOCK:
				if (make)
				{
					caps_lock = !caps_lock;
					set_leds();
				}
				break;
			case NUM_LOCK:
				if (make)
				{
					num_lock = !num_lock;
					set_leds();
				}
				break;
			case SCROLL_LOCK:
				if (make)
				{
					scroll_lock = !scroll_lock;
					set_leds();
				}
				break;
			default:
				break;				
			}
		}
			
		if (make)
		{
			int pad = false;
			
			if (key >= PAD_SLASH && key <= PAD_9)
			{
				pad = true;
				switch (key)
				{
				case PAD_SLASH:
					key = '/';
					break;
				case PAD_STAR:
					key = '*';
					break;
				case PAD_MINUS:
					key = '-';
					break;
				case PAD_PLUS:
					key = '+';
					break;
				case PAD_ENTER:
					key = ENTER;
					break;
				default:
					if (num_lock)
					{
						if (key >= PAD_0 && key <= PAD_9)
							key = key - PAD_0 + '0';
						else if (num_lock && key == PAD_DOT)
							key = '.';
					}
					else
					{
						switch (key)
						{
						case PAD_HOME:
							key = HOME;
							break;
						case PAD_END:
							key = END;
							break;
						case PAD_PAGEUP:
							key = PAGEUP;
							break;
						case PAD_PAGEDOWN:
							key = PAGEDOWN;
							break;
						case PAD_INS:
							key = INSERT;
							break;
						case PAD_UP:
							key = UP;
							break;
						case PAD_DOWN:
							key = DOWN;
							break;
						case PAD_LEFT:
							key = LEFT;
							break;
						case PAD_RIGHT:
							key = RIGHT;
							break;
						case PAD_DOT:
							key = DELETE;
							break;
						default:
							break;
						}
					}
					break;
				}
			}
					
			key |= shift_l ? FLAG_SHIFT_L : 0;
			key |= shift_r ? FLAG_SHIFT_R : 0;
			key |= ctrl_l ? FLAG_CTRL_L : 0;
			key |= ctrl_r ? FLAG_CTRL_R : 0;
			key |= alt_l ? FLAG_ALT_L : 0;
			key |= alt_r ? FLAG_ALT_R : 0;
			key |= pad ? FLAG_PAD : 0;
			
			in_process(tty, key);
		}
	}
}

static u8 get_byte_from_kbuf()
{
	u8 scan_code = 0;
	
	while (kb_in.count <= 0) {}
	
	disable_int();
	
	scan_code = *(kb_in.tail);
	kb_in.tail++;
	if (kb_in.tail == kb_in.buf + KB_IN_BYTES)
		kb_in.tail = kb_in.buf;
	kb_in.count--;
	
	enable_int();
	
	return scan_code;
}

static void kb_wait()
{
	u8 kb_stat;
	
	do {
		kb_stat = in_byte(KB_CMD);
	} while (kb_stat & 0x02);
}

static void kb_ack()
{
	u8 kb_read;
	
	do {
		kb_read = in_byte(KB_DATA);
	} while (kb_read != KB_ACK);
}

static void set_leds()
{
	u8 leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;
	
	kb_wait();
	out_byte(KB_DATA, LED_CODE);
	kb_ack();
	
	kb_wait();
	out_byte(KB_DATA, leds);
	kb_ack();
}
