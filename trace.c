#include <stdio.h>
#include <stdarg.h>

#include "trace.h"
#include "simtime.h"

#define DEFAULT_TRACE_FILE "sim-output.tr"

FILE *trace_f;


int trace_init(char *trace_file)
{

	if (!trace_file)
		trace_file = DEFAULT_TRACE_FILE;

	trace_f = fopen(trace_file, "w");

	if (trace_f == NULL) {
		fprintf(stderr, "Could not open trace file: %s\n", trace_file);
		return -1;		
	}
	return 0;
}

void trace_close(void)
{
	fclose(trace_f);
}

int do_trace(const char *fmt, ...)
{
	va_list args;
	int r;
	
	fprintf(trace_f, "%f ", simtime_get());
	va_start(args, fmt);
	r = vfprintf(trace_f, fmt, args);
	va_end(args);
	
	return r;
}

