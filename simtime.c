#include "simtime.h"

static simtime_t simulator_time = 0.0;

void simtime_set(simtime_t t)
{
	simulator_time = t;
}	

simtime_t simtime_get(void)
{
	return simulator_time;
}
