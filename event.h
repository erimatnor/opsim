#ifndef _EVENT_H
#define _EVENT_H

#include <sys/time.h>
#include <time.h>

#include "list.h"
#include "simtime.h"
#include "node.h"

struct event;

typedef void (*event_callback_t) (struct event *);

struct event {
	list_t l;
	int is_scheduled;
//	unsigned int type;
	simtime_t time;
	event_callback_t callback;
	struct node *node; /* The node related to the event */
//	char *message;
	void *data;
};


#define EVENT_PACKET_XMIT   1
#define EVENT_MOBILITY 2

int event_init(struct event *e, struct node *node, simtime_t timeout,
	       event_callback_t callback, void *data);
struct event *event_new(struct node *node, simtime_t timeout, 
			event_callback_t callback, void *data);
void event_free(struct event *e);
//int event_refresh(struct event *e, simtime_t timeout);

#endif /*_EVENT_H */
