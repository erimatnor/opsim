#include <stdio.h>
#include <string.h>

#include "neigh.h"
#include "node.h"
#include "event.h"
#include "simulator.h"
#include "epidemic.h"

#define NEIGHBOR_TIMEOUT 3000

void neigh_timeout(struct event *e);

int neigh_add(struct node *n, unsigned int id) 
{
	struct neighbor *neigh;

	if (neigh_find(n, id)) {
		fprintf(stderr, "Node %u: %u already neighbor!\n", n->id, id);
		return -1;
	}

	neigh = (struct neighbor *)malloc(sizeof(struct neighbor));
	
	if (!neigh)
		return -1;

	memset(neigh, 0, sizeof(struct neighbor));

	neigh->id = id;

	event_init(&neigh->timeout, n, simtime_get() + NEIGHBOR_TIMEOUT, neigh_timeout, neigh);
	simulator_schedule(&neigh->timeout);
	
	list_add_tail(&neigh->l, &n->neigh_list);

	if (n->agents[PROTOCOL_EPIDEMIC]) {
		epidemic_infect_neigh(n, id);
	}
	return 1;
}


int neigh_del(struct node *n, unsigned int id) 
{
	list_t *curr;
	
	list_for_each(curr, &n->neigh_list) {
		struct neighbor *neigh = (struct neighbor *)curr;

		if (neigh->id == id) {
			list_detach(&neigh->l);
			simulator_unschedule(&neigh->timeout);
			free(neigh);
			return 1;
		}
	}
	return 0;
}

int neigh_find(struct node *n, unsigned int id) 
{
	list_t *curr;
	
	list_for_each(curr, &n->neigh_list) {
		struct neighbor *neigh = (struct neighbor *)curr;

		if (neigh->id == id)
			return 1;
	}
	return 0;	
}

int neigh_refresh(struct node *n, unsigned int id) 
{
	list_t *curr;
	
	list_for_each(curr, &n->neigh_list) {
		struct neighbor *neigh = (struct neighbor *)curr;

		if (neigh->id == id) {
			simulator_unschedule(&neigh->timeout);
			neigh->timeout.time = simtime_get() + NEIGHBOR_TIMEOUT;
			simulator_schedule(&neigh->timeout);

		/* 	if (n->agents[PROTOCOL_EPIDEMIC]) { */
/* 				epidemic_infect_neigh(n, id); */
/* 			} */
			return 1;
		}
	}
	return 0;
}

void neigh_timeout(struct event *e)
{
	struct neighbor *neigh = (struct neighbor *)e->data;

	list_detach(&neigh->l);

	free(neigh);
}

void neigh_list_flush(struct node *n)
{
	while(!list_empty(&n->neigh_list)) {
		struct neighbor *neigh = (struct neighbor *)list_first(&n->neigh_list);
		list_detach(&neigh->l);
		simulator_unschedule(&neigh->timeout);
		free(neigh);		
	}
}
