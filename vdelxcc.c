/*
 * vdelxc: vde client for linux containers
 * Copyright (C) 2023  Renzo Davoli, Virtualsquare University of Bologna
 *
 * vxdlxc is free software; you can redistribute it and/or
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
#include <syslog.h>
#include <ctype.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include <fdprintf.h>
#include <passfd.h>

#define VDELXC_DEFAULT_PATH "/vde:/var/vde"
#define VDELXC_DEFAULT_SOCKNAME "vdelxcs"

void panic(char *msg) {
	perror(msg);
  exit(1);
}

int opentap(char *ifname) {
  struct ifreq ifr = {
    .ifr_flags = IFF_TAP | IFF_NO_PI
  };
  int fddata=-1;
  if((fddata = open("/dev/net/tun", O_RDWR)) < 0)
		return -1;
  strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
  if(ioctl(fddata, TUNSETIFF, (void *) &ifr) < 0)
		return -1;
  return fddata;
}

static int vdelxc_connect(int fd, char *path_name) {
	if (strchr(path_name,'/')) {  // it is a path
		struct sockaddr_un sun = {AF_UNIX, ""};
		snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", path_name);
		return connect(fd, (void *) &sun, sizeof(sun));
	} else {
		char *path = getenv("VDELXC_PATH");
		if (path == NULL) path = VDELXC_DEFAULT_PATH;
		size_t pathlen = strlen(path) + 1;
		char pathcopy[pathlen];
		snprintf(pathcopy, pathlen, "%s", path);
		char *item;
		for (char *pathscan = pathcopy;
				(item = strtok(pathscan, ":")) != NULL;
				pathscan = NULL) {
			struct sockaddr_un sun = {AF_UNIX, ""};
			snprintf(sun.sun_path, sizeof(sun.sun_path), "%s/%s", item, path_name);
			if (connect(fd, (void *) &sun, sizeof(sun)) == 0)
				return 0;
		}
		return -1;
	}
}

int vdelxc(char *path_name, char *ifname, char *vnl) {
	size_t ifnamelen = strlen(ifname);
	if (ifnamelen == 0)
		return errno = EFAULT, -1;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    panic("socket");
	if (vdelxc_connect(fd, path_name) < 0)
    panic("connect");

	int tapfd;

	if (isdigit(ifname[ifnamelen - 1]))
		tapfd = opentap(ifname);
	else {
		size_t ifnamelen_num = ifnamelen + 4;
		char ifname_num[ifnamelen_num];
		for (int n = 0; n < 256; n++) {
			snprintf(ifname_num, ifnamelen_num, "%s%d", ifname, n);
			tapfd = opentap(ifname_num);
			if (tapfd >= 0 || errno != EBUSY)
				break;
		}
	}

	if (tapfd < 0)
		panic("interface definition");

  write_fd(fd, vnl, strlen(vnl) + 1, tapfd);

  char buf[10];
  recv(fd, buf, 10, 0);
	int err = strtol(buf, NULL, 0);
	if (err != 0) {
		errno = err;
		panic("vdelxcd");
	}
  return 0;
}

/* Main and command line args management */
void usage(char *progname)
{
	fprintf(stderr,"Usage: %s [ OPTIONS ] [ VNL ]\n"
			"\toptions;\n"
			"\t  -s, --socket <socket>  use this unix socket\n"
			"\t  -i, --iface <socket>   interface name or prefix (default \"vde\")\n"
			"\t  -h, --help\n"
			"\tVNL: virtual network locator (default value defined by vdelxcd)\n",
			progname);
	exit(1);
}

static char short_options[] = "hs:i:";
static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"socket", 1, 0, 's'},
	{"iface", 1, 0, 'i'},
	{0,0,0,0}
};

static char arg_tags[] = "si";
static union {
	struct {
		char *socket;
		char *iface;
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
	char *vnl = "";
	while(1) {
		int c;
		if ((c = getopt_long (argc, argv, short_options,
						long_options, &option_index)) < 0)
			break;
		switch (c) {
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

	if (args.socket == NULL) args.socket = VDELXC_DEFAULT_SOCKNAME;
	if (args.iface == NULL) args.iface = "vde";

	if (argc != optind)
		vnl = argv[optind++];
	
	if (argc != optind)
		usage(progname);

	return vdelxc(args.socket, args.iface, vnl);
}

