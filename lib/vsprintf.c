#include "type.h"
#include "stdio.h"
#include "const.h"
#include "string.h"

static char* i2a(int value, int base, char **ps)
{
	int m = value % base;
	int q = value / base;
	
	if (q)
	{
		i2a(q, base, ps);
	}
	
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');
	
	return *ps;
}

int vsprintf(char *buf, const char *format, va_list args)
{
	char *p;
	char tmp[STR_DEFAULT_LEN];
	va_list next_arg = args;
	char cs;
	int m;
	int align_nr;
	
	for (p = buf; *format != 0; format++)
	{
		if (*format != '%')
		{
			*p++ = *format;
			continue;
		}
		else
		{
			align_nr = 0;
		}
		
		format++;
		
		if (*format == '%')
		{
			*p++ = *format;
			continue;
		}
		else if (*format == '0')
		{
			cs = '0';
			format++;
		}
		else
		{
			cs = ' ';
		}
		
		while (((u8)(*format) >= '0') && ((u8)(*format) <= '9'))
		{
			align_nr *= 10;
			align_nr += *format - '0';
			format++;
		}
		
		char *q = tmp;
		memset(q, 0, sizeof(tmp));
		
		switch (*format)
		{
		case 'c':
			*q++ = *((char*)next_arg);
			next_arg += 4;
			break;
		case 'x':
			m = *((int*)next_arg);
			i2a(m, 16, &q);
			next_arg += 4;
			break;
		case 'd':
			m = *((int*)next_arg);
			if (m < 0)
			{
				m = m * (-1);
				*q++ = '-';
			}
			i2a(m, 10, &q);
			next_arg += 4;
			break;
		case 's':
			strcpy(q, (*((char**)next_arg)));
			q += strlen(*((char**)next_arg));
			next_arg += 4;
			break;
		default:
			break;
		}
		
		int k;
		for (k = 0; k < ((align_nr > strlen(tmp)) ? (align_nr - strlen(tmp)) : 0); k++)
		{
			*p++ = cs;
		}
		
		q = tmp;
		while (*q != 0)
		{
			*p++ = *q++;
		}
	}
	
	*p = 0;
	
	return (p - buf);
}

int sprintf(char *buf, const char *format, ...)
{
	va_list args = (va_list)((char*)&format + 4);
	return vsprintf(buf, format, args);
}
