#include <string.h>

#include "list.h"
#include "debug.h"
#include "node.h"
#include "event.h"
#include "trace.h"
#include "scenario.h"
#include "simulator.h"
#include "hello.h"
#include "epidemic.h"

static simtime_t end_time = 200.0; /* End time in seconds */

static char *scenario_file = NULL;
static char *prog_name = NULL;
static char *trace_file = NULL;
static unsigned int random_seed = 1;
int contact_mode = 0;
unsigned int max_packet_buffer_len = 100;
static float percent_done = 0.01;

int simulator_init(void)
{
	sim.num_events = 0;
	INIT_LIST(&sim.eventQ);
	
	return 0;
}

int simulator_agent_init(void)
{
	int i;

	for (i = 0; i < sim.num_nodes; i++) {
		struct agent *a;
		
		a = hello_agent_new(NODE(i));
		
		if (!a) {
			fprintf(stderr, "Agent Error\n");
			exit(-1);
		}
		
		agent_register(NODE(i), a);

		a = epidemic_agent_new(NODE(i));
		
		if (!a) {
			fprintf(stderr, "Agent Error\n");
			exit(-1);
		}

		agent_register(NODE(i), a);		
	}

	return 0;
}
void simulator_set_endtime(simtime_t t)
{
	end_time = t;
}

int simulator_populate(unsigned int num_nodes)
{	
	int i;

	sim.num_nodes = num_nodes;
	
	sim.node_arr = (struct node *)calloc(sim.num_nodes, sizeof(struct node));
	
	if (!sim.node_arr) {
		fprintf(stderr, "Could not allocate nodes\n");
		return -1;
	}
	
	
	for (i = 0; i < sim.num_nodes; i++) {
		if (node_init(&sim.node_arr[i], i) < 0) {
			fprintf(stderr, "Init node failed for node %d\n", i);
			return -1;
		}
		DEBUG("Init node %u\n", i);
	}

	return i;
}

struct node *simulator_get_node(unsigned int id)
{
	if (id > sim.num_nodes)
		return NULL;

	return &sim.node_arr[id];
}

int simulator_schedule(struct event *new_e)
{
	list_t *curr;

	if (!new_e /* || new_e->is_scheduled */)
		return -1;
	
	if (new_e->is_scheduled) {
		DEBUG("Event already scheduled\n");
		return -1;
	}
#ifdef DEBUG_EVENTS
	DEBUG("Scheduling event for t=%lf\n", new_e->time); 
#endif
	if (new_e->time > end_time || new_e->time < simtime_get())
		return -1;

	list_for_each(curr, &sim.eventQ) {
		struct event *e = (struct event *)curr;
		
		if (new_e->time < e->time)
			break;		
	}
	
	new_e->is_scheduled = 1;

	listelm_add(&new_e->l, curr->prev, curr);

	return ++sim.num_events;
}

int simulator_unschedule(struct event *e)
{
	if (!e /* || !e->is_scheduled */)
		return -1;
	
	if (!e->is_scheduled) {
		DEBUG("Event not scheduled\n");
		return -1;
	}

	e->is_scheduled = 0;
	sim.num_events--;

	return list_detach(&e->l);
}

int simulator_reschedule(struct event *e, simtime_t timeout)
{
#ifdef DEBUG_EVENTS
	DEBUG("Scheduling event for t=%lf\n", new_e->time); 
#endif
	if (!e)
		return -1;
	
	if (simulator_unschedule(e) < 0) {
		fprintf(stderr, "Event rescheduling failed\n");
		return -1;
	}
	
	e->time = timeout;

	return simulator_schedule(e);
}

struct event *simulator_next_event(void)
{
	struct event *e;

	if (list_empty(&sim.eventQ))
		return NULL;

	e = (struct event *)sim.eventQ.next;
	
	if (simulator_unschedule(e) < 0) {
		fprintf(stderr, "Could not get next event!\n");
		return NULL;
	}
	
	simtime_set(e->time);
	
	e->is_scheduled = 0;

	return e;
}
int simulator_run(void)
{
	unsigned long int events_processed = 0;
	struct event *e;
       	
	//DEBUG("RUNNING SIMULATION...\n\n");
	printf("RUNNING SIMULATION...\n\n");
	
	printf("0%% done...");
	fflush(stdout);

	while((e = simulator_next_event())) {
		int i;
#ifdef DEBUG_EVENTS
		list_t *curr;
		int n_events = 0;

	
		list_for_each(curr, &sim.eventQ) {
			struct event *e2 = (struct event *)curr;
			DEBUG("Event %d t=%f\n", n_events, e2->time);
			n_events++;
		}
#endif /* DEBUG_EVENTS */
		if (simtime_get() / end_time > percent_done) {
			printf("\r%0.f%% done...", percent_done * 100);
			percent_done += 0.01;
			fflush(stdout);
		} 
			
		/* Update the positions of each node */
		for (i = 0; i < sim.num_nodes; i++) {
			node_update_pos(simulator_get_node(i), e->time);
		}
	
		/* Process event */
		if (e->callback) {
			e->callback(e);
			events_processed++;
#ifdef DEBUG_EVENTS
			DEBUG("Processed event %u\n", events_processed);
#endif /* DEBUG_EVENTS */
		}
	}
	printf("\n");

	DEBUG("Events processed = %u\n", events_processed);

	return events_processed;
}

void simulator_fini(void)
{
	int i;

	for (i = 0; i < sim.num_nodes; i++) {
		node_fini(NODE(i));
	}
	free(sim.node_arr);

	if (i > 0)
		DEBUG("Destroyed %d nodes\n", sim.num_nodes);

	sim.num_nodes = 0;	      
}

void print_usage(void)
{
	printf("Usage: %s [ OPTIONS ] [ SCENARIO ]\n", prog_name);
	printf("where OPTIONS:\n");
	printf("\t-tr, --trace TRACE\t set output trace file. Default \"sim.trace\"\n");
	printf("\t-s, --seed SEED\t set the random seed used in the simulation\n");
	printf("\t-bl, --buffer-len BUFFER_LEN\t set the length of the nodes' packet buffers\n");
	printf("\t-h, --help\t\t print help (this information)\n");
	printf("\n");
	printf("With no SCENARIO or when SCENARIO is \'-\' the file is read from standard input (STDIN).\n");

}

int parse_command_line(int argc, char **argv)
{
	prog_name = argv[0];
	argv++;
	argc--;

	while (argc) {
		if (strcmp("-tr", argv[0]) == 0 ||
			   strcmp("--trace", argv[0]) == 0) {
			if (argv[1] == NULL) {
				print_usage();
				return -1;
			}
			trace_file = argv[1];
			argc--;
			argv++;
			DEBUG("Trace file set to %s\n", trace_file);
		} else if (strcmp("-s", argv[0]) == 0 ||
			   strcmp("--seed", argv[0]) == 0) {
			if (argv[1] == NULL) {
				print_usage();
				return -1;
			}
			random_seed = atoi(argv[1]);

			srandom(random_seed);
			srand(random_seed);
			DEBUG("Random seed set to %u\n", random_seed);
			TRACE("# Random seed=%u\n", random_seed);
			argc--;
			argv++;
		} else if (strcmp("-bl", argv[0]) == 0 ||
			   strcmp("--buffer-len", argv[0]) == 0) {
			if (argv[1] == NULL) {
				print_usage();
				return -1;
			}
			max_packet_buffer_len = atoi(argv[1]);

			DEBUG("Packet buffer len set to %u\n", 
			      max_packet_buffer_len);
			argc--;
			argv++;
		} else if (strcmp("-h", argv[0]) == 0 ||
			   strcmp("--help", argv[0]) == 0) {
			print_usage();
			return -1;
		} else if (argc == 1) {
			if (argv[0] == NULL) {
				print_usage();
				return -1;
			}
			scenario_file = argv[0];
			if (strcmp("-", scenario_file) == 0) {
				scenario_file = NULL;
			} else  {
				DEBUG("Scenario file set to %s\n", 
				      scenario_file);
			}
			return 0;
		} else {
			print_usage();
			return -1;
		} 
		argc--;
		argv++;
	}
	return 0;
}

int main (int argc, char **argv)
{
	if (parse_command_line(argc, argv) < 0) 
		return -1;
	
	simulator_init();
	
	printf("Reading scenario...\n");
	if (scenario_read_from_file(scenario_file) < 0) {
		fprintf(stderr, "Aborting...\n");
		simulator_fini();
		return -1;
	}
		
	trace_init(trace_file);

	simulator_agent_init();

	simulator_run();

	simulator_fini();

	trace_close();


	return 0;
}
