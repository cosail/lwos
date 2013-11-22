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
#include "hd.h"

static void init_hd();
static void hd_open(int device);
static void hd_close(int device);
static void hd_rdwt(MESSAGE *msg);
static void hd_ioctl(MESSAGE *msg);
static void hd_cmd_out(struct hd_cmd *cmd);
static void get_part_table(int drive, int sect_nr, struct part_ent *entry);
static void partition(int device, int style);
static void print_hdinfo(struct hd_info *hdi);
static int waitfor(int mask, int val, int timeout);
static void interrupt_wait();
static void hd_identify(int drive);
static void print_identify_info(u16 *hdinfo);

static u8 hd_status;
static u8 hdbuf[SECTOR_SIZE * 2];
static struct hd_info hd_info[1];

#define DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
	dev / NR_PRIM_PER_DRIVE : \
	(dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

void task_hd(void)
{
	MESSAGE msg;

	init_hd();

	while (true)
	{
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;

		switch (msg.type)
		{
		case DEV_OPEN:
			hd_open(msg.DEVICE);
			break;

		case DEV_CLOSE:
			hd_close(msg.DEVICE);
			break;

		case DEV_READ:
		case DEV_WRITE:
			hd_rdwt(&msg);
			break;

		case DEV_IOCTL:
			hd_ioctl(&msg);
			break;

		default:
			dump_msg("HD driver::unknown message", &msg);
			spin("FS::main_loop(invalid msg.type)");
			break;
		}

		send_recv(SEND, src, &msg);
	}
}

/*
 * <ring 1> check hard drive, set IRQ handler, enable IQR and initialize data
 * structures.
 */
static void init_hd()
{
	int i;

	// get the number of drives from the BIOS data area
	u8 *pNrDrives = (u8*)(0x475);
	printl("HD::NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);

	for (i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
	{
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	}

	hd_info[0].open_cnt = 0;
}

/*
 *	<ring 1> get the disk information
 */
static void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_INDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16*)hdbuf);
	
	u16 * hdinfo = (u16*)hdbuf;
	hd_info[drive].primary[0].base = 0;
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

/*
 *	<ring 1> this routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * ha not been read.
 */
static void hd_open(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);

	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0)
	{
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(&hd_info[drive]);
	}
}

/*
 * <ring 1> this routine handles DEV_CLOSE message.
 *
 * @param device - the device to be close.
 */
static void hd_close(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);

	hd_info[drive].open_cnt--;
}

/*
 * <ring 1> this routine handles DEV_READ and DEV_WRITE message.
 *
 * @param msg - message
 */
static void hd_rdwt(MESSAGE *msg)
{
	int drive = DRV_OF_DEV(msg->DEVICE);

	u64 pos = msg->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

	assert((pos & 0x1ff) == 0);

	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT);
	int logidx = (msg->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	sect_nr += msg->DEVICE < MAX_PRIM ?
		hd_info[drive].primary[msg->DEVICE].base :
		hd_info[drive].logical[logidx].base;

	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.count = (msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low = sect_nr & 0xff;
	cmd.lba_mid = (sect_nr >> 8) & 0xff;
	cmd.lba_high = (sect_nr >> 16) & 0xff;
	cmd.device = MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xf);
	cmd.command = (msg->type == DEV_READ) ? ATA_READ : ATA_WRITE;

	hd_cmd_out(&cmd);

	int bytes_left = msg->CNT;
	void *la = (void*)va2la(msg->PROC_NR, msg->BUF);

	while (bytes_left > 0)
	{
		int bytes = min(SECTOR_SIZE, bytes_left);
		if (msg->type == DEV_READ)
		{
			interrupt_wait();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(la, (void*)va2la(TASK_HD, hdbuf), bytes);
		}
		else
		{
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
			{
				panic("hd writing error.");
			}

			port_write(REG_DATA, la, bytes);
			interrupt_wait();
		}

		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}

/*
 * <ring 1> this routine handles the DEV_IOCTL messge.
 *
 * @param msg - message.
 */
static void hd_ioctl(MESSAGE *msg)
{
	int device = msg->DEVICE;
	int drive = DRV_OF_DEV(device);

	struct hd_info *hdi = &hd_info[drive];

	if (msg->REQUEST == DIOCTL_GET_GEO)
	{
		void *dst = va2la(msg->PROC_NR, msg->BUF);
		void *src = va2la(TASK_HD, device < MAX_PRIM ?
			&hdi->primary[device] :
			&hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE]);
		phys_copy(dst, src, sizeof(struct part_info));
	}
	else
	{
		assert(0);
	}
}

/*
 * <ring 1> get the partition table of a drive
 * 
 * @param drive drive number(0 for the 1st disk, 1 for the 2nd disk,...)
 * @param sect_nr the sector at which the partition table is located.
 * @param entry pointer to part_ent struct.
 */
static void get_part_table(int drive, int sect_nr, struct part_ent *entry)
{
	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.count = 1;
	cmd.lba_low = sect_nr & 0xff;
	cmd.lba_mid = (sect_nr >> 8) & 0xff;
	cmd.lba_high = (sect_nr >> 16) & 0xff;
	cmd.device = MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xf);
	cmd.command = ATA_READ;

	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry, hdbuf + PARTITION_TABLE_OFFSET,
		sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

/*
 * <ring 1> this routine is called when a device is opened. It reads the
 * partition table and fills the hd_info struct.
 *
 * @param device - device number.
 * @param style - P_PRIMARY or P_EXTENDED.
 */
static void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info *hdi = &hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY)
	{
		get_part_table(drive, drive, part_tbl);

		int nr_prim_parts = 0;
		for (i = 0; i< NR_PART_PER_DRIVE; i++)
		{
			if (part_tbl[i].sys_id == NO_PART)
			{
				continue;
			}

			nr_prim_parts++;
			int dev_nr = i + 1;
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

			if (part_tbl[i].sys_id == EXT_PART)
				partition(device + dev_nr, P_EXTENDED);
		}
		
		assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED)
	{
		int j = device % NR_PRIM_PER_DRIVE;
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART;

		for (i = 0; i < NR_SUB_PER_PART; i++)
		{
			int dev_nr = nr_1st_sub + i;

			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

			s = ext_start_sect + part_tbl[1].start_sect;

			if (part_tbl[1].sys_id == NO_PART)
			{
				break;
			}
		}
	}
	else
	{
		assert(0);
	}
}

/*
 * <ring 1> print disk info.
 *
 * @param hdi - pointer to struct hd_info.
 */
static void print_hdinfo(struct hd_info *hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++)
	{
		printl("HD::%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
			i == 0 ? " " : "    ",
			i,
			hdi->primary[i].base,
			hdi->primary[i].base,
			hdi->primary[i].size,
			hdi->primary[i].size);
	}

	for (i = 0; i < NR_SUB_PER_DRIVE; i++)
	{
		if (hdi->logical[i].size == 0)
		{
			continue;
		}

		printl("HD::        "
			"%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
			i,
			hdi->logical[i].base,
			hdi->logical[i].base,
			hdi->logical[i].size,
			hdi->logical[i].size);
	}
}

/*
 *	<ring 1> print the hdinfo retrieved via ATA_IDENTIFY command.
 */
static void print_identify_info(u16 *hdinfo)
{
	int i, k;
	char s[64];

	struct iden_info_ascii {
		int idx;
		int len;
		char *desc;
	} iinfo[] = {{10, 20, "HD SN"},
				 {27, 40, "HD Model"}};

	for (k = 0; k < sizeof(iinfo) / sizeof(iinfo[0]); k++)
	{
		char *p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len / 2; i++)
		{
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}

		s[i*2] = 0;
		printl("HD::%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printl("HD::LBA supported: %s\n", (capabilities & 0x0200) ? "yes" : "no");

	int cmd_set_supported = hdinfo[83];
	printl("HD::LBA48 supported: %s\n", (cmd_set_supported & 0x0400) ? "yes" : "no");
	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printl("HD::size: %dMB, %dKB\n", sectors * 512 / 1000000, sectors * 512);	
}

/*
 *	<ring 1> output a command to HD controller.
 */
static void hd_cmd_out(struct hd_cmd *cmd)
{
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
	{
		panic("hd error.");
	}

	out_byte(REG_DEV_CTRL, 0);
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR, cmd->count);
	out_byte(REG_LBA_LOW, cmd->lba_low);
	out_byte(REG_LBA_MID, cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE, cmd->device);
	out_byte(REG_CMD, cmd->command);
}

/*
 *	<ring 1> wait until a disk interrupt occurs.
 */
static void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE, INTERRUPT, &msg);
}

/*
 *	<ring 1> wait for a certain status.
 */
static int waitfor(int mask, int val, int timeout)
{
	int t = get_ticks();

	while (((get_ticks() - t) * 1000 / HZ) < timeout)
	{
		if ((in_byte(REG_STATUS) & mask) == val)
		{
			return 1;
		}
	}
	
	return 0;
}

/*
 *	<ring 0> interrupt handler.
 */
void hd_handler(int irq)
{
	hd_status = in_byte(REG_STATUS);

	inform_init(TASK_HD);
}
