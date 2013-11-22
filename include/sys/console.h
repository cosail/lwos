#ifndef _CONSOLE_H_
#define _CONSOLE_H_

typedef struct console_t
{
	unsigned int crtc_start;
	unsigned int orig;
	unsigned int con_size;
	unsigned int cursor;
	int is_full;

} CONSOLE;


#define SCROLL_UP	0
#define SCROLL_DOWN	1

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH	80

#define DEFAULT_CHAR_COLOR	(MAKE_COLOR(BLACK, WHITE))
#define GRAY_CHAR			(MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR			(MAKE_COLOR(BLUE, RED) | BRIGHT)

#endif // _CONSOLE_H_
