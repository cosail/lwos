#include "type.h"
#include "config.h"
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
#include "elf.h"

void get_boot_params(struct boot_params *pbp)
{
	disp_str("---get_boot_params---\n");
	int *p = (int*)BOOT_PARAM_ADDR;
	__asm__ __volatile__ ("xchg %bx, %bx");
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->kernel_file = (unsigned char*)(p[BI_KERNEL_FILE]);
	//assert(memcmp(pbp->kernel_file, ELFMAG, SELFMAG) == 0);
}

int get_kernel_map(unsigned int *b, unsigned int *l)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr* elf_header = (Elf32_Ehdr*)bp.kernel_file;

	if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
	{
		return -1;
	}

	*b = ~0;
	unsigned int t = 0;
	int i;

	for (i = 0; i < elf_header->e_shnum; i++)
	{
		Elf32_Shdr *section_header = (Elf32_Shdr*)(bp.kernel_file + elf_header->e_shoff
			+ i * elf_header->e_shentsize);

		if (section_header->sh_flags & SHF_ALLOC)
		{
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr + section_header->sh_size;

			if (*b > bottom)
			{
				*b = bottom;
			}

			if (t < top)
			{
				t = top;
			}
		}
	}
	
	assert(*b < t);
	*l = t - *b - 1;

	return 0;	
}

char* itoa(char *string, int number)
{
	char *p = string;
	char ch = 0;
	int i = 0;
	int flag = false;
	
	*p++ = '0';
	*p++ = 'x';
	
	if (number == 0)
	{
		*p++ = '0';
	}
	else
	{
		for (i=28; i>=0; i-=4)
		{
			ch = (number >> i) & 0xf;
			if (flag || ch > 0)
			{
				flag = true;
				ch += '0';
				if (ch > '9')
				{
					ch += 7;
				}
				
				*p++ = ch;
			}
		}
	}
	
	*p = 0;
	
	return string;
}

void disp_int(int integer)
{
	char output[16];
	itoa(output, integer);
	disp_str(output);	
}

void delay(int time)
{
	int i, j, k;
	for (i = 0; i < time; i++)
	{
		for (j = 0; j < 10; j++)
		{
			for (k = 0; k < 10000; k++) {}
		}
	}
}

