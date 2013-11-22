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

int wait(int *status)
{
	MESSAGE msg;
	msg.type = WAIT;

	send_recv(BOTH, TASK_MM, &msg);

	*status = msg.STATUS;
	return (msg.PID == NO_TASK ? -1 : msg.PID);
}
