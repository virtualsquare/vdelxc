/*
 * vdelxc: vde support for linux containers: log management
 * Copyright (C) 2023  Renzo Davoli, Virtualsquare University of Bologna
 *
 * vxdlxcd is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

#include <log.h>

#define FLAG_VERBOSE 1
static int logok;
static int flags;
static char *progname;

void startlog(char *prog, int use_syslog, int verbose) {
  progname = prog;
  if (use_syslog) {
    openlog(progname, LOG_PID, 0);
    printlog(LOG_INFO, "%s started", progname);
    logok=1;
  }
	if (verbose) flags |= FLAG_VERBOSE;
}

void printlog(int priority, const char *format, ...) {
  va_list arg;

  va_start (arg, format);

  if (logok)
    vsyslog(priority, format, arg);
  else {
		if (priority <= LOG_NOTICE || (flags & FLAG_VERBOSE)) {
			//fprintf(stderr, "%s: ", progname);
			vfprintf(stderr, format, arg);
			fprintf(stderr, "\n");
		}
  }
  va_end (arg);
}
