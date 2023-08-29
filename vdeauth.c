/*
 * vdelxcd: vde deamon for linux containers: authorization management
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
#include <stdlib.h>
#include <string.h>
#include <strcase.h>
#include <errno.h>
#include <libvdeplug_mod.h>

#include <log.h>

#define AUTH_DEFAULT  0
#define AUTH_ALLOW    11
#define AUTH_DENY     10
#define AUTH_PFXALLOW 21
#define AUTH_PFXDENY  20

struct vdeauth {
	struct vdeauth *next;
	int tag;
	char vnl[];
};

static struct vdeauth *head, *tail;
static int vdeauth_active = 0;

static void vdeauth_add(int tag, char *vnl) {
	size_t elemlen = sizeof(struct vdeauth) + strlen(vnl) + 1;
	struct vdeauth *new = malloc(elemlen);
	if (new) {
		new->tag = tag;
		strcpy(new->vnl, vnl);
		if (tag == AUTH_DEFAULT) {
			new->next = head;
			head = new;
			if (tail == NULL)
				tail = head;
		} else {
			new->next = NULL;
			if (head == NULL)
				head = new;
			else
				tail->next = new;
			tail = new;
		}
	}
}

static int vdeauth_vnl_check(char *vnl) {
	for (struct vdeauth *scan = head; scan != NULL; scan = scan->next) {
		switch(scan->tag) {
			case AUTH_DEFAULT:  // default is a-priori authorized
				if (strcmp(vnl, scan->vnl) == 0)
					return 1;
				break;
			case AUTH_ALLOW:
				if (strcmp(vnl, scan->vnl) == 0)
					return 1;
				break;
			case AUTH_DENY:
				if (strcmp(vnl, scan->vnl) == 0)
					return 0;
				break;
			case AUTH_PFXALLOW:
				if (strncmp(vnl, scan->vnl, strlen(scan->vnl)) == 0)
					return 1;
				break;
			case AUTH_PFXDENY:
				if (strncmp(vnl, scan->vnl, strlen(scan->vnl)) == 0)
					return 0;
				break;
			default:
				return 0;
		}
	}
	return 0;
}

char *vdeauth_check(char *vnl) {
	if (vnl == NULL)
		return NULL;
	if (vdeauth_active == 0)
		return vnl;
	if (*vnl == '\0' && 
			head != NULL && head->tag == AUTH_DEFAULT)
		return head->vnl;
	size_t vnllen = strlen(vnl) + 1;
	char _vnl[vnllen];
	snprintf(_vnl, vnllen, "%s", vnl);
	for (char *pos = _vnl; pos != NULL;) {
		char *newpos = vde_parsenestparms(pos);
		if (vdeauth_vnl_check(pos) == 0)
			return NULL;
		pos = newpos;
	}
	return vnl;
}

void vdeauth_setdefault(char *vnl) {
	vdeauth_active = 1;
	vdeauth_add(AUTH_DEFAULT, vnl);
}

int vdeauth_parsercfile(char *path) {
	int retvalue = 0;
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printlog(LOG_ERR, "configuration file: %s", strerror(errno));
		return -1;
	}
	char *line = NULL;
	size_t len;
	vdeauth_active = 1;
	for (int lineno = 1; getline(&line, &len, f) > 0; lineno++) { //foreach line
		char *scan = line;
		while (*scan && strchr("\t ", *scan)) scan++; //skip heading spaces
		if (strchr("#\n", *scan)) continue; // comments and empty lines
		int len = strlen(scan);
		char optname[len], value[len];
		// parse the line
		*value = 0;
		/* optname <- the first alphanumeric field (%[a-zA-Z0-9])
			 value <- the remaining of the line not including \n (%[^\n])
			 and discard the \n (%*c) */
		if (sscanf (line, "%[a-zA-Z0-9] %[^\n]%*c", optname, value) > 0) {
			switch(strcase_tolower(optname)) {
				case STRCASE(d,e,f,a,u,l,t):
					vdeauth_add(AUTH_DEFAULT, value); break;
					break;
				case STRCASE(a,l,l,o,w):
					vdeauth_add(AUTH_ALLOW, value); break;
				case STRCASE(d,e,n,y):
					vdeauth_add(AUTH_DENY, value); break;
				case STRCASE(p,f,x,a,l,l,o,w):
					vdeauth_add(AUTH_PFXALLOW, value); break;
				case STRCASE(p,f,x,d,e,n,y):
					vdeauth_add(AUTH_PFXDENY, value); break;
					break;
				default:
					printlog(LOG_ERR, "%s (line %d): unknown directive %s", path, lineno, optname);
					errno = EINVAL, retvalue = -1;
			}
		} else {
			printlog(LOG_ERR, "%s (line %d): syntax error", path, lineno);
			errno = EINVAL, retvalue = -1;
		}
	}
	fclose(f);
	if (line) free(line);
	return retvalue;
}

#if 0
int main(int argc, char *argv[]) {
	int rv = vdeauth_parsercfile(argv[1]);
	printf("rv %d\n", rv);

	char vnl[1024];
	while(fgets(vnl, 1024, stdin) != NULL) {
		if (vnl[strlen(vnl) - 1] == '\n') vnl[strlen(vnl) - 1] = 0;
		char *authvnl = vdeauth_check(vnl);
		if (authvnl)
			printf("%s\n", authvnl);
		else
			printf("DENY\n");
	}
}
#endif
