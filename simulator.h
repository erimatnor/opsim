#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include "event.h"
#include "list.h"
#include "node.h"

struct simulator {
	unsigned int num_events;
	list_t eventQ;
	unsigned int num_nodes;
	struct node *node_arr;
};

struct simulator sim; /* Our simulator */

#define NODE(id) (&sim.node_arr[id])

int simulator_init(void);
void simulator_fini(void);
int simulator_run(void);
int simulator_populate(unsigned int num_nodes);
void simulator_set_endtime(simtime_t t);
struct node *simulator_get_node(unsigned int id);
int simulator_schedule(struct event *new_e);
int simulator_reschedule(struct event *e, simtime_t timeout);
int simulator_unschedule(struct event *e);

#endif
