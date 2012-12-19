#include <stdlib.h>

#include "event.h"
#include "simulator.h"
#include "debug.h"


int event_init(struct event *e, struct node *node, simtime_t timeout,
	       event_callback_t callback, void *data) {
	if (!e)
		return -1;
	
	e->time = timeout;
	e->callback = callback;
	e->node = node;
	e->data = data;
	e->is_scheduled = 0;
	
	return 0;
}

struct event *event_new(struct node *node, simtime_t timeout, 
			event_callback_t callback, void *data)
{
	struct event *e;

	e = (struct event *)malloc(sizeof(struct event));

	if (!e)
		return NULL;
	
	if (event_init(e, node, timeout, callback, data) < 0) {
		fprintf(stderr, "Event_init() failed\n");
		return NULL;
	}

	return e;	
}

void event_free(struct event *e)
{
	free(e);
}
