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

int close(int fd)
{
	MESSAGE msg;
	msg.type = CLOSE;
	msg.FD = fd;

	send_recv(BOTH, TASK_FS, &msg);

	return msg.RETVAL;
}