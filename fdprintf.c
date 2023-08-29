/*
 * fdprintf: fd printf
 * Copyright (C) 2022  Renzo Davoli Virtualsquare University of Bologna
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program;
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int vfdprintf(int fd, const char * __restrict format, va_list ap) {
	int ret;
	va_list aq;
	va_copy(aq, ap);
	ret = vsnprintf(NULL, 0, format, aq);
	va_end(aq);
	if (ret < 0) return ret;
	char buf[ret + 1];
	ret = vsnprintf(buf, ret + 1, format, ap);
	if (ret < 0) return ret;
	ret = write(fd, buf, ret);
	return ret;
}

int fdprintf(int fd, const char * __restrict format, ...) {
	va_list ap;
	int ret;
	va_start(ap, format);
	ret = vfdprintf(fd, format, ap);
	va_end(ap);
	return ret;
}
