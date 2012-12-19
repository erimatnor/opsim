#ifndef _TRAFFIC_H
#define _TRAFFIC_H

#include "event.h"

struct traffic {
	unsigned char type;
	unsigned int src;
	unsigned int dst;
	unsigned int len;
	char data[0];
};

#define TRAFFIC_EPIDEMIC 0

struct traffic *traffic_new(char type, unsigned int src, unsigned int dst, unsigned int len);
void traffic_event(struct event *e);

#endif /*_TRAFFIC_H */
