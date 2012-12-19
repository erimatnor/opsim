#include <stdlib.h>
#include <stdio.h>

#include "contact.h"
#include "node.h"
#include "list.h"
#include "simtime.h"

LIST(contact_list);

int contact_check(simtime_t t, unsigned int id1, unsigned int id2)
{
	list_t *curr, *tmp;

	list_for_each_safe(curr, tmp, &contact_list) {
		struct contact *c = (struct contact *)curr;
		
		if (c->start_time > t)
			break;

		if (c->end_time < t) {
			list_detach(&c->l);
			free(c);
			continue;
		} 

		if (c->id1 == id1 && c->id2 == id2 &&
		    c->start_time < t && c->end_time > t) 
			return 1;
	}
	return 0;
}

int contact_add(unsigned int id1, unsigned int id2, 
		simtime_t start_time, simtime_t end_time)
{
	list_t *curr;

	if (end_time <= start_time || id1 == id2) {
		fprintf(stderr, "contact_add error, bad times?\n");
		return -1;
	}

	struct contact *c = (struct contact *)malloc(sizeof(struct contact));

	if (!c)
		return -1;
	
	c->id1 = id1;
	c->id2 = id2;
	c->start_time = start_time;
	c->end_time = end_time;
	
	list_for_each(curr, &contact_list) {
		struct contact *lc = (struct contact *)curr;

		if (c->start_time < lc->start_time)
			break;
	}

	listelm_add(&c->l, curr->prev, curr);

	return 1;
}
