#ifndef _NEIGH_H
#define _NEIGH_H

#include "list.h"
#include "node.h"
#include "event.h"

struct neighbor {
	list_t l;
	unsigned int id;
	struct event timeout;
};

int neigh_add(struct node *n, unsigned int id);
int neigh_del(struct node *n, unsigned int id);
int neigh_find(struct node *n, unsigned int id);
int neigh_refresh(struct node *n, unsigned int id);
void neigh_list_flush(struct node *n);

#endif /* _NEIGH_H */
