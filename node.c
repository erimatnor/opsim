#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include "list.h"
#include "node.h"
#include "vector.h"
#include "simtime.h"
#include "packet.h"
#include "trace.h"
#include "simulator.h"
#include "debug.h"
#include "neigh.h"
#include "contact.h"

#define ABS(x) (((x) * (x)) / (x))

extern int contact_mode;
extern unsigned int max_packet_buffer_len;

#define MAX_PACKET_BUFFER_LEN max_packet_buffer_len

static int node_packet_buffer_flush(struct node *n);

int node_init(struct node *n, nodeid_t id)
{
	if (!n)
		return -1;

	memset(n, 0, sizeof(struct node));

	n->id = id;
	n->state = NODE_STATE_DEF;
	n->type = NODE_TYPE_DEF;
	n->radio_r = RADIO_RANGE;
	n->buffer_len = 0;
	n->bandwidth = BANDWIDTH;

	INIT_LIST(&n->neigh_list);
	INIT_LIST(&n->packet_buffer);

	return 0;
}

int node_fini(struct node *n)
{
	int i;

	DEBUG("Cleaning up node %u\n", n->id);

	neigh_list_flush(n);
	node_packet_buffer_flush(n);

	for (i = 0; i < MAX_AGENTS; i++) {
		if (n->agents[i])
			agent_deregister(n, i);
	}
	return 0;
}

struct node *node_new(nodeid_t id)
{
	struct node *n;

	n = (struct node *)malloc(sizeof(struct node));

	if (!n)
		return NULL;

	node_init(n, id);

	return n;
}

void node_free(struct node *n)
{
	free(n);
}

void node_set_pos(struct node *n, point2_t p)
{
	if (!n)
		return;

	DEBUG("Setting position of node %u to (%f, %f)\n", n->id, p[0], p[1]);

	memcpy(n->pos, p, sizeof(point2_t));
}

void node_set_speed(struct node *n, float speed)
{
	vector2_t norm_v; /* The normalized movement vector */

	if (!n)
		return;

	memcpy(norm_v, n->mov_v, sizeof(vector2_t));
	v2_normalize(norm_v);
	v2_scale(norm_v, speed, n->mov_v);

	return;
}

float node_get_speed(struct node *n)
{
	if (!n)
		return -1.0;

	return v2_length(n->mov_v);	
}

float node_distance(struct node *n1, struct node *n2)
{
	if (!n1 || !n2)
		return -1;

	return v2_distance(n1->pos, n2->pos);
}

int node_update_pos(struct node *n, simtime_t now)
{
	simtime_t t_diff;
	vector2_t translation;
	
	if (!n)
		return -1;

	if (ABS(n->dest[0] - n->pos[0]) < 0.01 &&  
	    ABS(n->dest[1] - n->pos[1]) < 0.01) {
		n->last_time = now;
		//DEBUG("Node %u pos=(%f, %f)\n", n->id, n->pos[0], n->pos[1]);
		return 0;
	}

	t_diff =  now - n->last_time;
	
//	printf("Time diff=%f, old pos = (%f, %f)\n", t_diff, n->pos[0], n->pos[1]);
	v2_scale(n->mov_v, t_diff, translation);

	v2_add(n->pos, translation, n->pos);
	
	n->last_time = now;
	//DEBUG("Node %u pos=(%f, %f)\n", n->id, n->pos[0], n->pos[1]);

	return 1;
}

int node_set_dest_and_speed(struct node *n, point2_t dest, float speed)
{
	if (!n)
		return -1;

	memcpy(n->dest, dest, sizeof(point2_t));
	
	v2_subtract(dest, n->pos, n->mov_v);

	node_set_speed(n, speed);

	return 0;	
}

int node_contact(struct node *n1, struct node *n2)
{
	float d;

	if (!n1 || !n2)
		return -1;

	if (n1->id == n2->id)
		return -1;

	if (contact_mode) {
		/* Hmm, when we check the contact it is perhaps better
		 * to treat the the src as the one responding on a
		 * scan. We therefore, give n2 (the dest) as the
		 * recording node */ 
		if (contact_check(simtime_get(), n2->id, n1->id)) {
			DEBUG("Nodes %u <-> %u have contact!\n", n1->id, n2->id);
			return 1;
		}	      
		return 0;
	}
	d = v2_distance(n1->pos, n2->pos);
	
	if (d > n1->radio_r) {
		return 0;
	}
	DEBUG("Nodes %u <-> %u have contact, d=%f m\n", 
	      n1->id, n2->id, d);
	return 1;
} 

int node_xmit(struct node *n, struct packet *p)
{
	struct event *e;
	
	if (!p || !n)
		return -1;

	/* TODO: Calculate queueing delay, etc. */
	e = event_new(n, simtime_get() + 0.10, packet_phy_xmit, p); 
	
	if (!e) {
		fprintf(stderr, "node_xmit: Could not create event\n");
		return -1;
	}
			
	if (simulator_schedule(e) < 0) {
		packet_free(p);
		return -1;
	}
	return 1;
}

int node_fwd(struct node *n, struct packet *p)
{
	if (!p || !n)
		return -1;
	
	p->ttl--;

	if (p->ttl <= 0) {
		DEBUG("Node %u, dropping packet %u -> %u TTL=%u\n", n->id, p->src, p->dst, p->ttl);
		TRACE("d %u : %3u %3u [ %u %u ]\n", n->id, p->src, p->dst, p->virtual_len, p->ttl);
		packet_free(p);
		return -1;
	}
	p->prev_hop = n->id;

	return node_xmit(n, p);
}

void node_packet_buffer_print(struct node *n)
{
	list_t *curr;

	DEBUG("Packet buffer for node %u:\n", n->id);

	list_for_each(curr, &n->packet_buffer) {
		struct packet *p;

		p = (struct packet *)curr;
		
		DEBUG("%lu src=%u dst=%u len=%u\n", 
		      p->id, p->src, p->dst, p->virtual_len);
	}

}
int node_buffer_packet(struct node *n, struct packet *p)
{
	if (!n || !p)
		return -1;

	if (!list_unattached(&p->l)) {
		fprintf(stderr, "Packet id=%u already buffered...\n", p->id);
		return -1;
	}
	if (n->buffer_len == MAX_PACKET_BUFFER_LEN) {
		struct packet *first_p;

		first_p = (struct packet *)list_first(&n->packet_buffer);

		n->buffer_len--;

		list_detach(&first_p->l);

		TRACE("d %u : %3u %3u [ %d ]\n", n->id, p->src, p->dst, p->virtual_len);
		packet_free(first_p);
	} 
	
	DEBUG("Node %u buffered packet id=%lu\n", n->id, p->id);

	n->buffer_len++;
	
	return list_add_tail(&p->l, &n->packet_buffer);
}

int node_unbuffer_packet(struct node *n, struct packet *p)
{
	if (!n || !p)
		return -1;

	n->buffer_len--;

	return list_detach(&p->l);
}

int node_packet_buffer_flush(struct node *n)
{
	int num = 0;

	while(!list_empty(&n->packet_buffer)) {
		struct packet *p = (struct packet *)list_first(&n->packet_buffer);
		list_detach(&p->l);
		
		packet_free(p);
		num++;
	}
	return num;
}

int node_send_buffered_packets(struct node *n, unsigned int dst)
{
	list_t *curr, *tmp;
	int sent = 0;
	
	if (!n)
		return -1;

	list_for_each_safe(curr, tmp, &n->packet_buffer) {
		struct packet *p = (struct packet *)curr;

		if (p->dst == dst && node_unbuffer_packet(n, p)) {
			
			if (node_unbuffer_packet(n, p) &&
			    node_xmit(n, p) < 0) {
				sent++;				
			} else {
				DEBUG("Failed to send buffered packet to %u at node %u\n", p->dst, n->id);
			}
		}
	}
	return sent;
}

int node_recv(struct node *n, struct packet *p)
{
	if (!n || !p)
		return -1;
		
	DEBUG("Node %u received packet from %u TTL=%d\n",
	      n->id, p->src, p->ttl);

	p->ttl--;
	
	if (agent_recv(n, p) < 0) {
		fprintf(stderr, "Agent receive error\n");
		return -1;
	}
	return 0;
}

#ifdef MAIN_DEFINED

int main(int argc, char **argv) 
{
	struct node *n1, *n2;
	point2_t p;
	int res;
	struct timeval now;

	n1 = node_new(0);
	n2 = node_new(1);
	
	p[0] = 100;
	p[1] = 100;
	
	node_set_pos(n1, p);
	
	printf("nod pos = (%f, %f)\n", n1->pos[0], n1->pos[1]);

	p[0] = 100.0;
	p[1] = 200.0;
       
	
	node_set_dest_and_speed(n1, p, 10.0);
	
	res = node_update_pos(n1, 1.0);

	printf("nod pos = (%f, %f)\n", n1->pos[0], n1->pos[1]);

	gettimeofday(&now, NULL);
	
	printf("sec=%ld usec=%ld\n", now.tv_sec, now.tv_usec);

	timeval_add_simtime(&now, 100.1);

	printf("sec=%ld usec=%ld\n", now.tv_sec, now.tv_usec);

	return res;
}

#endif
