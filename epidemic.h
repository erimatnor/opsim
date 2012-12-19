#ifndef _EPIDEMIC_H
#define _EPIDEMIC_H

#define PROTOCOL_EPIDEMIC 1

#include "node.h"

struct agent *epidemic_agent_new(struct node *n);
int epidemic_initiate_data(struct node *n, unsigned int dst, unsigned int len);
int epidemic_infect_neigh(struct node *n, unsigned int neigh_id);

#endif /* _EPIDEMIC_H */
