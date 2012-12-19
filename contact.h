#ifndef _CONTACT_H
#define _CONTACT_H

#include "simtime.h"
#include "list.h"

struct contact {
	list_t l;
	unsigned int id1, id2;
	simtime_t start_time, end_time;
};

int contact_check(simtime_t t, unsigned int id1, unsigned int id2);
int contact_add(unsigned int id1, unsigned int id2, 
		simtime_t start_time, simtime_t end_time);
#endif
