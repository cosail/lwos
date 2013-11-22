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

int unlink(const char *pathname)
{
	MESSAGE msg;
	msg.type = UNLINK;
	msg.PATHNAME = (void*)pathname;
	msg.NAME_LEN = strlen(pathname);

	//printl("unlink(%s)\n", pathname);
	send_recv(BOTH, TASK_FS, &msg);

	return msg.RETVAL;
}
