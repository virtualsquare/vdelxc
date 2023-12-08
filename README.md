# *vdelxc*: vde support for Linux Containers

... and other network namespaces

VDE is the Virtual Distributed Ethernet project.
VDE provides an effective communication platform for virtual entities interoperability. In this context virtual entities can be virtual machines, namespaces, virtual switches and routers, etc. (See: `www.virtualsquare.org`)

VDE connections are identiied by Virtual Network Locators (VNL). (See `http://wiki.virtualsquare.org/#!tutorials/vdebasics.md`).

## Install
The following command sequence
builds and installs all the programs for *vdelxc* and their manual pages
(the build system is CMake).

```
 $ mkdir build
 $ cd build
 $ cmake ..
 $ make
 $ sudo make install
```

## Description

*vdelxc* uses the client server paradigm. The server `vdelxcd` is a daemon process running outside the container, in the hosting environment. The client command `vdelxcc` (or `vdelxcc-static`), running inside the container, creates a
virtual networking interface and sends a request to the server to join a VDE network. The new virtual interface is then directly handled by the server, once the startup process is complete the client terminates. Client and Server communicate through a UNIX socket that needs to be accessible from both (i.e. in a directory bind-mounted in the container).

The client `vdelxcc` does not need any specific library.
`vdelxcc` is a dynamically linked executable requiring only
the C library (libc). `vdelxcc_static` is a statically linked executable. It is suitable for
minimal containers having incompatible C libraries or not supporting dynamic linking.

The *vdelxc* support was designed for Linux containers but can be proven effective for creating virtual interfaces in network namespaces.


`vdelxcd` implements three security models:

* permissive: Any VNL is allowed.

* pre-defined network: The
default VLN identifies the only accessible network.
The client can specify no VNL or exactly the default VNL.

* configured (a configuration file is loaded by the command
option `-f`/`--rcfile`).
The configuration file defines which VLNs are allowed and which do not.
Default networks can be defined.
VNLs defined as default networks are authorized.

A virtual interface can be detroyed at any time from the container.
For example the following command destroys `vde0`.
```
$ ip link delete vde0
```

## Tutorial example

* create the shared directory

    for the client-server communication socket
```bash
    $ mkdir /tmp/vde
```

* create a namespace

    this example uses a busybox template
```bash
    lxc-create -n bb1 -t /usr/share/lxc/templates/lxc-busybox

```

* edit the configuration file `.local/share/lxc/bb1/config`

    comment out the default networking options
```
     #lxc.net.0.type = veth
     #lxc.net.0.link = lxcbr0
     #lxc.net.0.flags = up
```
&nbsp; &nbsp; &nbsp; &nbsp; uncomment: disable apparmor (if needed) 
```
     lxc.apparmor.profile = unconfined

```

&nbsp; &nbsp; &nbsp; &nbsp; mount `/dev/net/tun` and the shared directory `/tmp/vde`
```
     lxc.mount.entry = /dev/net/tun dev/net/tun none bind,create=file
     lxc.mount.entry = /tmp/vde vde none rw,bind,create=dir
```

* start the Linux container
```
    $ lxc-start -n bb1
```

* copy `vdelxcc` to the container
```
    $ cat vdelxcc | lxc-attach -n bb1 -- /bin/sh -c "cat > /usr/bin/vdelxcc; chmod 755 /usr/bin/vdelxcc"
```

* start the vdelxc server
```
    $ vdelxcd /tmp/vde/vdelxcs
```

* start a shell on the container

    (in another terminal window)
```
    $ lxc-attach -n bb1

    / #
```

* create a new VDE interface.
```
    / # ip addr
    1: lo: <LOOPBACK> mtu 65536 qdisc noop qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    / # vdelxcc vxvde://
    / # ip addr
    1: lo: <LOOPBACK> mtu 65536 qdisc noop qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    2: vde0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop qlen 1000
        link/ether ae:e9:d8:1b:1b:1b brd ff:ff:ff:ff:ff:ff
```
* configure the network and test it

    (10.0.0.2 is the address of a host, virtual machine, namespace or container
connected to `vxvde://`)
```
    / # ip addr add 10.0.0.1/24 dev vde0
    / # ip link set vde0 up
    / # ip addr
    1: lo: <LOOPBACK> mtu 65536 qdisc noop qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    2: vde0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel qlen 1000
        link/ether ae:e9:d8:1b:1b:1b brd ff:ff:ff:ff:ff:ff
        inet 10.0.0.1/24 scope global vde0
           valid_lft forever preferred_lft forever
        inet6 fe80::ace9:d8ff:fe1b:1b1b/64 scope link 
           valid_lft forever preferred_lft forever
    / # ping 10.0.0.2
    PING 10.0.0.2 (10.0.0.2): 56 data bytes
    64 bytes from 10.0.0.2: seq=0 ttl=64 time=0.849 ms
    64 bytes from 10.0.0.2: seq=1 ttl=64 time=0.551 ms
```

