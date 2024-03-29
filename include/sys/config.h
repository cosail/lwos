#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MINOR_BOOT	MINOR_hd2a

#define BOOT_PARAM_ADDR		0x900
#define BOOT_PARAM_MAGIC	0xb007
#define BI_MAG			0
#define BI_MEM_SIZE		1
#define BI_KERNEL_FILE	2

// disk log
//#define ENABLE_DISK_LOG
#define SET_LOG_SECT_SMAP_AT_STARTUP
#define MEMSET_LOG_SECTS
#define NR_SECTS_FOR_LOG NR_DEFAULT_FILE_SECTS

#endif //_CONFIG_H_

