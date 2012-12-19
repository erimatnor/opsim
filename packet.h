#ifndef _PACKET_H
#define _PACKET_H

//#include "event.h"
#include "list.h"

struct event;

struct packet {
	list_t l;
	unsigned int protocol; /* Demultiplex on this */
	unsigned int src;
	unsigned int dst;
	unsigned int prev_hop;
	unsigned int id;
	int ttl;
	int virtual_len;
	int len;
	char data[0];
};

typedef struct packet packet_t;

#define BROADCAST 0xFFFFFFFF

void packet_phy_xmit(struct event *e);
void packet_phy_recv(struct event *e);
struct packet *packet_new(int size);
void packet_free(struct packet *p) ;
struct packet *packet_clone(struct packet *p);

#endif
