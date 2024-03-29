#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include "proc.h"
#include "string.h"
#include "fs.h"
#include "global.h"

void exit(int status)
{
	MESSAGE msg;
	msg.type = EXIT;
	msg.STATUS = status;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
}
