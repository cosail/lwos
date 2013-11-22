#ifndef _STRING_H_
#define _STRING_H_

void* memcpy(void *dst, void *src, int size);
void memset(void *dst, char value, int size);
int memcmp(const void *s1, const void *s2, int n);

int strlen(const char *string);
int strcmp(const char *s1, const char *s2);
char* strcpy(char *dst, char *src);
char* strcat(char *s1, const char *s2);


/*
  'phys_copy' and 'phys_set' are used only in the kernel, where segments
  are all flat (based on 0). In the meanwhile, currently linear address
  space is mapped to the identical physical address space. Therefore, a
  'physical copy' will be as same as a common copy, so does 'phys_set'.
*/
#define phys_copy	memcpy
#define phys_set	memset

#endif //_STRING_H_

