#ifndef _HELLO_H
#define _HELLO_H

#include "packet.h"
#include "node.h"

#define PROTOCOL_HELLO 0

struct msg_hello {
	int type;
};

#define MSG_HELLO_SIZE sizeof(struct msg_hello)

#define MSG_TYPE_HELLO 1

#define HELLO_INTERVAL 10

struct agent *hello_agent_new(struct node *n);

#endif
