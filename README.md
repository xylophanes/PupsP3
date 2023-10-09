# PupsP3
PupsP3 Organic Computing Environment
====================================

PUPS/P3 is a set of enhancements for Linux which support persistent and complex computations via hoemostasis, persistent object stores
and in conjuction with the MOSIX system dynamic distributed compute-context load balancing. The PUPS/P3 system will work on both
standalone computers and within clusters of networked commodity PCs. Although inter-operation with MOSIX requires a Intel-x86 family CPU
hardware running Linux, versions of the PUPS/P3 system which do not support pre-emptive compute-context process migration may be built
for any operating system offering full POSIX compliance.

Via PSRP (Process Status Request Protocol), the PUPS/P3 system offers comprehensive mechanisms for communicating with running processes
(PSRP servers) which are carrying persistent computational payloads. This communication interface is inherently multi access (multiple
clients can talk to the same PSRP server process at the same time), and multi-user (clients owned by different users can talk to the
same PSRP server process at the same time provided that they can authenticate themselves to it). In order to support multi-user access,
the PUPS/P3 system provides full token based authentication services. A user must supply an authentication token in order to get client
access to a PSRP server which is owned by another user. PSRP server processes are contacted using the PSRP client application psrp. Both
the PSRP client and server have a number of built in commands which are available to any client or server respectively. In addition
application writers can add further commands to the PSRP server process via the dispatch function interface. Two sorts of dispatch
function are currently supported by PUPS/P3: static and dynamic. The first type of dispatch function is pre-compiled into a given PSRP
server by its implementor. The second type of dispatch function is dynamically associated with the PSRP server using the dynamic
link-loader interface supported by POSIX. In addition to functions, PUPS/P3 also supports the static and dynamic attachment of other
objects to running PUPS/P3 enabled applications.
   
These include:
   
1. System V shared memory segments.
2. Databags (simple contiguous data objects organised as a stream of bytes).
3. PUPS/P3 persistent heaps.

The PUPS/P3 persistent heap is a functional implentation of a persistent object store. It supports multi-process and multi-user access.
In networked environments, given an appropriate synchronous file system, for example, MFS, (the Mosix File System) or NFS, the persistent
heap may be physically located on a different machine to that running the PUPS/P3 enabled process or processes mapping it. PUPS/P3 persistent
heaps are based on the POSIX mmap interface, and are created, managed and destroyed using an API which resembles the familiar malloc
memory allocation package.


Building
========

Installation of PUPS/P3 from source is relatively straightforward. Firstly, the installation scripts assume that the default shell for
the user when building the system is either csh or tcsh If neither of these shells is currently the default, the default shell must be
changed appropriately using the chsh utility. After the tarball has been unpacked, the first stage of the installation process is to cd
to the root directory of the PUPS/P3 source distribution: dist.p3.src/pupscore.src. The script build_pups_services is then run (type
./install_pups_services ) in order to build the PUPS/P3 libraries and service functions and install them in the default locations. If
the build is successful (see build log) and the user is not root, by default, the binaries for the PUPS/P3 service functions, libraries,
   
For a non-root installation, header files and man pages are located in:

* ~/bin (PUPS/P3 utility commands)
* ~/lib (static and shared PUPS/P3 libraries)
* ~/include/p3 (PUPS/P3 header files)
* ~/man (PUPS/P3 man pages)
* ~/p3 (PUPS/P3 application templates)

If a root installation the corresponding default locations are:

* /usr/bin
* /usr/lib
* /usr/include/p3
* /usr/share/man
* /p3

The following shellscript variables in the build_pups_services shellscript may be edited to change the default installation locations
for PUPS/P3 software components:

* set BINDIR = "location of PUPS/P3 service function binaries"
* set LIBDIR = "location of PUPS/P3 libraries"
* set HDRDIR = "location of PUPS/P3 header files"
* set MANDIR = "location of PUPS/P3 man files"

To build the PUPS/P3 environment (as root) change directory to pupsp3-<version string>.src.git/pupscore.src and then type:
./install_pupsP3.csh cluster tty default

To clean the build tree type:
./install_pupsP3.csh clean

To uninstall the environment type:
./uninstall_pupsP3.csh

PUPS/P3 libraries
=================

Currently there are 12 core PUPS/P3 libraries:

1. utilib: which contains general purpose utilities including command tail decoding, extended I/O functionality, application process
name overloading, extended asynchronous event handling and a number of standard homeostats for protecting files and interprocess
communica- tion channels.

2. casino: Provides a number of psuedo-random number generators (after Knuth, 1984) and generators for Poisson, Normal, Binomial and
other well known distribution function in order to support applications which use stochastic optimisation.

3. netlib: Provides support for network computing including asynchronous signalling of processes running on remote hosts, remote
execution of processes, gathering of loading (and other status information) from remote hosts.

4. psrplib: Implements PSRP (Process Status Request Protocol). PSRP is a novel interprocess communications Protocol which enables both
users and also peer processes (PSRP clients) to interact asynchronously with a running application (the PSRP server) on an Internet
wide basis. PSRP is a secure protocol. With the provision of appropriate authentication tokens (e.g. password, PGP signatures etc.)
it is possible for a PSRP client to access and make use of resources which it does not own. In addition, the PSRP protocol is
multithreaded, which means it is possible for a running PSRP server application to be simultaneously accessed by multiple PSRP
clients. The PSRP library also provides vides a large number of builtin functions which support standard PSRP client-server
interactions including:

   + Status information about the PSRP server including number and identity of any connected client, memory utilisation, attached
   dynamic objects (e.g. persistent object stores and/or dynamic functions), number of open files etc.

   + A mechanism for dynamically importing dynamic functions into a running application (the PSRP interface supports the dynamic
   linking of code modules with strong type checking and possible type broking for example promotion of int to long or the
   depracation of double to float.

   + A mechanism for attaching and modifying persistent object stores: these are managed using the POSIX memory mapping API which
   permits a process to directly map the contents of a file (containing the persistent object store) into its address space. The
   POSIX memory mapping functionality has been extended to enable memory mapping over network files systems such as MFS or NFS.

   + Mechanisms for modifying the hoemostatic responses of an active PSRP server (e.g. Server checkpointing, migration and
   homeostatic protection can be switched on or off by appropriately authenticated PSRP clients).

5. mvmlib: provides support for PUPS/P3 processes to (optimally) manage their own memory

6. tadlib: provides support for PSRP servers and other PUPS/P3 applications which are multithreaded using the POSIX thread libraries.
  
7. cachlib: provides support for memory mapped data caches.

8. larraylib: sparse matrix support library.
   
9. hashlib: portable hash access libary.
    
11. pheaplib: persistent heap library. A persistent heap is an area of data memory which may be mapped into the
address spaces of multiple process.

12. dllib: supports dynamically pluggable (C) functions with strong typing.



   
