#include <stdio.h>
#include <string.h>

#include "epidemic.h"
#include "event.h"
#include "node.h"
#include "trace.h"
#include "neigh.h"
#include "debug.h"
#include "bloomfilter.h"
#include "packet.h"
#include "simtime.h"

struct epidemic_id {
	unsigned int src;
	unsigned int dst;
	unsigned long seqno;
};


#define MAX_REC_ROUTE_HOPS 30

struct msg_epidemic {
	unsigned char type;
	unsigned int src;
	unsigned int dst;
	unsigned long seqno;
	unsigned int num_fwd;
	unsigned int len;
	simtime_t send_time;
	unsigned int rec_route[MAX_REC_ROUTE_HOPS];
	char *data;
};

#define EPIDEMIC_TYPE_MSG_BLOOMFILTER 1
#define EPIDEMIC_TYPE_MSG_DATA   2

struct epidemic_agent {
	struct agent a;
	unsigned long msg_seqno;
	struct bloomfilter *bf;
};

static int epidemic_recv(struct agent *a, struct packet *p);
static int epidemic_init(struct agent *a);
static int epidemic_fini(struct agent *a);
int epidemic_send_bloomfilter(struct epidemic_agent *ea, unsigned int neigh_id);

struct agent *epidemic_agent_new(struct node *n)
{
	struct epidemic_agent *ea;

	ea = (struct epidemic_agent *)malloc(sizeof(struct epidemic_agent));
		
	if (!ea) {
		fprintf(stderr, "Could not create Epidemic agent\n");
		return NULL;
	}
	
	memset(ea, 0, sizeof(struct epidemic_agent));
	
	ea->a.n = n;
	ea->a.id = PROTOCOL_EPIDEMIC;
	ea->a.init = epidemic_init;
	ea->a.recv = epidemic_recv;
	ea->a.fini = epidemic_fini;
	ea->msg_seqno = 0;

	ea->bf = bloomfilter_new(0.01, 1000);

	if (!ea->bf) {
		fprintf(stderr, "Could not allocate bloomfilter for node epidemic agent on node %u\n", n->id);
		free(ea);
		return NULL;
	}
	return &ea->a;

}
int epidemic_init(struct agent *a)
{

	return 0;
}

int epidemic_fini(struct agent *a)
{
	struct epidemic_agent *ea = (struct epidemic_agent *)a;

	if (!ea)
		return -1;

	bloomfilter_free(ea->bf);
	free(ea);

	return 0;
}

int epidemic_infect_neigh(struct node *n, unsigned int neigh_id)
{
	struct epidemic_agent *ea;
       
	if (!n)
		return -1;

	ea = (struct epidemic_agent *)n->agents[PROTOCOL_EPIDEMIC];

	if (!ea) {
		fprintf(stderr, "No Epidemic agent for node %u\n", n->id);
		return -1;
	}
	
	return epidemic_send_bloomfilter(ea, neigh_id);
}

int epidemic_infect_all_neighbors(struct epidemic_agent *ea, unsigned int neigh_id_exception)
{
	struct node *n;
	list_t *curr; 
	int num = 0;

	if (!ea || !ea->a.n)
		return -1;

	n = ea->a.n;

	list_for_each(curr, &n->neigh_list) {
		struct neighbor *neigh = (struct neighbor *)curr;
		
		if (neigh->id == neigh_id_exception)
			continue;

		if (epidemic_send_bloomfilter(ea, neigh->id)) 
			num++;
	}
	
	return num;
}
int epidemic_send_bloomfilter(struct epidemic_agent *ea, unsigned int neigh_id)
{
	struct packet *p;
	struct msg_epidemic *msgv;
	
	if (!ea || !ea->a.n)
		return -1;
	
	DEBUG("Node %u: sending bloomfilter to %u\n", ea->a.n->id, neigh_id);

	p = packet_new(sizeof(struct msg_epidemic) + 
		       sizeof(struct bloomfilter *));

	if (!p)
		return -1;

	p->protocol = PROTOCOL_EPIDEMIC;
	p->src = ea->a.n->id;
	p->dst = neigh_id;
	//p->virtual_len = sizeof(struct msg_epidemic) + sizeof(struct bloomfilter *);
		
	msgv = (struct msg_epidemic *)p->data;
	
	msgv->type = EPIDEMIC_TYPE_MSG_BLOOMFILTER;
	msgv->src = ea->a.n->id;
	msgv->dst = neigh_id;
	msgv->seqno = 0;
	
	msgv->data = (char *)ea->bf;

	if (node_xmit(ea->a.n, p) < 0) {
		fprintf(stderr, "Epidemic: xmit of vector failed\n");
		packet_free(p);
		return -1;
	}
	return 0;
}

int epidemic_check_bloomfilter_and_xmit(struct epidemic_agent *ea, 
					struct msg_epidemic *msg_e,
					unsigned int dst)
{
	struct node *n;
	struct bloomfilter *bf;
	list_t *curr;
	int num_pkts = 0;
	
	if (!msg_e || !ea || !ea->a.n)
		return -1;

	n = ea->a.n;

	bf = (struct bloomfilter *)msg_e->data;
	
	node_packet_buffer_print(n);

	list_for_each(curr, &n->packet_buffer) {
		struct packet *p = (struct packet *)curr;
		struct msg_epidemic *msg_e;

		if (!p->data)
			continue;

		msg_e = (struct msg_epidemic *)p->data;

		if (msg_e->type == EPIDEMIC_TYPE_MSG_DATA) {
		
			/* If the packet is not in the filter, send
			   the packet */

			if (0 == bloomfilter_check(bf, (char *)&msg_e->src, 
						   sizeof(struct epidemic_id))) {
				struct packet *p_clone = packet_clone(p);
				
				if (!p_clone)
					continue;
				
				p_clone->src = n->id;
				p_clone->dst = dst;

				msg_e = (struct msg_epidemic *)p_clone->data;
				
				if (node_xmit(n, p_clone) < 0) {
					fprintf(stderr, "Epidemic: xmit of data failed\n");
					packet_free(p_clone);
					return -1;
				}
				num_pkts++;
			} else {
				//DEBUG("Node %u: Message already in filter, not sending to node %u\n", ea->a.n->id, dst);
			}
		}
	}
	DEBUG("Node %u: sent %d pkts to %u\n",  n->id, num_pkts, dst);

	return 0;

}
char *print_rec_route(unsigned int *rec_route, unsigned int num)
{
	static char buf[1024];
	int i, idx = 0;

	if (num == 0)
		return "";

	for (i = 0; i < num - 1; i++) {
		idx = idx + snprintf(buf+idx, 1024 - idx, "%u-", rec_route[i]);
	}
	snprintf(buf+idx, 1024 - idx, "%u", rec_route[i]);

	return buf;
}
int epidemic_recv(struct agent *a, struct packet *p) 
{
	struct epidemic_agent *ea = (struct epidemic_agent *)a;
	struct msg_epidemic *msg_e;

	if (!ea || !a->n || !p) {
		return -1;
	}
	DEBUG("received epidemic packet\n");

	msg_e = (struct msg_epidemic *)p->data;

	if (!msg_e)  {
		fprintf(stderr, "No epidemic header\n");
		return -1;
	}
	
	msg_e->num_fwd++;

	switch(msg_e->type) {
	case EPIDEMIC_TYPE_MSG_BLOOMFILTER:
		DEBUG("Node %u: received BLOOMFILTER from %u\n", a->n->id, p->src);
		epidemic_check_bloomfilter_and_xmit(ea, msg_e, p->src);
		break;
	case EPIDEMIC_TYPE_MSG_DATA:
		DEBUG("Node %u: received DATA from %u msg=(%u,%u)\n", a->n->id, p->src, msg_e->src, msg_e->dst);
	
		/* Add to bloom filter */
		bloomfilter_add(ea->bf, (char *)&msg_e->src, sizeof(struct epidemic_id));		
		if (msg_e->dst == a->n->id) {
			if (msg_e->num_fwd == 1)
				TRACE("%-3u epidemic r %-3u %-3u %-3lu %-2u %lf %u-%u\n", a->n->id, msg_e->src, msg_e->dst, msg_e->seqno, msg_e->num_fwd, simtime_get() - msg_e->send_time, msg_e->src, msg_e->dst);
			else
				TRACE("%-3u epidemic r %-3u %-3u %-3lu %-2u %lf %u-%s-%u\n", a->n->id, msg_e->src, msg_e->dst, msg_e->seqno, msg_e->num_fwd, simtime_get() - msg_e->send_time, msg_e->src, print_rec_route(msg_e->rec_route, msg_e->num_fwd-1), msg_e->dst);
			packet_free(p);
			return 1;
		} else {
			
			if (msg_e->num_fwd < MAX_REC_ROUTE_HOPS)
				msg_e->rec_route[msg_e->num_fwd - 1] = a->n->id;

			if (node_buffer_packet(ea->a.n, p) < 0) {
				fprintf(stderr, "Error buffering packet\n");
				packet_free(p);
				return -1;
			}
			//epidemic_infect_all_neighbors(ea, p->src);
		}
		
		break;
	}

	/* Try to infect all neighbors */
	
	//epidemic_infect_all_neigh(a->n);

	return 0;
}

int epidemic_initiate_data(struct node *n, unsigned int dst, unsigned int len)
{
	struct packet *p;
	struct epidemic_agent *ea;
	struct msg_epidemic *msg_e;

	if (!n)
		return -1;

	ea = (struct epidemic_agent *)n->agents[PROTOCOL_EPIDEMIC];

	if (!ea) {
		fprintf(stderr, "No Epidemic agent for node %u\n", n->id);
		return -1;
	}
	p = packet_new(sizeof(struct msg_epidemic));

	if (!p) {
		fprintf(stderr, "Could not send epidemic data\n");
		return -1;
	}
	
	p->protocol = PROTOCOL_EPIDEMIC;
	p->virtual_len = len + sizeof(struct msg_epidemic);
	p->src = n->id;
	
	msg_e = (struct msg_epidemic *)p->data;
	
	msg_e->type = EPIDEMIC_TYPE_MSG_DATA;
	msg_e->src = n->id;
	msg_e->dst = dst;
	msg_e->seqno = ea->msg_seqno++;
	msg_e->len = len;
	msg_e->num_fwd = 0;
	msg_e->send_time = simtime_get();	

	if (node_buffer_packet(n, p) < 0) {
		fprintf(stderr, "Error buffering packet\n"); 
		packet_free(p); 
		return -1;
	}
	
	if (neigh_find(n, dst)) {
		struct packet *p_clone = packet_clone(p);
		
		if (!p_clone) {
			fprintf(stderr, "Could not clone packet\n");
			return -1;
		}
		
		p_clone->dst = dst;
		
		if (node_xmit(n, p_clone) < 0) {
			fprintf(stderr, "Epidemic: xmit of data failed\n");
			packet_free(p_clone);
			return -1;
		}
	}
	
	DEBUG("Node %u: Initiated epidemic data for node %u\n", n->id, msg_e->dst);
	bloomfilter_add(ea->bf, (char *)&msg_e->src, sizeof(struct epidemic_id));
	TRACE("%-3u epidemic s %-3u %-3u %-3lu\n", 
	      n->id, msg_e->src, msg_e->dst, msg_e->seqno);
	
	return 0;
}
