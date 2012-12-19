#include <string.h>

#include "debug.h"
#include "mobility.h"
#include "event.h"
#include "point.h"
#include "node.h"
#include "simulator.h"

struct mobility *mobility_new(unsigned int id, point2_t dst, float speed)
{
	struct mobility *m;

	m = (struct mobility *)malloc(sizeof(struct mobility));

	if (!m)
		return NULL;

	m->id = id;
	memcpy(m->dst, dst, sizeof(point2_t));;
	m->speed = speed;

	return m;
}

void mobility_event(struct event *e)
{
	struct mobility *m = (struct mobility *)e->data;
	
	if (!e->node)
		goto out;

	node_set_dest_and_speed(e->node, m->dst, m->speed);
out:
	free(m);
	event_free(e);
}
