<!--
.\" Copyright (C) 2023 VirtualSquare. Project Leader: Renzo Davoli
.\"
.\" This is free documentation; you can redistribute it and/or
.\" modify it under the terms of the GNU General Public License,
.\" as published by the Free Software Foundation, either version 2
.\" of the License, or (at your option) any later version.
.\"
.\" This manual is distributed in the hope that it will be useful,
.\" but WITHOUT ANY WARRANTY; without even the implied warranty of
.\" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
.\" GNU General Public License for more details.
.\"
.\" You should have received a copy of the GNU General Public License
.\" along with this program. If not, see <http://www.gnu.org/licenses/>.
.\"
-->

# NAME
vdelxcc(1) -- vde for containers: client

# SYNOPSIS

`vdelxcc`  [*options*]  [*VNL*]

`vdelxcc_static`  [*options*]  *socket* [*VNL*]

# DESCRIPTION

Create a network interface connected to a VDE network. `vdelxcc` forwards the
request to the `vdelxcd` daemon using a UNIX socket that must be shared with
the hosting environment (e.g. in a directory mounted at the container startup).

The default name of the communication socket is `vdelxcs` and the default
search path is `/vde:/var/vde` (i.e. `vdelxcc` looks for the socket as
`/vde/vdelxcs` and `/var/vde/vdelxcs`). The socket name or socket path can be
redefined using a specific option (see below). The search path can be modified
using the environment variable `VDELXC_PATH`.

The new interface is connected to the network defined by the Virtual Network Locator *VNL*
(see `vde_plug(1)` for the definitiion of VNL).
*VNL* can be omitted if the daemon `vdelxcd` defines a default network.

`vdelxcc` does not need any specific library. `vdelxcc` is a dynamically linked executable requiring only
the C library (libc). `vdelxcc_static` is a statically linked executable. It is suitable for
minimal containers having incompatible C libraries or not supporting dynamic linking.

# OPTIONS
  `-s` *socket*, `--socket` *socket*
: use this UNIX socket to communcate with the vdelxc daemon. If *socket* is a name (does not contain
slashes `/`) `vdelxcc` looks for the socket using the search path. Id *socket* is not a name
(contains slashes) *socket* is used as the pathname of the socket. Files of the current directory
can be referred as './name'.

  `-i` *iface*, `--iface` *iface*
: set the name or the prefix for the interface value. If the last char of *iface* is a digit,
(e.g. `vde3`) it is used as the interface name otherwise *iface* is the prefix of interface name.
For example using the option `-i eth` the interface is named `eth`*nnn*, i.e. `eth0` or `eth1`...
`vdelxcc` searches the first available name counting *nnn* from 0.
The default value of *iface* is `vde`, so the interfaces are named `vde0`, `vde1`, etc.

# NOTES

`vdelxcc` creates an interface using the tuntap support and transfers the file descriptor
to the `vdelxcd` daemon through the unix socket. The process of `vdelxcc` terminates, the
virtual network interface is kept alive by the `vdelxcd` server.

If necessary virtual interfaces can be terminated from inside the container, e.g.:
`ip link delete vde0`

# SEE ALSO
vde\_plug(1), vdelxcd(1)

# AUTHOR
VirtualSquare. Project leader: Renzo Davoli.
