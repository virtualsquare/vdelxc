#ifndef _LOG_H
#define _LOG_H
#include <syslog.h>

void startlog(char *prog, int use_syslog, int verbose);
void printlog(int priority, const char *format, ...);

#endif
