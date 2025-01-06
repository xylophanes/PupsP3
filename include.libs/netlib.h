/*-------------------------------------------------------------------------
    Purpose: distributed programming library. Note almost all functions in
             this library are #ifdef'd. There are two sets, one for a
             generic POSIX enviroment and one with minimal functionality
             for non POSIX environments.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 5.00 
    Dated:   27th February 2024
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------*/

#ifndef NETLIB_H
#define NETLIB_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <utils.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#endif /* __C2MAN */


/***********/
/* Version */
/***********/

#define NETLIB_VERSION    "5.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE              2048


/*------------------------------------------------------------*/
/* Define dummy flag (which is set when we want blocking I/O) */
/*------------------------------------------------------------*/

#ifndef O_BLOCK
#define O_BLOCK 0
#endif


/*----------------------------------*/
/* Return codes from pidname parser */
/*----------------------------------*/

#define NOT_PARSED    0                // Failed to parse pidname
#define PARSED        1                // Parsed fully qualified pidname
#define PARSED_LOCAL  2                // Parsed local pidname


/*--------------------------------------------------------------------*/
/* xsystem control flags - note that whether these flags are honoured */
/* is in part a function of the shell used by the xsystem routine     */
/*--------------------------------------------------------------------*/

#define PUPS_STREAMS_DETACHED (1 << 0) // stdio detached from caller
#define PUPS_WAIT_FOR_CHILD   (1 << 1) // Wait for child before return
#define PUPS_ERROR_EXIT       (1 << 2) // Exit exec shell on first err
#define PUPS_READ_FIFO        (1 << 3) // Create a read FIFO from child
#define PUPS_WRITE_FIFO       (1 << 4) // Create write FIFO to child
#define PUPS_RW_FIFO          (1 << 5) // Create r/w FIFO to child
#define PUPS_NEW_SESSION      (1 << 6) // Run child in new session 
#define PUPS_NOAUTO_CLEAN     (1 << 7) // Don't automatically wait for child
#define PUPS_OBITUARY         (1 << 8) // Log death of child process


/*--------------------------------------------------*/
/* Stdio closure codes for pipeline detach function */
/*--------------------------------------------------*/

#define CLOSE_STDIN           (1 << 0) // Close standard input
#define CLOSE_STDOUT          (1 << 1) // Close standard output
#define CLOSE_STDERR          (1 << 2) // Close standard error
#define NEW_PGRP              (1 << 3) // Detach from pipeline process group


#ifdef __NOT_LIB_SOURCE__




/*-----------------------------------------------*/
/* Public variables exported by this application */
/*-----------------------------------------------*/

_EXPORT _BOOLEAN    use_nodel;         // Nodel toggle
_EXPORT jmp_buf     ch_jmp_buf;        // Channel err handlrr
_EXPORT  int32_t    rstat_timeout;     // Timeout rstat

#else
#   undef  _EXPORT
#   define _EXPORT
#endif

// Extended command processor routine
_PROTOTYPE _EXPORT int32_t pups_system(const char *, const char *, const uint32_t, pid_t  *);

// Open a command descriptor
_PROTOTYPE _EXPORT des_t pups_copen(const char *, const char *, const uint32_t);

// Close command descriptor
_PROTOTYPE _EXPORT int32_t pups_cclose(const des_t);

// Open a command stream
_PROTOTYPE _EXPORT FILE *pups_fcopen(const char *, const char *, const char *);

// Close command stream
_PROTOTYPE _EXPORT FILE *pups_fcclose(const FILE *);


#ifdef ZLIB_SUPPORT
// Open a command zstream
_PROTOTYPE _EXPORT gzFile *pups_gzcopen(const char *, const char *, const char *);

// Close command zstream
_PROTOTYPE _EXPORT gzFile *pups_gzcclose(const gzFile *);
#endif /* ZLIB_SUPPORT */

 
// Extended command processor routine [version 2]
_PROTOTYPE _EXPORT int32_t pups_system2(const char *, const char *, const uint32_t, pid_t *, des_t *, des_t *, des_t *); 

// Open a set command descriptors bound to pipeline [version 2]
_PROTOTYPE _EXPORT int32_t pups_copen2(const char *, const char *,  pid_t *, des_t *, des_t *, des_t *); 

// Open a set of streams bound to pipeline [version 2]
_PROTOTYPE _EXPORT int32_t pups_fcopen2(const char *, const char *, pid_t *, FILE *, FILE *, FILE *); 

// Close stdio descriptors
_PROTOTYPE _EXPORT void detach_from_pipeline(const  int32_t);

// Relay signal to process running on remote host
_PROTOTYPE _EXPORT int32_t pups_rkill(const char *, const char *, const char *, const char *, const uint32_t);

// Disable automatic killing of pipestream on the closure of associated descriptor
_PROTOTYPE _EXPORT int32_t pups_pipestream_kill_disable(const des_t);

// Enable automatic killing of pipestream on the closure of associated descriptor
_PROTOTYPE _EXPORT int32_t pups_pipestream_kill_enable(const des_t);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* Network support library */
