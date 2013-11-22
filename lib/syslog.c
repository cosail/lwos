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

int syslog(const char *fmt, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4);

	i = vsprintf(buf, fmt, arg);
	assert(strlen(buf) == i);

	return disklog(buf);
}
