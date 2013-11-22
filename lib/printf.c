#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

int printf(const char *format, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list args = (va_list)((char*)(&format) + 4);
	i = vsprintf(buf, format, args);

	int c = write(1, buf, i);
	assert(c == i);

	return i;
}

int printl(const char *format, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];
	
	va_list args = (va_list)((char*)(&format) + 4);
	i = vsprintf(buf, format, args);
	buf[i] = 0;
	printx(buf);
	
	return i;
}
