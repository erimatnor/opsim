#include <stdlib.h>

#include "traffic.h"
#include "epidemic.h"

struct traffic *traffic_new(char type, unsigned int src, unsigned int dst, unsigned int len)
{
	struct traffic *t;

	t = (struct traffic *)malloc(sizeof(struct traffic));

	if (!t)
		return NULL;
	
	if (type =='e') 
		t->type = TRAFFIC_EPIDEMIC;
	t->src = src;
	t->dst = dst;
	t->len = len;

	return t;
}

void traffic_event(struct event *e)
{
	struct traffic *t = (struct traffic *)e->data;

	if (!e->node)
		goto out;

	switch(t->type) {
	case TRAFFIC_EPIDEMIC:
		 epidemic_initiate_data(e->node, t->dst, t->len);
		break;
	}
out:
	free(t);
	event_free(e);
}
