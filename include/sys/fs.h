#ifndef _FS_H_
#define _FS_H_

struct dev_drv_map {
	int driver_nr;
};

#define MAGIC_V1	0x111

struct super_block {
	u32 magic;
	u32 nr_inodes;
	u32 nr_sects;
	u32 nr_imap_sects;
	u32 nr_smap_sects;
	u32 n_1st_sect;

	u32 nr_inode_sects;
	u32 root_inode;
	u32 inode_size;
	u32 inode_isize_off;
	u32 inode_start_off;
	u32 dir_ent_size;
	u32 dir_ent_inode_off;
	u32 dir_ent_fname_off;

	int sb_dev;
};

#define SUPER_BLOCK_SIZE	56

struct inode {
	u32 i_mode;
	u32 i_size;
	u32 i_start_sect;
	u32 i_nr_sects;
	u8 _unused[16];

	int i_dev;
	int i_cnt;
	int i_num;
};

#define INODE_SIZE	32

#define MAX_FILENAME_LEN 12

struct dir_entry {
	int inode_nr;
	char name[MAX_FILENAME_LEN];
};

#define DIR_ENTRY_SIZE sizeof(struct dir_entry)

struct file_desc {
	int fd_mode;
	int fd_pos;
	int fd_cnt;
	struct inode *fd_inode;
};

#define RD_SECT(dev,sect_nr) rw_sector(DEV_READ, \
	dev, \
	(sect_nr) * SECTOR_SIZE, \
	SECTOR_SIZE, \
	TASK_FS, \
	fsbuf);

#define WR_SECT(dev,sect_nr) rw_sector(DEV_WRITE, \
	dev, \
	(sect_nr) * SECTOR_SIZE, \
	SECTOR_SIZE, \
	TASK_FS, \
	fsbuf);

#endif //_FS_H_