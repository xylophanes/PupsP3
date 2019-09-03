The 5.00 release is a major release. The sprint(), strcpy() and strcat()
functions have been replaced by their snprintf(), strlcpy() and
strlcat() replacments which are not susceptable to buffer overuns.
In addition, checkpoiting functionality (via Criu) has been added
and a number of minor bugs found and squashed.

--------------------------------------------------------------------------

The 4.0.3 release rationalises varibale passing to the core PUPS/P3
libraries via const parameters where possible. In making these changes
many minor bugs have been squashed. Added function strand to utilib
which generates random strings.

--------------------------------------------------------------------------

The 4.0.2 release corrects a serious bug in the ask function. In addition
the string functions provided by utilities library now (correctly) use
size_t (as opposed to int/long int). The string functions which make use
of static local variables ("strext" family) have been made thread safe.

--------------------------------------------------------------------------

The 4.0.0 is a major new release. The shared heap paradigm has been
completely updated and much functionality (e.g. slaved clients and
threading) has been improved. Bug fixes have been implemented in an
ad-hoc fashion as the code was upgraded and serveral features which
have not been used in production code have been deprecated.

--------------------------------------------------------------------------

The 3.0.6 release removes some debug code unintentionally left in
the signal handlers. Changed reading (and saving) of psrp rc
files. Now a PUPS/P3 application has to explicitly state it want to
read or write these files. Changed kill process group on exit action:
now a PUPS/P3 process needsd to explicitly state it wants to do this.

--------------------------------------------------------------------------

The 3.05 relase corrects some race conditions in pthreads code which
might lead to threaded processes haning when bubble memeory is allocated
or freed.

-------------------------------------------------------------------------- 

The 3.0.4 release ratinalises the debug code. Applications now build
properly with fully debuggable PUPS/P3 libaries. Added some additional
file descriptor tests (isafile, isapipe and isadataconduit) which
return the type of file system object(s) attached to stdin, stdout and
stderr. Bug in command line history has been corrected and somake
(used to build attachable shared libraries) upgraded.

--------------------------------------------------------------------------

The 3.0.3  backports mvmlib a library for dealing with virtual objects
back into the PUPS/P3 build tree. This functionality is required for
a number of legacy codes using PUPS/P3 but it might also be generally
useful for applications which need more memory than is physically
available on a host. HIPS (image) library API has been updated. PSRP
client help display updated. Bug in installer script squashed.

--------------------------------------------------------------------------

The 3.0.2 release fixes minor bugs in the 3..0 release

--------------------------------------------------------------------------

MAJOR NEW RELEASE 3.0.0. This release is a complete clean up of
the PUPS/P3 codebase. Some redundant features have been removed and
the "look and feel" of the API components has been stanadardised. The
level of in-line documentation in the code has been improved. In the
near future, "man" page documentation of the API will also be provided.
The psrp shell has also been cleaned up. It now uses "less" as opposed
to "more" as the default pager and now implements command history
correctly.

-------------------------------------------------------------------------

The 2.1.9 release is a MAJOR RELEASE. The PUPS/P£ API has been
rationalised with more meaningful function names (for functions
exported by each library). In addition, the code is now thread
safe and additional dynamic DLL functionality and smart data
caching facilities have been added.

-------------------------------------------------------------------------

The 2.1.8 release adds a new fast cache library. This allows
caches of data to memory mapped into the calling process address
space which allows for faster startup and object access within
calling PUPS/P3 applications. Also added build-date reporting to all
libraries and applications, some (limited) support for OpenMP and
removed bugs and added functionality to the Kepher garbage collector.

-------------------------------------------------------------------------

The 2.1.7 release adds a new function millitime() to the PUPS/P3
libraries - this returns time since the epoch accurate to
milliseconds. In addition the code base has been cleaned up and
several functions in the utilities library which were not threadsafe
are now. Support for older architectures (e.g. MIPS, SPARC, etc) has
been discontinued as has support of OS'es other than Linux. Build
scripts have also been streamlined and the functionality of the Kepher
garbage recycler has been improved.

-------------------------------------------------------------------------

The 2.1.6 release fixes more bugs in the automated build scripts so
non protected versions of PUPS/P3 now build. In addition support for
the Criu user space checkpointing package has been added.

------------------------------------------------------------------------

The 2.1.5b release fixes an annoying bug which stopped the
automated build scripts from working cleanly and bug in
pname_to_pid() which caused erronious duplicate process
errors.

------------------------------------------------------------------------

The 2.1.5 release fixes more bugs in the build and uninstall
scripts. Now it is possible to build a version of the software
where support tools and libraries can be debugged using gdb. Also
made a few minor cosmetic changes to some of the scripts and
code.

Added programs fadd, fsub, fmul and fdiv to add floating point
support to csh/tcsh scripts supporting PUPS/P3 applications.

Fixed some issues in the ask application which is part of the
the shell script support for PUPS/P3i.

Added fint and intf - float -> int and int -> float type casting
functions to support PUPS/P3 shell scripts.
-----------------------------------------------------------------------

The 2.1.4 release fixes some minor bugs in the main build script
and the VTE build script. These were uncovered while recently
porting PUPS/P3 to Odroid-x (ARMV7L) architecture which is now
supported.

----------------------------------------------------------------------

The 2.1.3 release fixes bugs encountered when psrp server processes
are backgrounded. The -nodetach flag now works correctly (e.g.
backgrounded processes stay attached to stdio) and servers which
have been started in the backgorund now (correctly) acquire an
instance of /dev/tty (and can can communicate with the controlling
terminal device (and hence the user) via stdio when they are
brought into the foreground.

This behaviour makes the use of P3 servers within sheel and other
sorts of scripts a little easier.

Added a default option to PUPS/P3 configure, Make configuration
tool. Canned builds using configure can now proceed without the
user having to explicitly default security options in each
Makefile generated using configure.

--------------------------------------------------------------------

The 2.1.2 release fixes a bug in the build script (which caused
problems when trying to build as root).

Minor improvements have been made to the larraylib fodling matrix
library to reduce the memory overhead of vector and matrix list
objects.

Added function to utilities library to check if a command is
installed (means that functions which call popen() do not have
to fail siliently).

Added function to utilities library to allow PUPS/P3 processes
to check CPU usage on host running them (so for example they
can migrate off the host if CPU usage on it gets too high).

Added -DPSRP_ENABLE to config scripts so applications using
skelpapp.c build correctly.

Removed deprecated X dependencies (which cuse build to break on
machines which don't have X installed.

Added sanity check for Intel/AMD 32 bit "namecreep" forces i686
to be synonomised with i386 so build doesn't break.

Corrected error in fp_dec() command line decoder function %lf
should be %F as user can now select whether P3 is built with
single (float) or double precision f.p. representation.

Corrected error in list array library function
lmatrix_get_compression_factor() which caused an incorrect
compression factor to be returned.

Added add, sub, mul and div to make it easier to perform integer
operations in (t)cshell, tcl and similar scripting languages. 

-------------------------------------------------------------------

The 2.1.1 release fixes bugs in the build script (fixed error
which caused debug build to fail).

This release also fixes problems in the list array library
(larraylib.c) which is used to process sparse matrices with
minimal memory overhead. Space saving based on realloc() has
been replaced with a quantisation scheme as the P3 malloc()
implementations idoesn't always release memory freed by
realloc to the system.

-------------------------------------------------------------------

2.1.0 is A MAJOR NEW RELEASE

Build scripts have been tidied up. Support for debugging
applications and libraries via gdb.

Support for user selectable floating point representation
(e.g. float or double). This was added to support ARM Neon,
CUDA, and Open CL implemetation which do not support double.

Sundry bugs in interface code and libraries fixed.

Clarity of psrp 'help' and 'chelp' options has been improved.

Versioning of PUPS/P3 libraries has been improved.

Much historic junk has been cleaned out the build tree.

MAO 08/08/2013

------------------------------------------------------------------

The 2.0.26 release adds extra debug features:

The configure script can now be specified with an optional
debug directive - if it is P3 application binaries which can
be interactively dbugged (via gdb) are built.

An editable Boolean debug_install has been added to
build_pups_services.csh (the primary buiild script). If set
to TRUE gdb debuggable P3 libraries are installed. If not
local libraries are built (which can only be used from within
the pupsp3-2.0.26/pupscore.src directory.

To build debuggable sustem (from pupsp3-2.0.26/pupscore.src)
type:

build_pups_services.csh cluster tty force default debug

These facilities are now available via the master build script:
buildP3.csh (from pupsp3-2.0.26):

To build (debuggable) type : buildP3.csh tty cluster force default debug
To build optimised type:     buildP3.csh tty cluster force default


MAO 05/08/2013

----------------------------------------------------------------

The 2.0.25 release fixes a number of bugs/features in the psrp client:

1. cntrl-c (interrupt) now works correctly, previously psrp
crashed if cntrl-c was pressed before a client process was
attached.

2. The simple macro language used by the PSRP client to define
macro commands has beed tidied up, deprecating a number of macro
command which do the same thing. See README.mtf in pupscore
directory

3.The help screen produced by typing 'chelp' has been tidied up.

4. A debug option has been added to the build script - so you can
build libraries and applications which can be debugged symbolically
using gdb. See README.debug in pupscore directory.

5. "man" pages have been tidied up where this was required.

6. The per-process fixed resources for PSRP servers, e.g. open files,
children etc. have been increased from 32 to 512 - 32 slots is tiny
by modern standards.

7. The bubble memory system used by P3 has been soak tested (at
last) without any bugs surfacing (yet)!

An additional library, larraylib which handles sparse vectors and
matrices has been added to the system to support applications which
need to do linear algebra on sparse systems. This library defines
a foldable list array type: objects of this type can be used to
optimally store sparse vectors and matrices with minimal memory
overhead.


MAO 19/07/2013

------------------------------------------------------------------

The 2.0.24 release fixes a number of minor bugs in the build scripts and
extends P3 support to embedded ARM linux systems: currently this release
builds on the Odroid (armv7l) and Raspberry Pi (armv6l) systems.

------------------------------------------------------------------

The 2.0.21 release fixes a bug which causes P3 enable applications to crash
if they run in certain environments (for example called via apache).

------------------------------------------------------------------

The 2.0.20 release has multithreading tested and running (at least on simple
examples in embryo.c test application).

------------------------------------------------------------------

The 2.0.19 release has eliminated further bugs. Dynamic objects are now
correctly saved by applications when they exist and automatically reloaded
when they start. I will be testing threads in the near future (which could
mean further releases over the next month).

------------------------------------------------------------------

The 2.0.18 release has eliminated a number of small bugs throughout the PUP/P3
libraries. More importantly, it is the first release of PUPS/P3 in which hash
and persistent object functionality is tested and working. This is a big deal
as it allows multiple processes (which could be on mutliple hosts) to access
shared memory via a low level interface which is similar to (and based upon)
the GNU malloc library.

The dynamic function interface is now also tested and functional. Access to
remote psrp resources now uses openssh (as opposed to a modified ssh client)
as this will be much easier to maintain without any real loss of functionality.

Multi-threading (via pthreads) is now in the codebase, compiles, but is yet to
be tested.

------------------------------------------------------------------

I apologise that I managed to break the build (in the 2.0.18 release of 21st
August). I have now repaired this - I was adding functionality to somake
(which builds shared objects for dynamic function import) and forgot it is
also used in the build process to generate the P3 .so libraries. This should
now be fixed. I suggest you download this version of the code if you grabbed the
21st August snapshot.

MAO - 24th August 2012
