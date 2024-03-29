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

int memcmp(const void *s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0))
	{
		return (s1 -s2);
	}

	const char *p1 = (const char*)s1;
	const char *p2 = (const char*)s2;

	int i;
	for (i = 0; i < n; i++, p1++, p2++)
	{
		if (*p1 != *p2)
		{
			return (*p1 - *p2);
		}
	}
	
	return 0;
}

int strcmp(const char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
	{
		return (s1 - s2);
	}
	
	const char *p1 = s1;
	const char *p2 = s2;

	for (; *p1 && *p2; p1++, p2++)
	{
		if (*p1 != *p2)
		{
			break;
		}
	}
	
	return (*p1 - *p2);
}

char* strcat(char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
	{
		return (s1 - s2);
	}

	char *p1 = s1;
	for (; *p1; p1++)
	{
	}

	const char *p2 = s2;
	for (; *p2; p1++, p2++)
	{
		*p1 = *p2;
	}
	
	*p1 = 0;

	return s1;	
}

void spin(char *func_name)
{
	printl("\nspinning in %s ...\n", func_name);
	while (true) {}
}

void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printl("%c assert(%s) failed: file: %s, base_file: %s, line: %d", MAGIC_CHAR_ASSERT, exp, file, base_file, line);
	
	/*
	  If assertion fails in a TASK, the system will halt before printl() returns.
	  If it happens in a USER PROCESS, printl() will return like a common
	  routine and arrive here.
	  @see sys_printx()
	  
	  we use a forever loop to prevent the process from going on.
	*/
	spin("assertion_failure()");
	
	// should never arrive here
	__asm__ __volatile__("ud2");
}
