#ifndef _SIMTIME_H
#define _SIMTIME_H

#include <sys/time.h>
#include <time.h>

typedef double simtime_t;

static inline int timeval_add_simtime(struct timeval *tv, simtime_t t)
{
        long add;
 
        if (!tv)
                return -1;

	add = tv->tv_usec + (long)((simtime_t)(t - (long)t) * 1000000);
	
        tv->tv_sec += (long)t + (add / 1000000);
        tv->tv_usec = add % 1000000;

        return 0;
}


void simtime_set(simtime_t t);
simtime_t simtime_get(void);

#endif
