#include <string.h>
#include <stdio.h>

#include "packet.h"
#include "event.h"
#include "simulator.h"
#include "simtime.h"
#include "node.h"
#include "debug.h"
#include "trace.h"

static unsigned long packet_id = 0;

struct packet *packet_new(int size)
{
	struct packet *p;

	p = (struct packet *)malloc(sizeof(struct packet) + size);

	if (!p)
		return NULL;
	
	memset(p, 0, sizeof(struct packet) + size);

	/* For now, set the virtual len to the real len */
	p->len = p->virtual_len = size;

	INIT_LIST_ELM(&p->l);

	p->id = packet_id++;

	return p;
}
struct packet *packet_clone(struct packet *p)
{
	struct packet *p_clone;

	if (!p)
		return NULL;
		 
	p_clone = (struct packet *)malloc(sizeof(struct packet) + p->len);

	if (!p_clone)
		return NULL;

	memcpy(p_clone, p, sizeof(struct packet) + p->len);

	INIT_LIST_ELM(&p_clone->l);

	p_clone->id = packet_id++;

	return p_clone;
}

void packet_free(struct packet *p) 
{
	DEBUG("Freeing packet id=%lu\n", p->id);
	free(p);
}

void packet_phy_xmit(struct event *e)
{
	struct packet *p;
	struct node *dnode, *snode;
	float delay;

	if (!e || !e->data || !e->node)
		goto out;
	
	
	p = (struct packet *)e->data;
	
	DEBUG("Xmit packet src=%u dst=%u\n", p->src, p->dst);
	
	snode = e->node;
	
	if (!snode) {		
		fprintf(stderr, "Xmit failed, no nodes for src = %u\n", p->src);
		packet_free(p);
		goto out;
	}

	if (p->dst == 0xFFFFFFFF)
		TRACE("s %u : %3u   %s [ %d ]\n", snode->id, p->src, "B", p->virtual_len);
	else
		TRACE("s %u : %3u %3u [ %d ]\n", snode->id, p->src, p->dst, p->virtual_len);

	/* Set previous hop */
	p->prev_hop = e->node->id;

	if (p->dst == BROADCAST) {
		int i;

		/* Call recv on all nodes in contact */
		for (i = 0; i < sim.num_nodes; i++) {
			dnode = simulator_get_node(i);
			if (dnode->id != snode->id && node_contact(snode, dnode)) {
				struct event *e_new;
				struct packet *p_clone;
				
				
				DEBUG("Xmit phy for node %u\n", snode->id);
				
				p_clone = packet_clone(p);

				if (!p_clone) {
					packet_free(p);
					goto out;
				}
				
				/* Transmission delay + propagation
				 * delay + random scheduling and
				 * queueing delay */
				delay = (p->len * 8) / (snode->bandwidth * 1000000) + (node_distance(snode, dnode) / SPEED_OF_LIGHT) + ((float)rand() / (RAND_MAX) / 10);
				
				DEBUG("delay %u->%u=%f s\n", snode->id, dnode->id, delay);
				e_new = event_new(dnode, simtime_get() + delay, packet_phy_recv, p_clone); 
				if (!e_new) {
					fprintf(stderr, "packet_phy_xmit: Could not create event\n");
					packet_free(p_clone);
					packet_free(p);
					goto out;
				}

				if (simulator_schedule(e_new) < 0) {
					event_free(e_new);
					packet_free(p_clone);
					packet_free(p);
					goto out;
				}
			}
		}
		packet_free(p);
	} else {
		/* Unicast */
		dnode = simulator_get_node(p->dst);		
		
		if (node_contact(snode, dnode)) {
			struct event *e_new;
			
			delay = (p->len * 8) / (snode->bandwidth * 1000000) + (node_distance(snode, dnode) / SPEED_OF_LIGHT) +((float)rand() / (RAND_MAX) / 10);	
	
			e_new = event_new(dnode, simtime_get() + delay, packet_phy_recv, p); 
			if (!e_new) {
				fprintf(stderr, "packet_phy_xmit: Could not create event\n");
				packet_free(p);
				goto out;
			}

			if (simulator_schedule(e_new) < 0) {
				event_free(e_new);
				packet_free(p);
				goto out;
			}
		} else {
			packet_free(p);
		}
	}
out:
	event_free(e);
	
	return;
}

void packet_phy_recv(struct event *e)
{
	struct packet *p;
	struct node *snode;
	
	if (!e)
		return;

	DEBUG("node %u\n", e->node->id);

	p = (struct packet *)e->data;
	
	if (!p)
		goto out;
	
	if (!e->node) {
		packet_free(p);
		goto out;
	}
	
	snode = simulator_get_node(p->src);

	if (!snode) {
		packet_free(p);
		goto out;
	}	
	if (!node_contact(snode, e->node)) {
		DEBUG("Transmission failed for nodes %u -> %u: OUT of RANGE\n", 
		      snode->id, e->node->id);
		packet_free(p);
		goto out;		
	}

	if (p->dst == 0xFFFFFFFF)
		TRACE("r %u : %3u   %s [ %d ]\n", e->node->id, p->src, "B", p->virtual_len);
	else
		TRACE("r %u : %3u %3u [ %d ]\n", e->node->id, p->src, p->dst, p->virtual_len);
	if (node_recv(e->node, p) < 0) {
		fprintf(stderr, "node receive error\n");
		packet_free(p);		
	}
out:
	event_free(e);

	return;
}
