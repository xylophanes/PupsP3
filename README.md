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
--------

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
-----------------

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

PUPS/P3 build tools
-------------------

There are a number of build tools which can be used to automate the generation of PUPS/P3 applications. These include:
 
1. pupsconf: which is used to generate makefiles from PUPS/P3 makefile templates.
   
2. vtagup: which sets the build count of a given target source file.
   
4. vstamp: which timestamps a given target source file. The timestamp generated by this utility is used by various PUPS/P3 subsystems
to detect stale dynamic objects.

5. downcase: changes its argument string to lower case. Used in PUPS/P3 build and install scripts.
 
6. upcase: changes its argument string to upper case.
    
7. prefix: given a string prefix.suffix writes prefix to standard output. Used in PUPS/P3 build scripts.
    
8. suffix: given a string prefix.suffix writes suffix to standard output. Used in PUPS/P3 build scripts.

9. tas: test and set (lock directory). Used by the build scripts to ensure exclusive access to source directories in a multithreaded
mulituser environment.
    
10. error: prints error string associated with error numbers on standard output. Essentially, this is a wrapper around perror.

11. application: generates application template from skeleton template files.


PUPS/P3 Scripts
---------------

The PUPS/P3 distribution includes a number of shell scripts which facilitate building the PUPS/P3 environment. These include:

1. configure: generates a Makefile from a PUPS/P3 Makefile template. Essentially this is a wrapper script for pupsconf.

2. pupsuname: echo CPU and OS architecture to standard output (format is .). This is used by other tools, notably the build scripts to
put binaries and libraries in appropriate places.

3. install_pupsP3.csh: builds PUPS/P3 libraries, service functions, scripts and header files and installs them in specified locations.

4. uninstall_pupsP3.csh: uninstall PUPS/P3 libraries, service functions, scripts and header files.

5. export_pupsp3_sig.csh: export (source) PUPSP3 extended signal names for csh/tcsh shell.

6. unexport_pupsp3_sig.csh: unexport PUPSP3 extended signal names for csh/tcsh shell.

7. export_pupsp3_sig.sh: export (source) PUPSP3 extended signal names for bash shell.

8. unset_pupsp3_sig.sh: unexport (source) PUPSP3 extended signal names for bash shell.

9. restart.sh: restart (PUPS/P3) process which has been checkpointed via criu.

Service Functions
-----------------

The PUPS/P3 service functions are a set of tools which facilitate building of user applications in the PUPS/P3 environment. Many of the
functions (for example fsw, xcat and xtee) are effectively used as glue modules in virtual dataflow machines (which consist co-operating
pipelines of application processes). Other functions (for example nkill) extend the functionality of existing UNIX tools (e.g. Kill) in
potentially useful ways. The PUPS/P3 environment currently provides eight service functions:

1. nkill: Permits processes (which may be on remote hosts) to be signalled using a unique name rather than their Process IDentifier
(PID).

2. fsw, xcat, xtee: permit the efficent implementation of Virtual Dataflow Machines (VDMs). Fsw is a file system watcher which is able
to halt the execution of any VDM of which it is a member in the event of the VDM's output filesystem becoming full. Xcat is
primarily intended to act a place marker within VDMs which can be overforked by ephemeral processes (e.g. X11 and other interactive
applications). Typically, xcat processes provide targets within VDMs which can be overlaid (via execv system call) by interactive
monitoring and/or visulisation applications.

3. embryo: is an uncommited PSRP server application. Typically embryo is used to test new PUPS/P3 installations, but it may also be
used to build applications whose computational payloads are constructed of dynamic functions.
pass: an application carrier which enables non PUPS/P3 processes to gain access to PUPS/P3 and therefore PSRP services. Pass provides
homeostatic protection for application payload pipeline, and acts as an I/O manager which is capable of reading/writing data to and
from regular files, FIFOS and sockets. In addition, it provides hoemeostatic protection for the file system objects it is performing
I/O on, and can provide its payload process pipeline with thread-of-execution protection.

4. protect: is a file system object protection homeostat. This application will provide homeostatic protection for the file system
objects files, FIFOs and sockets presented on its command line.

5. lyosome: is a lightweight non PUPS/P3 aware utility which is similar in function to protect which protects a file system object that
is a file, FIFO or socket for a specified period of time. When this lifetime has expired, lyosome deletes the file system object and
exits.

6. maggot: is a service function which cleans the PUPS/P3 environment. It does this by removing stale checkpoint files, PSRP FIFOS,
sockets and other derelict file system objects. Maggot thus acts in a manner which is in keeping with its biological namesake.
Without the services of maggots, the PUPS/P3 environment could become cluttered with garbage objects generated by PUPS/P3
applications and services, and the corpses of processes which have ended abnormally (causing a core dump). Maggot detects and
removes these objects returning the space that they occupied to the file system: without a cleanup crew, ecosystems, both digital
and real-world will cease to function as they drown in their own waste!

7. psrp: is a text based client which allows the user sitting at an interactive terminal to interact with a PSRP server processes. From
the standpoint of the user psrp appears to be a pseudo-shell which supports many of the features associated with true UNIX shells
(e.g. Command history, command line editing, language [PSRP Macro Language - PML], initialisation scripts etc.). With psrp, the user
can open an asynchronous connection to any PSRP server, they can authenticate themselves to. Once the server is open (e.g. a
communication channel exists between psrp the PSRP client and the (PSRP) server, the PSRP protocol is used to transfer requests to
the server and its responses back to psrp (and hence to the users terminal).

8. thread: Is a tool for executing homogenous farms which consist of multiple instances of a given command.

9. p3f: tests to see is a given process is PSRP aware.

10. ecrypt: encrypt a plaintext file (via software simulation of an 8 rotor enigma machine).

11. servertool: start specified PSRP server in its own xterm window.

12. psrptool: start an instance of the psrp client in its own xterm window.

13. lol: tests to see if the owner of a (simple link) lock, PSRP channel or PUPS/P3 linkpost is alive

14. gob: provides an active mouth (the gob) into which data can be stuffed. This filter typically sits at the head of a process pipeline
and swallows up data to be processed by the pipeline in an interactive fashion.

15. arse: provides an active data sink (arse) from which data is emitted. This filter typically sits at the end of a process pipeline.

16. stripper: provides a comment stripper for virtual dataflow machines. Commented text (containing comment prepended by the '#'

17. vector vectors PSRP requests to a server located on remote host.

In addition to the service function described above, the PUPS/P3 environment also makes use of a modified version of the Secure SHell
(ssh) communication client. The version of ssh used by PUPS/P3 contains a set of extensions which permit the ssh client to work non
interactively using password authentication. This allows PSRP server processes to use ssh functionality to build encrypted tunnels for
data traffic between PSRP servers on networked hosts thereby enhancing system security.


The PUPS/P3 Project Directory
-----------------------------

The PUPS/P3 project is the source tree within which an implementor devel- ops PUPS/P3 applications. In order to create a PUPS/P3
project, the PUPS/P3 tree must have been installed in /home/pups. A project is created by typing:
project root directory architecture

This will make local copies (in the specified root directory) of all the components required to develop PUPS/P3 applications. The
architecture is a combination of CPU architecture, OS architecture and PUPS/P3 architecture. For example, a PUPS/P3 cluster
installation, running under Linux on an i686 would be specified as IX86.linux.cluster. A PUPS/P3 cluster installation, running under
Solaris on a SPARC would be SPARC.solaris.cluster. The project command creates a directory called src in root tree, and installs an
example Makefile, skeleton application and DLL development library file in it.

Once the PUPS/P3 project directory is installed and assuming that the implentor is in the directory src, the PUPS/P3 application builder
may be used to generate an application template using skelpapp.c and Make_skelpapp.in by default. The application template is built by
invoking the application command (if no arguments are given the default skeleton templates in the current directory are used). The
application builder will prompt (interactively) for the following:

   * Application name.
   * Author of application.
   * Authors E-mail.
   * Authors institution
   * A string describing the purpose of the application
   * The date (year).

The resulting application template and associated makefile may then be used as a basis for the application which is being implemented.


PSRP client command summary
---------------------------

The PSRP client, psrp has its own language, PML (PSRP Macro Language) and a large number of builtin commands. PML and its associated
builtin commands are documented below:

1. if cond cmd: execute cmd if condition cond is TRUE.

2. %label: target label for resume command.

3. resume %label: resume macro execution at label %label.

4. errorabort on|off: PML script aborted if on and error code for last command != "ok"

5. exit: exit macro that is currently executing.

6. abort: abort current PML script.

7. atomic cmd: do not attempt to expand cmd as macro.

8. body cmd: show body (if cmd is a macro).

9. repeat cnt : repeat command (which can be a macro) cnt times.

10. rperiod seconds: set repeat command repeat interval (in seconds).

11. repeat command>: repeat command infinitely.

12. raise : raise condition (pups_mainly used for testing PML scripts)

13. cinit: enter curses mode. This is mainly used prior to executing commands or macros which require curses style screen access.

14. cend: exit curses mode (returns to normal "glass tty" screen).

15. segaction [action]: specify/display request the action which is to be taken when a server segments (e.g. saves its state and then executes a child
which inherits that state). In modern PUPS/P3 applications segmentation has been rendered obsolete. It was developed as an ad hoc
way of working around malloc bugs which caused memory leaks but PUPS/P3 now has its own memory efficient (bubble) memory allocation
package based on the memory allocator shipped with The Tennessee Checkpointing Libraries.

17. thandler [handler]: Specify/display thread handler.

18. cls: clear screen (this is usually called just after a cinit command to prepare the screen for curses I/O.

19. sleep secs: Delay PML script execution for secs seconds.

20. retry on | off: enable on or disable off automatic request repetition (if PSRP server busy).

21. wait: Make PSRP client wait for PSRP server to start (and then connect to it).

22. nowait: Make PSRP client abort connection attempt (if target PSRP server is not running).

23. linktype: show type of PSRP channel linkage.

24. linktype hard | soft: set type of PSRP channel link to hard or soft. If the PSRP client is soft linked to the PSRP server it will
abort its connection if the server is stopped. A PSRP client which is hard linked to a PSRP server will stay connected to the server
even if the server stops. This option has two uses:

    + in the debugging of PUP/PSRP based VDM's.
    + in peer-to-peer PSRP connections between PSRP servers mediated by Slaved Interaction Clients SICs).
      
26. version: display version of this PSRP interaction client

27. id: print owner, uid, gid and controlling tty for this [psrp] process.

28. chanstat directory: show active PSRP channels in directory. If directory is ommitted, the default PSRP channel directory, /tmp is
assumed.

29. dllstat DLL pathname: show orifices (pointers to exportable objects) associated with Dynamic Link Library DLL name.

30. quit | exit | bye: terminates psrp client.

31. trys number: set number of attempts to open connection to PSRP server to number trys.

32. lcwd path: set current (local) working directory for PSRP client to path.

33. open PSRP server[@host]: open connection to PSRP server process PSRP server [on host]. If host is not specified, it is assumed that
the required server is running on the local host. The PSRP server may be specified by either name or PID.

34. close: close connection to PSRP server process.

35. chelp: display help on builtin commands for PSRP client.

36. pager: toggles paging mode (via less filter) on and off.

37. quiet: do not display output from builtin PSRP client commands

38. squiet: do not display output from PSRP server dispatch functions.

39. noisy: display output from builtin PSRP client commands

40. snoisy: display output from builtin PSRP server dispatch functions.

41. perror: print error handler status.

42. segcnt: display number of segmentations (for segmented server).

43. medit file: update/edit PML (PSRP Macro Language) definition file, file.

44. mload file: load (macro file (overwriting currently loaded list of macros).

45. mappend : append macro file to currently loaded list of macros.

46. mpurge all | file: delete all PML macros or those in file respectively.

47. macros: show tags for all loaded PML macros.

48. user username: change session owner for PSRP client to username.

49. password: set remote PSRP services authentication token. If this token is set it will be used for authentication when connecting to
PSRP servers running on remote hosts.

50. diapause: generate restartable PSRP server checkpoint file and exit (via Criu Checkpointing Library).

51. checkpoint: generate restartable PSRP server checkpoint file (via Criu Checkpointing Library).

52. !command: send command to users default shell.
    
53. relay slave process: relay data to/from slave process via psuedotty

54. server: request: send request (server builtin or dispatch function) request to PSRP server server.

55. server@<>host>: request: send (server builtin or dispatch function) request to PSRP server server running on node host.

56. c1; c2; c3: process multiple PSRP requests

57. "a1 a2": glob argument "a1 a2" to a1a2


PSRP Server Builtin Commands
-----------------------------
   
In addition to the set of static and dynamic dispatch functions which are built into a PSRP server application by the implementor, any
PSRP server (that is any program linked to the static library psrplib.o or the dynamic library psrplib.so) inherits a number of builtin
functions which can be accessed via the PSRP client. A list of these builtin functions is given below:

1. log on | off: switch server transaction logging on or off

2. appl_verbose on | off: switch server error logging on or off

3. show: display PSRP handler status showing all dispatch functions, databags an other objects attached to the server and server status
information.

4. clients: display clients connected to this server. Currently up to 256 clients may be concurrently bound to a given PSRP server. In
the present implementation, only one client at a time may actually transact with the server (the channels to the other attached
clients are temporarily blocked). In the future, if concurent PSRP server applications are implemented via the pthreads interface,
it will be possible to provide non-blocking concurrent access to such a PSRP server.

5. bindtype: display the binding type for the current PSRP server. Object binding may be either static or (static and) dynamic.

6. help: display on-line help information for builtin (PSRP server) commands.

7. atentrance: display list of (PUPS/P3) application entry functions (these functions are executed once at startup by the PSRP server).

8. atexit: display list of (PUPS/P3) application exit functions (these functions are executed once at exit by the PSRP server).

9. retrys retrys: set number of times PSRP server retrys operation (before aborting).

10. atrestart: display list of (PUPS/P3) application (checkpoint) restart functions. These functions are executed after a checkpointed
process has restored its state. Typically this functionality is used to re-attach dynamic objects which where detached by the
process prior to the checkpoint being taked.

11. atckpt: display list of (PUPS/P3) application (checkpoint) check- point functions. These functions are executed just before a