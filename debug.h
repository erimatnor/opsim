#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <stdarg.h>

#include "simtime.h"

#ifdef DEBUG
#undef DEBUG
#define DEBUG(f, args...) simlog(__FUNCTION__, f, ## args)
#else
#define DEBUG(f, args...)
#endif

static inline int simlog(const char *func, const char *fmt, ...)
{
	static char buf[1024];
	va_list args;
	int r, len = 0;
	
	len = snprintf(buf, 1024, "%f %s: ", simtime_get(), func);
	va_start(args, fmt);
	r = vsnprintf(buf + len, 1024 - len, fmt, args);
	va_end(args);

	printf("%s", buf);

	return r;
}



#endif
