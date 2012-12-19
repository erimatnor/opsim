#ifndef _AGENT_H
#define _AGENT_H

#include "packet.h"

struct agent {
	unsigned int id;
	struct node *n;
	int (* init) (struct agent *);
	int (* recv) (struct agent *, struct packet *);
	int (* fini) (struct agent *);
};

#define MAX_AGENTS 10

int agent_register(struct node *n, struct agent *a);
int agent_recv(struct node *n, struct packet *p);
int agent_deregister(struct node *n, unsigned int agent_num);

#endif /* _AGENT_H */
