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

int read(int fd, void *buf, int count)
{
	MESSAGE msg;
	msg.type = READ;
	msg.FD = fd;
	msg.BUF = buf;
	msg.CNT = count;

	send_recv(BOTH, TASK_FS, &msg);

	return msg.CNT;
}
