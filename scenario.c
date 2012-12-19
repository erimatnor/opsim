#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "simtime.h"
#include "simulator.h"
#include "point.h"
#include "debug.h"
#include "mobility.h"
#include "node.h"
#include "traffic.h"
#include "contact.h"

#define BUFSIZE 512

extern int contact_mode;

int scenario_read_from_file(char *filename)
{
	FILE *f;
	int n = 0;

	if (filename) {
		f = fopen(filename, "r");

		if (!f) {
			fprintf(stderr, "Could not open scenario file \"%s\": ", 
				filename);
			perror("");
			
			return -1;
		}
		
	} else {
		DEBUG("Reading scenario from STDIN\n");
		f = stdin;
	}
	
	while(1) {
		struct mobility *m;
		char buf[BUFSIZE];
		char *line;
		int res;
		simtime_t time = 0.0;
		unsigned int num_nodes;
		struct event *e;
		struct node *node;

		line = fgets(buf, BUFSIZE, f);
		
		if (line == NULL) {
			return n;
		}
		if (buf[0] == '#') {
			continue;
		}

		if (strncmp(line, "num_nodes=", 10) == 0) {

			res = sscanf(buf, "num_nodes=%u", &num_nodes);

			if (res == EOF || res < 1) {
				DEBUG("Could not read number of nodes\n");
			}
			simulator_populate(num_nodes);
			continue;
		}
		if (strncmp(line, "end_time=", 9) == 0) {

			res = sscanf(buf, "end_time=%lf", &time);

			if (res == EOF || res < 1) {
				DEBUG("Could not read end time\n");
			}
			simulator_set_endtime(time);
			continue;
		}
		
		/* Mobility */
		if (line[0] == 'm') {
			point2_t pos;
			float speed;
			unsigned int id;
			char type;
			
			res = sscanf(buf, "%c %lf %d %f %f %f\n", &type, &time, &id, pos, pos + 1, &speed);
			if (res < 6) {
				fprintf(stderr, "Matching error: %s", line);
			}
			if (res == EOF) 
				return n;
			
			m = mobility_new(id, pos, speed);
			
			if (!m) {
				fprintf(stderr, "could not create mobility\n");
				return n;
			}
			
			DEBUG("m=%u\n", (unsigned int)m);
			
			node = simulator_get_node(id);
			
			if (!node) {
				fprintf(stderr, "No node with id=%u\n", id);
				free(m);			
				exit(-1);
			}
			
			e = event_new(node, time, mobility_event, m);
			
			if (!e) {
				fprintf(stderr, "could not add mobility event\n");
				free(m);
				return n;
			}
			
			if (simulator_schedule(e) < 0) {
				event_free(e);
				free(m);
			}
			
			DEBUG("time = %lf id=%u pos=(%f, %f) speed=%f\n", time, id, pos[0], pos[1], speed);
		}
		
		if (line[0] == 't') {
			char type;
			char traf_type;
			unsigned int src, dst;
			unsigned int size;
			struct traffic *t;
			struct node *node;
			
			res = sscanf(buf, "%c %lf %c %u %u %u\n", &type, &time, &traf_type, &src, &dst, &size);
			if (res < 5) {
				fprintf(stderr, "Matching error: %s", line);
			}
			if (res == EOF) 
				return n;
			
			DEBUG("Send traffic src=%u dst=%u size=%u\n", src, dst, size);			
			node = simulator_get_node(src);
			
			if (!node) {
				fprintf(stderr, "No node with id=%u\n", src);
				exit(-1);
			}
			
			t = traffic_new(traf_type, src, dst, size);

			if (!t) 
				return n;

			e = event_new(node, time, traffic_event, t);

			if (!e) {
				fprintf(stderr, "could not add traffic event\n");
				free(t);
				return n;
			}
			
			if (simulator_schedule(e) < 0) {
				event_free(e);
				free(t);
			}			
				
		}
		/* Set initial position */
		if (line[0] == 'p') {
			char type;
			unsigned int id;
			point2_t pos;
			struct node *node;
			
			res = sscanf(buf, "%c %u %f %f\n", &type, &id , pos, pos + 1);
			if (res < 4) {
				fprintf(stderr, "Matching error: %s", line);
			}
			if (res == EOF) 
				return n;
		
			DEBUG("Set pos id=%u pos=(%f, %f)\n", id, pos[0], pos[1]);			
			node = simulator_get_node(id);

			if (node)
				node_set_pos(node, pos);
		}
		if (line[0] == 'c') {
			char type;
			unsigned int id1, id2;
			simtime_t start_time, end_time;
			
			contact_mode = 1;

			res = sscanf(buf, "%c %u %u %lf %lf\n", &type, &id1 , &id2, &start_time, &end_time);
			if (res < 5) {
				fprintf(stderr, "Matching error: %s", line);
			}
			if (res == EOF) 
				return n;
			
			contact_add(id1, id2, start_time, end_time);
		}
		n++;	
	}
	return n;
}
