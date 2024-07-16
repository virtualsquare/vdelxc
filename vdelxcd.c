/*
 * vdelxcd: vde daemon for linux containers
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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <libvdeplug.h>
#include <fdprintf.h>
#include <log.h>
#include <passfd.h>
#include <vdeauth.h>

#define ETH_ALEN 6
#define ETH_HDRLEN (ETH_ALEN+ETH_ALEN+2)

static int fdcwd = -1;
static pid_t mypid;
struct sockaddr_un socket_sa = {AF_UNIX, ""};
static mode_t mode = 0;

void panic(char *msg) {
	printlog(LOG_CRIT, "%s: %s", msg, strerror(errno));
	exit(1);
}

void vdethread(int connfd) {
	char vnl[PATH_MAX];
	int tapfd = -1;
	int n = read_fd(connfd, vnl, PATH_MAX, &tapfd);
	if (n < 0 || tapfd < 0)
		goto err;
	char *safevnl = vdeauth_check(vnl);
	if (safevnl == NULL) {
		printlog(LOG_WARNING, "open interface permission denied: %s", vnl);
		errno = EACCES;
		goto err2;
	}
	printlog(LOG_INFO, "open interface: %s", safevnl);
	VDECONN *vdeconn = vde_open(safevnl, "vdelxcd", NULL);
	if (vdeconn == NULL)
		goto err2;
	int datafd = vde_datafd(vdeconn);
	fdprintf(connfd, "%ld", 0);
	close(connfd);
	struct pollfd pfd[] = {{tapfd, POLLIN, 0}, {datafd, POLLIN, 0}};
	for(;;) {
		char *buf[VDE_MAXMTU];
		int n = poll(pfd, 2, -1);
		if (n < 0)
			break;
		if (pfd[0].revents & POLLERR || pfd[0].revents & POLLERR)
      break;
		if (pfd[0].revents & POLLIN) {
			ssize_t len = read(tapfd, buf, VDE_MAXMTU);
			if (len <= 0)
				break;
			vde_send(vdeconn, buf, len, 0);
		}
		if (pfd[1].revents & POLLIN) {
			ssize_t len = vde_recv(vdeconn, buf, VDE_MAXMTU, 0);
			if (len <= 0)
				break;
			if (len >= ETH_HDRLEN) {
				if (write(tapfd, buf, len) != len)
					break;
			} 
		}
	}
	printlog(LOG_INFO, "close interface: %s", safevnl);
	vde_close(vdeconn);
	close(tapfd);
	return;
err2:
	close(tapfd);
err:
	fdprintf(connfd, "%ld", errno);
	close(connfd);
	return;
}

static void ck_unlinkold(void) {
	struct stat st;
	int rv = stat(socket_sa.sun_path, &st);
	if (rv == -1) {
		if (errno == ENOENT)
			return;
		else
			panic("can't create socket");
	}
	if (S_ISSOCK(st.st_mode)) {
		unlink(socket_sa.sun_path);
	} else {
		errno = EEXIST;
		panic("can't create socket");
	}
}

int vdelxcd(void) {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		panic("socket");
	ck_unlinkold();
	if (bind(fd, (void *) &socket_sa, sizeof(socket_sa)) < 0)
		panic("bind");
	if (mode)
		chmod(socket_sa.sun_path, mode);
	if (listen(fd, 10) < 0)
		panic("listen");

	for (;;) {
		int connfd = accept(fd, NULL, NULL);

		switch (fork()) {
			case 0:
				close(fd);
				vdethread(connfd);
				exit(0);
			default:
				close(connfd);
				break;
			case -1:
				panic("fork");
		}

	}
	return 0;
}

/* signal handling */
static void terminate(int signum) {
	pid_t pid = getpid();
	if (pid == mypid) {
		printlog(LOG_INFO, "(%d) leaving on signal %d", pid, signum);
		unlink(socket_sa.sun_path);
	}
	exit(0);
}

static void cont_handler(int signum) {
	switch (signum) {
		case SIGCHLD: wait(NULL); break;
	}
}

static void setsignals(void) {
	struct sigaction action = {
		.sa_handler = terminate
	};
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	struct sigaction cont = {
		.sa_handler = cont_handler,
		.sa_flags = SA_RESTART
	};
	sigaction(SIGHUP, &cont, NULL);
	sigaction(SIGCHLD, &cont, NULL);
}

void save_pidfile(char *pidfile, int fdcwd)
{
	int fd = openat(fdcwd, pidfile,
			O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0)
		panic("save pid file");
	fdprintf(fd, "%ld\n", (long int)mypid);
	close(fd);
}

/* Main and command line args management */
void usage(char *progname)
{
	fprintf(stderr,"Usage: %s [ OPTIONS ] SOCKET [ VNL ] \n"
			"\t-f, --rcfile <conffile>  configuration file\n"
			"\t-d, --daemon             daemon mone\n"
			"\t-p, --pidfile <pidfile>  save daemon's pid\n"
			"\t-v, --verbose            enable verbose mode\n"
			"\t-m, --mode <MODE>        set socket permission bits to MODE\n"
			"\t-h, --help\n",
			progname);
	exit(1);
}

static char short_options[] = "hdvf:p:m:";
static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"rcfile", 1, 0, 'f'},
	{"daemon", 0, 0, 'd'},
	{"pidfile", 1, 0, 'p'},
	{"verbose", 0, 0, 'v'},
	{"mode", 1, 0, 'm'},
	{0,0,0,0}
};

static char arg_tags[] = "dvpm";
static union {
	struct {
		char *daemon;
		char *verbose;
		char *pidfile;
		char *mode;
	};
	char *argv[sizeof(arg_tags)];
} args;

#ifndef _GNU_SOURCE
static inline char *strchrnul(const char *s, int c) {
	while (*s && *s != c)
		s++;
	return (char *) s;
}
#endif

static inline int argindex(char tag) {
	return strchrnul(arg_tags, tag) - arg_tags;
}

int main(int argc, char *argv[]) {
	char *progname = basename(argv[0]);
	int option_index;
	char *default_vnl = NULL;
	char *conffile = NULL;
	while(1) {
		int c;
		if ((c = getopt_long (argc, argv, short_options,
						long_options, &option_index)) < 0)
			break;
		switch (c) {
			case 'f':
				conffile = optarg;
				break;
			case -1:
			case '?':
			case 'h': usage(progname); break;
			default: {
								 int index = argindex(c);
								 if (args.argv[index] == NULL)
									 args.argv[index] = optarg ? optarg : "";
							 }
								break;
		}
	}
	if (args.mode) {
		mode = strtol(args.mode, NULL, 8);
		if (mode == 0)
			usage(progname);
	}

	if (argc == optind)
		usage(progname);

	snprintf(socket_sa.sun_path, sizeof(socket_sa.sun_path), "%s", argv[optind++]);

	if (argc != optind)
		default_vnl = argv[optind++];

	if (argc != optind)
		usage(progname);

	if (conffile && vdeauth_parsercfile(conffile) < 0)
		panic("load rc file");

	if (default_vnl)
		vdeauth_setdefault(default_vnl);

	/* saves current path in cwd, because otherwise with daemon() we
	 * forget it */
	if((fdcwd = open(".", O_PATH)) < 0)
		panic("getcwd");

  setsignals();
  if (args.daemon && daemon(0, 0))
    panic("daemon");

	mypid = getpid();

	if (args.pidfile)
		save_pidfile(args.pidfile, fdcwd);

	startlog(progname, args.daemon != NULL, args.verbose != NULL);

	return vdelxcd();
}
