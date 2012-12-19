#include <stdio.h>

#include "agent.h"
#include "node.h"
#include "packet.h"

int agent_register(struct node *n, struct agent *a)
{
	int i;

	if (!n || !a)
		return -1;

	for (i = 0; i < MAX_AGENTS; i++) {
		if (n->agents[i] == NULL) {
			n->agents[i] = a;
			a->init(a);
			return 0;
		}
	}
	fprintf(stderr, "MAX agents for node %u reached\n", n->id);
	
	return -1;
}

int agent_deregister(struct node *n, unsigned int agent_num)
{
	if (!n)
		return -1;

	struct agent *a;

	a = n->agents[agent_num];
	
      	n->agents[agent_num] = NULL;
	
	a->fini(a);
	
	return 1;
}

int agent_recv(struct node *n, struct packet *p)
{
	struct agent *a;

	if (!n || !p)
		return -1;
	
	a = n->agents[p->protocol];

	if (!a)
		return -1;
	
	return a->recv(a, p);
}
