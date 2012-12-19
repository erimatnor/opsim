#ifndef _TRACE_H
#define _TRACE_H

#define TRACE(f, args...) do_trace(f, ## args)

int trace_init(char *trace_file);
void trace_close(void);
int do_trace(const char *fmt, ...);

#endif
