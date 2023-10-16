[image](https://github.com/xylophanes/PupsP3/assets/40770847/8d9b88ea-cf60-4c48-bde4-3e707041c6e8)

## Overview

PUPS/P3 is an implementation of an organic computing environment for [Linux](https://en.wikipedia.org/wiki/Linux)  which provides support for the implementation of low level persistent software agents.

PUPS/P3 processes are homeostatic agents. These agents are able to save their state and migrate between machines running compatible Linux kernels (via [criu](https://criu.org/Main_Page)). The PUPS/P3 API also gives them significant access to the state of their environment: like biological organisms they are animate. That is, they are able to sense changes in their environment and respond appropriately. Fir example, a P3 process may elect to save its state or migrate if some resource, for example processor cycles become scarce. Effectively, this is the machine equivalent of an animal electing hibernate or migrate when its food resources become scarce. PUPS/P3 can also share data resources via a low level persistent object, the shared heap. The semantics of using this are similar to those used by the free()/malloc() API supplied by standard C libraries.
<br>
<br>

![image](https://github.com/xylophanes/PupsP3/assets/40770847/077f2339-6532-487c-be53-ff61a5f26e54)

<br>
Figure 1. Schematic of PUPS/P3 process cluster. A number of PSRP server processor are shown and also; (1) a
maggot process (responsible for file system cleanup and continuity) and (2) psrp client processes communicating
with the server processors. The sensitive file is a file which contains an extension which enables it to be recognised by a
given PSRP server and then processed by it. The mechanism used is effectively the digital equivalent "lock and key" binding in enzymes an cellular signalling cascades.
<br>
<br>

Computations can be jointly executed by a cluster of co-operating P3 processes. This cluster is in many ways analalogous to a multicellular organism: like cells within an organisms, individual P3 processes can specialise. For example, in the case of the [Daisy](https://en.wikipedia.org/wiki/Digital_Automated_Identification_System) a pattern recognition system, the cluster consists of (ipm) processes which pre-process pattern-data, (floret) processes which run the PSOM neural nets used to classify those patterns, and (vhtml) processes which communicate the identity of patterns Daisy has discovered to the user via a dynamic [html](https://en.wikipedia.org/wiki/HTML) document. In addition, the Daisy cluster also has specialist (maggot) processes which clear and recycle file and memory space sand (lyosome) processes which destroy and replace other processes within the cluster which have become corrupted and therefore non functional.

In conjunction with virtualisation systems, for example [Oracle VirtualBox](https://www.oracle.com/uk/virtualization/virtualbox), it is possible to use PUPS/P3 to build homeostatic virtual (Linux) machines which can carry computational payloads while living in a dynamic cloud environment. The latest release of PUPS/P3 supports container based virtualization (via [Docker](https://www.google.com/search?client=firefox-b-e&q=docker) and check pointing with subsequent processs migration and/or restoration via criu.

## History
<br>
<br>

![image](https://github.com/xylophanes/PupsP3/assets/40770847/f80108f7-845b-4375-9490-0d67a15dbeb4)

<br>
Figure 2. Screenshot (circa 1985) of MSPS enviroment running on BBC model B.
<br>
<br>

PUPS/P3 began life as the MSPS operating environment, written in AcornSoft ISO-Pascal on the legendary BBC Model B Microcomputer in 1983. It migrated to C (and UNIX) in 1987, the first UNIX implementations being for SunOS and 4.3 BSD. The Linux PUPS/P3 implementation was begun in 1992, but most of the biologically inspired functionality was added between 1995 and the present date: the result of an ongoing and inspired collaboration between neuroscientists, biologists and computer scientists. Although PUPS/P3 is an organic computing environment its underlying philosophy draws on the goals of the influential Multiplexed Information and Computing Service, Multics.

