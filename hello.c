#include <stdlib.h>
#include <string.h>

#include "hello.h"
#include "debug.h"
#include "agent.h"
#include "node.h"
#include "simulator.h"
#include "packet.h"
#include "list.h"
#include "neigh.h"

struct hello_agent {
	struct agent a;
	struct event *e;
	unsigned long hello_seqno;
	unsigned long hello_recvd;
};

static int hello_recv(struct agent *a, struct packet *p);
static int hello_init(struct agent *a);
static int hello_fini(struct agent *a);
static void hello_event(struct event *e);

struct agent *hello_agent_new(struct node *n)
{
	struct hello_agent *ha;

	ha = (struct hello_agent *)malloc(sizeof(struct hello_agent));
	
	if (!ha) {
		fprintf(stderr, "Could not create HELLO agent\n");
		return NULL;
	}
	
	memset(ha, 0, sizeof(struct hello_agent));
	
	ha->a.n = n;
	ha->a.id = PROTOCOL_HELLO;
	ha->a.init = hello_init;
	ha->a.recv = hello_recv;
	ha->a.fini = hello_fini;
	ha->hello_seqno = 0;
	ha->hello_recvd = 0;

	ha->e = event_new(ha->a.n, 0, hello_event, NULL);	

	if (!ha->e) {
		free(ha);
		return NULL;
	}
	return &ha->a;
}

int hello_fini(struct agent *a)
{
	struct hello_agent *ha = (struct hello_agent *)a;

	if (!ha)
		return -1;

	DEBUG("free HELLO agent for node %u, sent/recvd HELLOS=%lu/%lu\n", 
	      ha->a.n->id, ha->hello_seqno, ha->hello_recvd);

	event_free(ha->e);
	free(ha);

	return 0;
}

int hello_create(char *buf, int len)
{
	struct msg_hello *msg;

	if (!buf || len < sizeof(struct msg_hello))
		return -1;

	msg = (struct msg_hello *)buf;

	msg->type = MSG_TYPE_HELLO;

	return 0;
}

int hello_send(struct agent *a)
{
	struct hello_agent *ha = (struct hello_agent *)a;
	struct packet *p;
	int res;

	if (!ha || !ha->a.n)
		return -1;

	p = packet_new(MSG_HELLO_SIZE);
	
	if (!p)
		return -1;

	p->protocol = PROTOCOL_HELLO;
	p->src = ha->a.n->id;
	p->dst = BROADCAST;
	p->ttl = 1;

	hello_create(p->data, MSG_HELLO_SIZE);
	
	res = node_xmit(ha->a.n, p);

	if (res < 0) {
		packet_free(p);
	} else 
		ha->hello_seqno++;
	
	DEBUG("HELLO num=%lu for node %u\n", ha->hello_seqno, ha->a.n->id);

	return res;	
}

void hello_event(struct event *e)
{
	
	struct node *n = (struct node *)e->node;
	//struct event *e_new;
	float delay;
	
	if (!n) {
		DEBUG("node pointer is NULL!!!\n");
		return;
	}
	
	delay = (float)(rand() / 2) / (RAND_MAX);

	e->time = simtime_get() + HELLO_INTERVAL + delay;
	
	DEBUG("Scheduling next hello at %f\n", e->time);
      
	if (simulator_schedule(e) < 0)
		return;

	hello_send(n->agents[PROTOCOL_HELLO]);
}

int hello_init(struct agent *a)
{
	struct hello_agent *ha = (struct hello_agent *)a;
	float delay;
	
	if (!a || !a->n)
		return -1;
	
	/* Start hellos for each node */
	
	delay = (float)(rand() / 2) / (RAND_MAX);
	
	ha->e->time = simtime_get() + HELLO_INTERVAL + delay;
	
	if (simulator_schedule(ha->e) < 0)
		return -1;
	
	return 0;
}

int hello_recv(struct agent *a, struct packet *p)
{
	struct hello_agent *ha = (struct hello_agent *)a;

	if (!ha || !ha->a.n || !p)
		return -1;

	if (p->protocol != PROTOCOL_HELLO) {
		fprintf(stderr, "Demultiplexing Error!\n");
		return -1;
	}
	DEBUG("Node %u received an HELLO from node %u\n", a->n->id, p->src);

	if (neigh_find(ha->a.n, p->src)) {
		if (neigh_refresh(ha->a.n, p->src) < 0) 
			neigh_del(ha->a.n, p->src);
	} else { 
		neigh_add(ha->a.n, p->src);
	}
	ha->hello_recvd++;

	packet_free(p);
	
	return 0;
}
