#ifndef _NODE_H
#define _NODE_H

#include <sys/time.h>
#include <time.h>

#include "point.h"
#include "vector.h"
#include "list.h"
#include "simtime.h"
#include "event.h"
#include "agent.h"

typedef unsigned int nodeid_t;

struct node {
	list_t l;
	int state;
	int type;
	nodeid_t id;
	point2_t pos;
	point2_t dest;
	vector2_t mov_v; /* heading and speed in m/s */
	simtime_t last_time;
	float radio_r; /* Radio range in meters */
	float bandwidth;
	list_t neigh_list;
	list_t packet_buffer;
	unsigned int buffer_len;
	struct agent *agents[MAX_AGENTS];
};

#define RADIO_RANGE 100
#define BANDWIDTH 11 /* Mbit/s */
#define SPEED_OF_LIGHT 299792458 /* m/s */
 
#define NODE_STATE_DEF 0
#define NODE_STATE_MOVING 1
#define NODE_STATE_STATIONARY 2

#define NODE_TYPE_DEF 0

struct packet;

struct node *node_new(nodeid_t id);
int node_init(struct node *n, nodeid_t id);
int node_fini(struct node *n);

void node_free(struct node *n);
int node_update_pos(struct node *n, simtime_t now);
int node_contact(struct node *n1, struct node *n2);
int node_send_hello(struct node *n);
void node_set_pos(struct node *n, point2_t p);
float node_distance(struct node *n1, struct node *n2);

int node_hello_start(struct node *n);
int node_set_dest_and_speed(struct node *n, point2_t dest, float speed);
int node_xmit(struct node *n, struct packet *p);
int node_fwd(struct node *n, struct packet *p);
int node_recv(struct node *n, struct packet *p);
int node_buffer_packet(struct node *n, struct packet *p);
int node_unbuffer_packet(struct node *n, struct packet *p);
void node_packet_buffer_print(struct node *n);

#endif /* _NODE_H */

