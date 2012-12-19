#ifndef _MOBILITY_H
#define _MOBILITY_H

#include "point.h"
#include "event.h"

struct mobility {
	unsigned int id;
	point2_t dst;
	float speed;
};

void mobility_event(struct event *e);
struct mobility *mobility_new(unsigned int id, point2_t dst, float speed);

#endif /* _MOBILITY_H */
