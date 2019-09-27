/*------------------------------------------------------------------------------------------
    Purpose: PUPS/PSRP signal mappings for generic Linux implementation

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   26th September 2019 
    Version: 2.00 
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------------------*/

#ifndef SIG_LINUX
#define SIG_LINUX

#define MAX_SIGS 68 

#ifdef __DEFINE__
_PRIVATE char  *signame[MAX_SIGS + 1]  =  { 
                                         "",
                            
                                         /*------------------*/
                                         /* Names of signals */
                                         /*------------------*/

                                         "SIGHUP",
                                         "SIGINT",
                                         "SIGQUIT",
                                         "SIGILL",
                                         "SIGTRAP",
                                         "SIGABRT",
                                         "SIGBUS",
                                         "SIGFPE",
                                         "SIGKILL",
                                         "SIGUSR1",
                                         "SIGSEGV",
                                         "SIGUSR2",
                                         "SIGPIPE",
                                         "SIGALRM",
                                         "SIGTERM",
                                         "SIGSTKFLT",
                                         "SIGCHLD",
                                         "SIGCONT",
                                         "SIGSTOP",
                                         "SIGTSTP",
                                         "SIGTTIN",
                                         "SIGTTOU",
                                         "SIGURG",
                                         "SIGXCPU",
                                         "SIGXFSZ",
                                         "SIGVTALRM",
                                         "SIGPROF",
                                         "SIGWINCH",
                                         "SIGIO",
                                         "SIGPWR",
                                         "SIGUNUSED1",
                                         "SIGRTUSED1",
                                         "SIGRTUSED2",
                                         "SIGRTUSED3",
				         "SIGRT0",
				         "SIGRT1",
				         "SIGRT2",
				         "SIGEVENT",
				         "SIGPSRP",
				         "SIGCHAN",
				         "SIGINIT",
				         "SIGCLIENT",
				         "SIGTHREAD",
				         "SIGSLAVE",
				         "SIGCHECK",
				         "SIGRESTART",
				         "SIGALIVE",
				         "SIGTHREADSTOP",
				         "SIGTHREADRESTART",
                                         "SIGCRITICAL",
                                         "SIGRT16",
				         "SIGRT17",
				         "SIGRT18",
				         "SIGRT19",
				         "SIGRT20",
				         "SIGRT21",
				         "SIGRT22",
				         "SIGRT23",
				         "SIGRT24",
				         "SIGRT25",
				         "SIGRT26",
				         "SIGRT27",
				         "SIGRT28",
				         "SIGRT29",
				         "SIGRT30",
				         "SIGRT31",
				         "SIGRT32"
                                      };
#endif /* __DEFINE__ */


/*------------------------------*/
/* P3 signal mappings for linux */
/*------------------------------*/

#define EXTENDED_SIGNAL_SET

#define SIGEVENT          SIGRTMIN + 4
#define SIGPSRP           SIGRTMIN + 5
#define SIGCHAN           SIGRTMIN + 6
#define SIGINIT           SIGRTMIN + 7
#define SIGCLIENT         SIGRTMIN + 8
#define SIGTHREAD         SIGRTMIN + 9
#define SIGSLAVE          SIGRTMIN + 10
#define SIGCHECK          SIGRTMIN + 11
#define SIGRESTART        SIGRTMIN + 12
#define SIGALIVE          SIGRTMIN + 13
#define SIGTHREADSTOP     SIGRTMIN + 14
#define SIGTHREADRESTART  SIGRTMIN + 15
#define SIGCRITICAL       SIGRTMIN + 16


#endif /* SIG_LINUX */
