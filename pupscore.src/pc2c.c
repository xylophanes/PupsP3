/*-------------------------------------------------------------------------------------------
    Purpose: automatic (POSIX) threading of sources which are written to comply with
             PUPS semantics (impemnted as a C pre-processor).

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.01 
    Dated:   24th May 2023
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <xtypes.h>




/*-------------------------------------------------------------------------------------------
    Defines which are private to this application ...
-------------------------------------------------------------------------------------------*/
/*-----------------*/
/* Version of pc2c */
/*-----------------*/

#define PC2C_VERSION   "2.01"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE          2048 


#define UP             1
#define DOWN           (-1)
#define MTABLE_SIZE    256




/*--------------------------------------------------------------*/
/* Check that function head modifiers _ROOTTHREAD, _DLL_ORIFICE */
/* and _THREADSAFE are being used correctly                     */
/*--------------------------------------------------------------*/

_PROTOTYPE _PRIVATE _BOOLEAN check_function_head_modifiers(char *);

// Strip comments from line
_PROTOTYPE _PRIVATE void strip_line(char *, char *);

// Set up a default function mutex definition
_PROTOTYPE _PRIVATE void default_fmutex_define(char *);

// Set up a use function mutex definition
_PROTOTYPE _PRIVATE void use_fmutex_define(char *);

// Translate a _TKEY directive
_PROTOTYPE _PRIVATE void tkey_transform(char *);

// Translate a _TKEY_BIND directive
_PROTOTYPE _PRIVATE void tkey_bind_transform(char *);

// Translate a _TKEY_FREE directive
_PROTOTYPE _PRIVATE void tkey_free_transform(char *);

// Replace characrer c_1 in string s by character c_2 */
_PROTOTYPE _PRIVATE void ch_rep(char *, char, char);

// Check that string is upper case
_PROTOTYPE _PRIVATE _BOOLEAN is_upper(char *);

// Parse a PUPS-C macro function (checking its syntax)
_PROTOTYPE _PRIVATE _BOOLEAN parse_pups_c_macro_function(char *, int, char [32][SSIZE]);

// Check for empty lines (contain only spaces cntl-c)
_PROTOTYPE _PRIVATE _BOOLEAN empty_line(char *);

// Check if-else construct syntax
_PROTOTYPE _PRIVATE _BOOLEAN check_if_else(char *);

// Count number of (non-embedded) characters in line
_PROTOTYPE _PRIVATE int chcnt(char *, char);

// Check that function head conforms to PUPS-C syntax
_PROTOTYPE _PRIVATE _BOOLEAN ansi_c_head(char *);

// Check that orifice function prototype is correct
_PROTOTYPE _PRIVATE _BOOLEAN check_orifice_arguments(char *);

// Strip characters from string
_PROTOTYPE _PRIVATE void strpach(char *, char);

// Extended fputs function (checks for newline in O/P data)
_PROTOTYPE _PRIVATE void xfputs(char *, FILE *);

// Routine to skip text which has already been substituted
_PROTOTYPE _PRIVATE void skip_pre_substituted_text(char *);

// Routine to skip comments in source text
_PROTOTYPE _PRIVATE void skip_comment(char *);

// Initialise mutex table
_PROTOTYPE _PRIVATE void init_mtable(void);

// Add mutex to list of defined mutexes
_PROTOTYPE _PRIVATE _BOOLEAN defined(char *);

// Add mutex to list of defined mutexes
_PROTOTYPE _PRIVATE _BOOLEAN undefine(char *);

// Adjust function block count
_PROTOTYPE _PRIVATE void braket(int);

// Get position of character
_PROTOTYPE _PRIVATE int ch_pos(char *, char);

// Count number of bra '{' symbols in line
_PROTOTYPE _PRIVATE int nbras(char *);

// Count number of ket '}' symbols in line
_PROTOTYPE _PRIVATE int nkets(char *);

// Detect a PUPS function head
_PROTOTYPE _PRIVATE _BOOLEAN is_function_head(char *);

// Detect a PUPS function exit point
_PROTOTYPE _PRIVATE _BOOLEAN is_function_exit(char *);

// Test for occurence of s2 within s1
_PROTOTYPE _PRIVATE _BOOLEAN strin(char *, char *);


/*----------------------------------------------------------*/
/* Test for occurence of s2 within s1 retruning position of */
/* of extracted string                                      */
/*----------------------------------------------------------*/

_PROTOTYPE _PRIVATE _BOOLEAN strinpos(long *, long *, char *, char *);

// Reverse strip character from strinposg
_PROTOTYPE _PRIVATE char *rstrpch(char *, char);

// Routine to strip s2 from s1
_PROTOTYPE _PRIVATE _BOOLEAN strinstrip(char *, char *);




/*-------------------------------------------------------------------------------------------
    Variables which are private to this application ...
-------------------------------------------------------------------------------------------*/

                                                      /*------------------------------------*/
_PRIVATE _BOOLEAN in_function_body       = FALSE;     /* TRUE if in function body           */
_PRIVATE _BOOLEAN is_prototype           = FALSE;     /* TRUE if function prototype         */
_PRIVATE _BOOLEAN is_rootthreaded        = FALSE;     /* TRUE if func runs root thread only */
_PRIVATE _BOOLEAN is_threadsafe          = FALSE;     /* TRUE if funcguaranteed threadsafe  */
_PRIVATE _BOOLEAN is_dll_orifice         = FALSE;     /* TRUE if func DLL access orifice    */
_PRIVATE _BOOLEAN no_auto_unlock         = FALSE;     /* TRUE if auto mutex unlock mode     */
_PRIVATE _BOOLEAN use_default_mutex      = TRUE;      /* TRUE if using default mutex        */
_PRIVATE _BOOLEAN in_comment             = FALSE;     /* TRUE if pc2c translator in comment */
_PRIVATE _BOOLEAN one_line_comment       = FALSE;     /* TRUE if pc2c translator in comment */
_PRIVATE _BOOLEAN m_alternative_body     = FALSE;     /* TRUE if func has conitional forms  */
_PRIVATE int      f_ts_cnt               = 0;         /* Number of threadsafe functions     */
_PRIVATE int      f_rto_cnt              = 0;         /* Number of exec root thread funcs   */
_PRIVATE int      f_orifice_cnt          = 0;         /* Number of DLL orifice functions    */
_PRIVATE int      l_cnt                  = 0;         /* Line counter                       */
_PRIVATE int      r_cnt                  = 0;         /* Function block counter             */
_PRIVATE int      tsd_cnt                = 0;         /* TSD key counter                    */
_PRIVATE int      n_bras                 = 0;         /* bra '{' counter                    */
_PRIVATE int      n_kets                 = 0;         /* ket '}' counter                    */
_PRIVATE int      n_mutexes              = 0;         /* Number of user defined mutexes     */
_PRIVATE char     current_f_mutex[SSIZE] = "";        /* Currently active mutex macro       */ 
_PRIVATE char     default_mutex[SSIZE]   = "default"; /* Current default mutex              */ 
_PRIVATE char     mutex[SSIZE]           = "default"; /* Current insertion mutex            */ 
_PRIVATE char     current_mutex[SSIZE]   = "default"; /* Currently selected mutex           */
_PRIVATE char     line[SSIZE]            = "";        /* Line being parsed                  */
_PRIVATE char     current_f_line[SSIZE]  = "";        /* Current func def line              */
_PRIVATE char     m_alt_body_line[SSIZE] = "";        /* Line following #else statement     */                
_PRIVATE char     fname[SSIZE]           = "";        /* Current function name              */                
                                                      /*------------------------------------*/

                                                      /*------------------------------------*/
_PRIVATE char     defined_mutex[MTABLE_SIZE][SSIZE];  /* Table of user defined mutexes      */
                                                      /*------------------------------------*/




/*-------------------------------------------------------------------------------------------
    Main entry point for pc2c ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   _BOOLEAN is_f_head      = FALSE,
             is_f_exit      = FALSE; 

    char arglist[32][SSIZE] = { "" };

    (void)fprintf(stderr,"\nPUPS parallel C  to C translation tool version %s\n",PC2C_VERSION);
    (void)fprintf(stderr,"(C) Tumbling Dice, 2006-2023 (built %s %s)\n\n",__TIME__,__DATE__);

    if(isatty(0) == 1 || isatty(1) == 1)
    {  (void)fprintf(stderr,"Usage: pc2c < <input PUPS parallel C file> > <output C file>\n\n");
       (void)fflush(stderr);

       (void)fprintf(stderr,"PC2C is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PC2C comes with ABSOLUTELY NO WARRANTY\n\n");

       exit(255);
    }

    init_mtable();
    do {   

next_line: (void)fgets(line,SSIZE,stdin);
           if(feof(stdin) == 1)
              goto done;

           ++l_cnt;


           /*-------------------------------------------------------------------*/
           /* Print PUPS-C/C++ to C/C++ translation banner if this is the first */
           /* translation pass on input source file                             */
           /*-------------------------------------------------------------------*/

           if(l_cnt == 1 && strin(line,">>>>") == FALSE)
           {  (void)fprintf(stdout,"/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
              (void)fprintf(stdout,"/*    PUPS C/C++ text processed by pc2c version %-5s                 */\n",PC2C_VERSION);
              (void)fprintf(stdout,"/*    (C) M.A. O'Neill, Tumbling Dice                                */\n");
              (void)fprintf(stdout,"\n\n#include <tad.h>\n\n");
              (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
              (void)fflush(stdout);
           }


           /*-------------------------------------------------------------*/
           /* Skip lines which contain only spaces and/or '\n' characters */
           /*-------------------------------------------------------------*/

           if(empty_line(line) == TRUE)
              goto next_line;


           /*--------------------------------------------------------------------*/
           /* Substituted text - read lines of source until we reach end of text */
           /*--------------------------------------------------------------------*/

           skip_pre_substituted_text(line);


           /*--------------------------------------------------------------*/
           /* Comment - read lines of source until we reach end of comment */
           /*--------------------------------------------------------------*/

           skip_comment(line);


           /*------------------------------------------------------------*/
           /* Check for multi statement lines - invalid syntax in PUPS-C */
           /*------------------------------------------------------------*/

           if(chcnt(line,';') > 1 && strin(line,"for") == FALSE)
           {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
              (void)fflush(stderr);

              exit(255);
           }           


           /*------------------------------------------------------------*/
           /* Check to see if this line contains a macro #else statement */
           /*------------------------------------------------------------*/

           if(strin(line,"#else") == TRUE)
           {  m_alternative_body = TRUE;

              (void)xfputs(line,stdout);
              (void)fflush(stdout);
              (void)fgets(line,255,stdin);
              ++l_cnt;

              (void)strlcpy(m_alt_body_line,line,SSIZE);
           }


           /*-----------------------------------------------------------*/
           /* Check to see if this line has set a specific thread mutex */
           /* if it has, convert definition to macro                    */
           /*-----------------------------------------------------------*/

           if(strin(line,"_USE_FMUTEX") == TRUE)
           {  use_fmutex_define(line);
              goto next_line;
           }


           /*-------------------------------------------------------------*/
           /* Define a default mutex to be used globally as the mutex for */
           /* a set of contiguous functions. If no arguments are supplied */
           /* the builtin default mutex is used                           */
           /*-------------------------------------------------------------*/

           if(strin(line,"_DEFAULT_FMUTEX") == TRUE)
           {  default_fmutex_define(line);
              goto next_line;
           }


           /*--------------------------------------------------*/
           /* Destroy a key in which per thread data is stored */
           /*--------------------------------------------------*/

           if(in_function_body == TRUE && strin(line,"_TKEY_BIND") == TRUE)
           {  tkey_bind_transform(line);
              goto next_line;
           }  


           /*-----------------------------------------------*/
           /* Bind a key in which per thread data is stored */
           /*-----------------------------------------------*/

           if(in_function_body == TRUE && strin(line,"_TKEY_FREE") == TRUE)
           {  tkey_free_transform(line);
              goto next_line;
           }  


           /*----------------------------------------------------------*/
           /* Define a key variable in which per-thread data is stored */
           /*----------------------------------------------------------*/

           if(in_function_body == TRUE && strin(line,"_TKEY") == TRUE)
           {  tkey_transform(line);
              goto next_line;
           }


           /*-----------------------------------------*/
           /* Check for multiple '{' or '}' on a line */
           /* illegal under PUPS                      */
           /*-----------------------------------------*/

           n_bras = nbras(line);
           n_kets = nkets(line);


           /*-----------------------------------------------------------*/
           /* Check that block structuring conforms to PUPS-C semantics */
           /* Also check syntax of PUPS-C keywords                      */
           /*-----------------------------------------------------------*/

           if(n_bras  > 1                                                                                      ||
              n_kets  > 1                                                                                      ||
              (n_bras >  1  && n_kets >  1)                                                                    ||
              (n_bras == 1  && n_kets == 1 && strin(line,"=")            == FALSE)                             ||
              (strin(line,"_PRIVATE") == TRUE && strin(line,"_PUBLIC")   == TRUE)                              ||
              (strin(line,"_PRIVATE") == TRUE && strin(line,"_IMMORTAL") == TRUE)                              ||
              (strin(line,"_PUBLIC")  == TRUE && strin(line,"_IMMORTAL") == TRUE)                              ||
              (strin(line,"_PUBLIC")  == TRUE && strin(line,"(")         == TRUE) && in_function_body == TRUE  || 
              (strin(line,"_PRIVATE") == TRUE && strin(line,"(")         == TRUE) && in_function_body == TRUE   ) 
           {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
              (void)fflush(stderr);

              exit(255);
           }           


           /*-----------------------------------------*/
           /* Check to see if we have a function head */
           /*-----------------------------------------*/

           is_f_head = is_function_head(line);


           /*------------------------------------------------------------------*/
           /* Check that we don't have _ROOTHREAD, _THREADSAFE or _DLL_ORIFICE */
           /* anywhere except in function heads                                */
           /*------------------------------------------------------------------*/

           if(is_f_head == FALSE && is_prototype == FALSE)
           {  if(check_function_head_modifiers(line) == FALSE)           
              {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
                 (void)fflush(stderr);

                 exit(255);
              }
           }


           /*--------------------------------------------------------*/
           /* Check that if-else statements conform to PUPS-C syntax */ 
           /*--------------------------------------------------------*/

           if(check_if_else(line) == FALSE)
           {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
              (void)fflush(stderr);

              exit(255);
           }


           /*-----------------------------------------------*/
           /* Check to see if we have a function exit point */
           /*-----------------------------------------------*/

           is_f_exit = is_function_exit(line);


           /*-------------------------------------------------------------------*/
           /* If our previous (none space) line was #else and we were in a      */
           /* function auto re-enter it when we hit { (assumes that we have     */
           /* a multiple part conditional function)                             */
           /*-------------------------------------------------------------------*/

           if(m_alternative_body == TRUE)
           {  if(r_cnt == 0 && n_bras == 1)
              {  (void)strlcpy(mutex,current_mutex,SSIZE);
                 if(strcmp(current_mutex,default_mutex) != 0)
                    use_default_mutex = FALSE;

                 is_f_head = is_function_head(current_f_line);
              }
              m_alternative_body     = FALSE;
           }
 
           if(is_f_head == FALSE && is_f_exit == FALSE)
           {

              /*--------------------------------------------------*/
              /* Check for PUPS-C semantics before line is output */ 
              /* also check for end of function block             */
              /*--------------------------------------------------*/

              if(n_bras == 1 && n_kets == 0)
                 braket(UP);
              else if(n_bras == 0 && n_kets == 1)
              {  braket(DOWN);

                 if(r_cnt == 0)
                 {  in_function_body  = FALSE;
                    is_rootthreaded   = FALSE;
                    is_threadsafe     = FALSE;
                    is_dll_orifice    = FALSE;
                    no_auto_unlock    = FALSE;
                    use_default_mutex = TRUE;
                    (void)strlcpy(mutex,default_mutex,SSIZE);
                 }
                 else if(r_cnt < 0)
                 {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
                    (void)fflush(stderr);

                    exit(255);
                 }
              }

              (void)xfputs(line,stdout);
              (void)fflush(stdout);
           }
       } while(feof(stdin) == 0);

done:


    /*-------------------------------------------------------*/
    /* Print out summary of PUPS-C/C++ to C/C++ translations */
    /*-------------------------------------------------------*/

    if(f_ts_cnt > 0)
       (void)fprintf(stderr,"\n\n%d function(s) have been made thread safe\n",f_ts_cnt);

    if(f_orifice_cnt > 0)
       (void)fprintf(stderr,"%d function(s) have been made DLL orifices\n",f_orifice_cnt);

    if(f_rto_cnt > 0)
       (void)fprintf(stderr,"%d function(s) have been made exec root thread only\n\n",f_rto_cnt);

    if(tsd_cnt > 0)
       (void)fprintf(stderr,"%d TSD key(s) have been defined to protect thread-static data\n",tsd_cnt);

    (void)fprintf(stderr,"\nfinished\n\n");
    (void)fflush(stderr);

    exit(0);
}




/*-------------------------------------------------------------------------------------------
    Check that the orifice function conforms to a PUPS orifice function prototype ...
-------------------------------------------------------------------------------------------*/

_PRIVATE int check_orifice_arguments(char *line)

{   char tmp_str[SSIZE]  = "",
         ret_type[SSIZE] = "",
         arg1[SSIZE]     = "",
         arg2[SSIZE]     = "",
         arg3[SSIZE]     = "",
         arg4[SSIZE]     = "",
         line_buf[SSIZE] = "";

    (void)strlcpy(line_buf,line,SSIZE);
    (void)strpach(line_buf,'('); 
    (void)strpach(line_buf,','); 
    (void)strpach(line_buf,')'); 


    /*--------------------------------------------------------------------------------*/
    /* Case 1 function of form _PUBLIC _DLL_ORIFICE _BOOLEAN func(char *a1, char *a2) */
    /*--------------------------------------------------------------------------------*/

    if(sscanf(line_buf,"%s %s %s %s %s %s %s %s",tmp_str,
                                                 tmp_str,
                                                ret_type,
                                                 tmp_str,
                                                    arg1,
                                                    arg2,
                                                    arg3,
                                                    arg4) == 8)
    {   if(strcmp(ret_type,"_BOOLEAN") != 0)
           return(FALSE);

        arg2[1] = '\0';
        (void)strlcat(arg1,arg2,SSIZE);

        arg4[1] = '\0';
        (void)strlcat(arg3,arg4,SSIZE);

        if(strcmp(arg1,"char*") == 0 || strcmp(arg3,"char*") == 0)
           return(TRUE); 
    }


    /*---------------------------------------------------------------------------------*/
    /* Class 2 function of form _PUBLIC _DLL_ORIFICE _BOOLEAN func(char* a1, char *a2) */
    /*---------------------------------------------------------------------------------*/

    if(sscanf(line_buf,"%s %s %s %s %s %s",tmp_str,
                                           tmp_str,
                                          ret_type,
                                           tmp_str,
                                              arg1,
                                              arg2) == 6)
    {   if(strcmp(ret_type,"_BOOLEAN") != 0)
           return(FALSE);

        if(strcmp(arg1,"char*") != 0 || strcmp(arg2,"char*") != 0)
           return(FALSE);
    }

    return(FALSE);
}




/*-------------------------------------------------------------------------------------------
    Check that function head conforms to PUPS-C syntax ...
-------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN ansi_c_head(char *line)

{   int i,
        eq_index        = (-1),
        bra_index       = (-1),
        cnt             = 0;

    char line_buf[4096] = "";

    if(in_function_body == TRUE)
       return(FALSE);


    /*----------------------------------------------------*/
    /* Strip any embedded comments out of the line before */
    /* we try to process it                               */
    /*----------------------------------------------------*/

    strip_line(line,line_buf);


    /*-----------------------------------------*/
    /* If this is a macro function - ignore it */
    /*-----------------------------------------*/

    if(strin(line_buf,"_USE_FMUTEX")            == TRUE    ||
       strin(line_buf,"_DEFINE_FMUTEX")         == TRUE    ||
       strin(line_buf,"_DEFINE_DEFAULT_FMUTEX") == TRUE    ||
       strin(line_buf,"_TKEY_BIND")             == TRUE    ||
       strin(line_buf,"_TKEY_FREE")             == TRUE    ||
       strin(line_buf,"_TKEY")                  == TRUE     )
       return(FALSE);
 
    for(i=0; i<cnt; ++i)
    {  if(line_buf[i] == '(' && bra_index == (-1))
          bra_index = i;
       else if(line_buf[i] == '=' && eq_index == (-1))
          eq_index  = i;
    }

    if(eq_index != (-1) && bra_index > eq_index)
       return(FALSE);

    if(strin(line_buf,"_EXTERN")  == FALSE    &&
       strin(line_buf,"_PRIVATE") == FALSE    &&
       strin(line_buf,"_PUBLIC")  == FALSE    &&
       bra_index                  != (-1)      )
       return(TRUE);

    return(FALSE);
}




/*-------------------------------------------------------------------------------------------
    Find function head - if found make sure threads calling the function lock it
    on entry ...
-------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN is_function_head(char *line)

{   int      h_cnt;

    char     stripped_line[4096] = "",
             f_head_line[4096]   = "";

    if(in_function_body == TRUE)
       return(FALSE);

    strip_line(line,stripped_line);


    /*------------------------------------------------------*/
    /* Check that we do not have a non PUPS-C function head */
    /*------------------------------------------------------*/

    if(ansi_c_head(stripped_line) == TRUE)
    {  (void)fprintf(stderr,"%d: ANSI PUPS-C syntax error %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }


    /*-------------------------------------------*/
    /* We have a PUPS-C function head - parse it */
    /*-------------------------------------------*/

    if(strin(stripped_line,"(") == TRUE  &&
       strin(stripped_line,"=") == FALSE && 
       (strin(line,"_PRIVATE") == TRUE || strin(line,"PUBLIC") == TRUE))
    {

       /*-----------------------------------------*/
       /* We have a function head - find its body */
       /*-----------------------------------------*/

       int h_cnt           = 0,
           index,
           n_args;

       char prev_item[SSIZE] = "",
            next_item[SSIZE] = "",
            line_buf[SSIZE]  = "",
            tmp_str[SSIZE]   = "";

       _BOOLEAN is_main    = FALSE,
                looper     = FALSE;


       /*----------------------------------------------------*/
       /* If the function is not declared as either _PUBLIC  */
       /* or _PRIVATE it does not conform to PUPS-C syntax   */
       /*----------------------------------------------------*/

       if(strin(stripped_line,"_PUBLIC") == FALSE && strin(stripped_line,"_PRIVATE") == FALSE)
       {  (void)fprintf(stderr,"%d: PUPS parallel C syntax error %s\n",h_cnt,rstrpch(f_head_line,'\n'));
          (void)fflush(stderr);

          exit(255);
       }


       /*----------------------------------------*/
       /* Save function head for error reporting */
       /*----------------------------------------*/

       h_cnt = l_cnt;
       (void)strlcpy(f_head_line,stripped_line,SSIZE);


       /*---------------------------------------------------*/
       /* If we have function definition of form:           */
       /* _PROTOTYPE int func(void); etc return immediately */
       /*---------------------------------------------------*/

       if(strin(stripped_line,";") == TRUE)
       {  if(strin(stripped_line,"_PROTOTYPE") == TRUE)
          {  is_prototype = TRUE;
             return(FALSE);
          }
          else
          {  (void)fprintf(stderr,"%d: PUPS-C syntax error %s\n",h_cnt,rstrpch(f_head_line,'\n'));
             (void)fflush(stderr);

             exit(255);
          }
       }


       /*----------------------------------------------------*/
       /* Extract function name from function head statement */
       /*----------------------------------------------------*/

       (void)strlcpy(current_f_line,line,SSIZE);
       (void)strlcpy(line_buf,stripped_line,SSIZE);
       do {   (void)strlcpy(prev_item,next_item,SSIZE); 
              looper = strinstrip(line_buf,next_item);

              if(strin(next_item,"(") == TRUE)
              {  if(next_item[0] == '(')
                   (void)strlcpy(fname,prev_item,SSIZE);
                 else 
                   (void)strlcpy(fname,next_item,SSIZE);

                 looper = FALSE;
              }
          } while(looper == TRUE);

       index = ch_pos(fname,'(');
       fname[index] = '\0';


       /*-------------------------------------------*/
       /* Check for type of substitution to be made */
       /*-------------------------------------------*/

       if(strin(stripped_line,"main(") == TRUE)
       {  is_main         = TRUE;
          is_rootthreaded = TRUE;
       }

       if(strin(stripped_line,"_ROOTTHREAD") == TRUE)
          is_rootthreaded = TRUE;

       if(strin(stripped_line,"_DLL_ORIFICE") == TRUE && m_alternative_body == FALSE)
       {  if(check_orifice_arguments(line) == FALSE)
          {  (void)fprintf(stderr,"%d: PUPS-C syntax error %s\n",l_cnt,rstrpch(line,'\n'));
             (void)fflush(stderr);

             exit(255);
          }

          if(fname[0] == '*')
             (void)strlcpy(tmp_str,&fname[1],SSIZE);
          else
             (void)strlcpy(tmp_str,fname,SSIZE);

          (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
          (void)fprintf(stdout,"/* Inserted automatically by pc2c */\n");
          (void)fprintf(stdout,"_PUBLIC _BOOLEAN %s_is_orifice = TRUE;\n",tmp_str);
          (void)fprintf(stdout,"_PUBLIC char %s_name[SSIZE]      = \"%s\";\n",SSIZE,fname,fname);
          (void)fprintf(stdout,"_PUBLIC char %s_prototype[SSIZE] = \"%s\";\n",SSIZE,fname,rstrpch(line,'\n'));
          (void)fflush(stdout);
          (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
              
          is_dll_orifice = TRUE;
          ++f_orifice_cnt;
       }
       else if(m_alternative_body == FALSE)
       {  if(fname[0] == '*')
             (void)strlcpy(tmp_str,&fname[1],SSIZE);
          else
             (void)strlcpy(tmp_str,fname,SSIZE);

          (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
          (void)fprintf(stdout,"/* Inserted automatically by pc2c */\n");
          (void)fprintf(stdout,"_PUBLIC _BOOLEAN %s_is_orifice = FALSE;\n",tmp_str);
          (void)fflush(stdout);
          (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
       }

       if(strin(stripped_line,"_PROTOTYPE") == TRUE)
          is_prototype = TRUE;
       else
          is_prototype = FALSE;


       /*----------------------------------------------------*/
       /* Do not re-emit head if this is an alternative body */
       /* for a macro conditional function                   */
       /*----------------------------------------------------*/

       if(m_alternative_body == FALSE)
       {  (void)xfputs(line,stdout);
          (void)fflush(stdout);
       }


       /*------------------------------------------------------*/
       /* If this function has been declared threadsafe simply */
       /* note that we are in a function body and return       */
       /*------------------------------------------------------*/

       if(strin(stripped_line,"_THREADSAFE") == TRUE)
          is_threadsafe = TRUE;

       do {    _BOOLEAN is_substituted = FALSE; 


               /*--------------------------------------------*/
               /* Search for body of function                */ 
               /* unless this is a macro-expansion           */
               /* in which case we simply skip to the action */
               /*--------------------------------------------*/

               if(m_alternative_body == FALSE)
               {  do {   (void)fgets(line,SSIZE,stdin);
                         ++l_cnt;


                         /*----------------------------------------*/
                         /* Check to see if this is a function     */
                         /* definition. If it is we will find a ;  */
                         /* rather than a { as function definitios */
                         /* have no bodies                         */
                         /*----------------------------------------*/

                         strip_line(line,stripped_line);
                         if(strin(stripped_line,";") == TRUE && strin(stripped_line,"{") == FALSE)
                         {

                            /*---------------------------------------------------*/
                            /* If we have got here and we are not in a prototype */
                            /* function declaration, this is an error            */
                            /*---------------------------------------------------*/

                            if(is_prototype == FALSE)
                            {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",h_cnt,rstrpch(f_head_line,'\n'));
                               (void)fflush(stderr);

                               exit(255);
                            } 

                            is_rootthreaded   = FALSE;
                            is_threadsafe     = FALSE;
                            is_dll_orifice    = FALSE;
                            use_default_mutex = TRUE;
                            in_function_body  = FALSE;

                            return(FALSE);
                         }


                         /*-------------------------------------*/
                         /* Check for pre-substitutions         */
                         /* if pre-substituted abort processing */
                         /*-------------------------------------*/

                         if(strin(stripped_line,">>>>") == TRUE)
                         {  skip_pre_substituted_text(stripped_line);

                            is_rootthreaded   = FALSE;
                            is_threadsafe     = FALSE;
                            is_dll_orifice    = FALSE;
                            use_default_mutex = TRUE;
                            in_function_body  = TRUE;

                            return(TRUE);
                         }
                         else
                         {

                            /*--------------------------------------------*/
                            /* Check for syntax errors in function header */
                            /*--------------------------------------------*/

                            n_bras = nbras(stripped_line);
                            n_kets = nkets(stripped_line);

                            if((n_bras > 1 && n_kets > 0)                                     ||
                               n_kets  > 0                                                    || 
                               (strin(stripped_line,"_PUBLIC")  == TRUE && strin(line,"(") == TRUE)    || 
                               (strin(stripped_line,"_PRIVATE") == TRUE && strin(line,"(") == TRUE)     ) 
                            {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",h_cnt,rstrpch(f_head_line,'\n'));
                               (void)fflush(stderr);

                               exit(255);
                            } 

                            if((n_bras = nbras(stripped_line)) == 0)
                            {  (void)xfputs(line,stdout);
                               (void)fflush(stdout);
                            }
                         }
                      } while(n_bras == 0);
               }


               /*---------------------------------------------------------*/ 
               /* If we have got here and we have a prototype definition  */
               /* we must raise an error -- prototypes do not have bodies */ 
               /*---------------------------------------------------------*/ 

               if(is_prototype == TRUE)
               {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",h_cnt,rstrpch(f_head_line,'\n'));
                  (void)fflush(stderr);

                  exit(255);
               }


               /*---------------------------------------*/
               /* Problem with function block structure */
               /*---------------------------------------*/

               if(n_bras > 1)
               {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
                  (void)fflush(stderr);

                  exit(255);
               }


               in_function_body = TRUE; 
               if(n_bras == 1 && n_kets == 0)
                  braket(UP);

               if(is_threadsafe == TRUE) 
               {  if(m_alternative_body == FALSE)
                  { (void)xfputs(line,stdout);
                    (void)fflush(stdout);
                  }
                  else
                  {  (void)xfputs(m_alt_body_line,stdout);
                     (void)fflush(stdout);
                  }

                  return(TRUE);
               }


               /*---------------------------------------------------------*/
               /* Check to see that this function is not threaded already */
               /* if it is skip to the end of the function                */
               /*---------------------------------------------------------*/

               if(strin(stripped_line,"_LOCK_THREAD_FMUTEX_")      == TRUE    ||
                  strin(stripped_line,"_INIT_PTHREAD_FMUTEXES")    == TRUE    ||
                  strin(stripped_line,"_DLL_ORIFICE_FUNCTION")     == TRUE     )
               {  do {    (void)fgets(line,SSIZE,stdin);
                          ++l_cnt;

                          strip_line(line,stripped_line);
                          if(feof(stdin) == 1)
                          {  (void)fprintf(stderr,"Premature end of file\n");
                             (void)fflush(stderr);

                             exit(255);
                          }

                          n_bras = nbras(stripped_line);
                          n_kets = nkets(stripped_line);


                          /*----------------------------------------------------------*/
                          /* Check for syntax errors in pre-substituted function body */
                          /*----------------------------------------------------------*/

                          if(n_bras > 1 && n_kets > 1                                       ||
                             (strin(stripped_line,"_PUBLIC")  == TRUE && strin(stripped_line,"(") == TRUE)    || 
                             (strin(stripped_line,"_PRIVATE") == TRUE && strin(stripped_line,"(") == TRUE)     ) 
                          {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",h_cnt,rstrpch(f_head_line,'\n'));
                             (void)fflush(stderr);
                          }  

                          if(n_bras == 1 && n_kets == 0)
                             braket(UP);
                          else if(n_bras == 0 && n_kets == 1)
                             braket(DOWN);

                     } while(r_cnt > 0);

                  return(TRUE);
               }


               /*-----------------------------------------------------------*/
               /* If this function is root threaded make sure that only the */
               /* root thread fgets to run it                               */
               /*-----------------------------------------------------------*/

               if(is_rootthreaded == TRUE || is_main == TRUE)
               {  (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
                  (void)fprintf(stdout,"{   /* Inserted automatically by pc2c */\n");

                  if(is_main == TRUE)
                     (void)fprintf(stdout,"    _INIT_PUPS_THREADS\n");

                     (void)fprintf(stdout,"    _EXEC_THREAD_ROOT_THREAD_ONLY\n");

                  (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");

                  if(m_alternative_body == FALSE)
                  {  index = ch_pos(line,'{');
                     line[index] = ' ';
                     (void)xfputs(line,stdout);
                     (void)fflush(stdout);
                  }
                  else
                  {  index = ch_pos(m_alt_body_line,'{');
                     m_alt_body_line[index] = ' ';
                     (void)xfputs(m_alt_body_line,stdout);
                     (void)fflush(stdout);
                  }

                  //index = ch_pos(fname,'(');
                  //fname[index] = '\0';

                  if(is_main == TRUE)
                     (void)fprintf(stderr,"%d: making PUPS \"main\" function pthread initialisation function\n",l_cnt);

                  (void)fprintf(stderr,"%d: making PUPS function \"%s\" root thread only\n",l_cnt,fname);

                  if(is_dll_orifice == TRUE)
                     (void)fprintf(stderr,"%d: making PUPS function \"%s\" a DLL orifice\n",l_cnt,fname);

                  (void)fflush(stderr);

                  ++f_rto_cnt;
                  return(TRUE);
               }
               else if(is_rootthreaded == FALSE)
               {  (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
                  (void)fprintf(stdout,"{   /* Inserted automatically by pc2c */\n");

                  if(use_default_mutex == TRUE)
                     (void)fprintf(stdout,"    _LOCK_THREAD_FMUTEX(%s)\n",default_mutex);
                  else
                     (void)fprintf(stdout,"    _LOCK_THREAD_FMUTEX(%s)\n",mutex);

                  (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
                  (void)fflush(stdout); 
               }

               if(m_alternative_body == FALSE)
               {  index = ch_pos(stripped_line,'{');
                  line[index] = ' ';
                  (void)xfputs(line,stdout);
                  (void)fflush(stdout);
               }
               else
               {  index = ch_pos(m_alt_body_line,'{');
                  m_alt_body_line[index] = ' ';
                  (void)xfputs(m_alt_body_line,stdout);
                  (void)fflush(stdout);
               }

               index = ch_pos(fname,'(');
               fname[index] = '\0';

               (void)fprintf(stderr,"%d: making PUPS function \"%s\" threadsafe (using mutex \"%s\")\n",
                                                                                l_cnt,fname,mutex);

               if(is_dll_orifice == TRUE)
                  (void)fprintf(stderr,"%d: making PUPS function \"%s\" a DLL orifice\n",l_cnt,fname);

               (void)fflush(stderr);

               ++f_ts_cnt;
               return(TRUE);
          } while(feof(stdin) == 0);
    }

    return(FALSE);
}




/*-------------------------------------------------------------------------------------------
    Find the exit points for a function and make sure thread releases locks on exit
    from them ...
-------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN is_function_exit(char *line)

{   char tmp_str[SSIZE] = "";
    int  index;


    /*-----------------------------------------*/
    /* If we are not in a function body return */
    /*-----------------------------------------*/

    if(in_comment       == TRUE     ||
       in_function_body == FALSE    ||
       is_rootthreaded  == TRUE     ||
       is_threadsafe    == TRUE      )
       return(FALSE);


    /*---------------------------------------------------*/  
    /* If we have a return embedded in a text literal or */
    /* comment, it should be ignored                     */
    /*                                                   */ 
    /* Search for an explicit return statement           */
    /* and return starting index for it in line          */
    /*---------------------------------------------------*/  

    if(strinpos((long *)&index,(long *)NULL,line,"return") == TRUE)
    {  char tmp_str[SSIZE] = "";
       int  embed_index;


       /*---------------------------------------------*/
       /* Make sure return is not embedded in comment */
       /* or text literal in which case it should be  */
       /* ignored                                     */ 
       /*---------------------------------------------*/

       embed_index = ch_pos(line,'"');
       if(index > embed_index && embed_index != (-1))
          return(FALSE);

       if(strinpos((long *)&embed_index,(long *)NULL,line,"/*") == TRUE)
       {  if(index > embed_index)
             return(FALSE);
       }

       if(strinpos((long *)&embed_index,(long *)NULL,line,"//") == TRUE)
       {  if(index > embed_index)
             return(FALSE);
       }


       /*------------------------------------------------*/
       /* Strip out leading spaces from return statement */
       /*------------------------------------------------*/

       (void)strlcpy(tmp_str,&line[index],SSIZE);
       (void)strlcpy(line,tmp_str,SSIZE);
       index = ch_pos(line,'\n');
       line[index] = '\0';

       (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
       (void)fprintf(stdout,"    {   /* Inserted automatically by pc2c */\n");

       if(use_default_mutex == TRUE)
          (void)fprintf(stdout,"        _UNLOCK_THREAD_FMUTEX(%s)\n",default_mutex);
       else
          (void)fprintf(stdout,"        _UNLOCK_THREAD_FMUTEX(%s)\n",mutex);

       (void)fprintf(stdout,"        %s\n    }\n",line);
       (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
       (void)fflush(stdout);


       /*-------------------------------------------------*/
       /* If this is the outermost block of a function    */
       /* there is no need to automatically unlock the    */
       /* current mutex when we finally exit the function */
       /*-------------------------------------------------*/

       if(r_cnt - 1 == 0)
          no_auto_unlock = TRUE;

       return(TRUE);
    }

    n_kets = nkets(line);
    if(n_bras == 0 && n_kets == 1 && no_auto_unlock == FALSE)
    {  if(no_auto_unlock == FALSE) 
       {  braket(DOWN);
          if(r_cnt == 0) 
          {  (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n"); 
             (void)fprintf(stdout,"    /* Inserted automatically by pc2c */\n");

             if(use_default_mutex == TRUE)
                (void)fprintf(stdout,"    _UNLOCK_THREAD_FMUTEX(%s)\n}\n",default_mutex);
             else
                (void)fprintf(stdout,"    _UNLOCK_THREAD_FMUTEX(%s)\n}\n",mutex);

             (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
             in_function_body  = FALSE;
             is_rootthreaded   = FALSE;
             is_threadsafe     = FALSE;
             is_dll_orifice    = FALSE;
             use_default_mutex = TRUE;
             (void)strlcpy(mutex,default_mutex,SSIZE);

             return(TRUE);
          }
          braket(UP);
       }
    }
    
    no_auto_unlock = FALSE;
    return(FALSE);
}





/*-------------------------------------------------------------------------------------------
    Find the exit points for a function and make sure thread releases locks on exit
    from them ...
-------------------------------------------------------------------------------------------*/

_PRIVATE int ch_pos(char *s, char c)

{   int i;

    for(i=0; i<strlen(s); ++i)
       if(s[i] == c)
          return(i);

    return(-1);
}





/*-------------------------------------------------------------------------------------------
    Count the number of (non-embedded) characters in a line ...
-------------------------------------------------------------------------------------------*/

_PRIVATE int chcnt(char *line, char ch)

{   int i,
        bra_cnt = 0;

    _BOOLEAN in_comment     = FALSE,
             in_txt_literal = FALSE;

    for(i=0; i<strlen(line); ++i)
    {

       /*----------------------------------------------------------*/
       /* Is the block delimiter embedded in a text literal? If so */
       /* it can be safley ignored                                 */ 
       /*----------------------------------------------------------*/

       if(line[i] == '"')
       {  if(in_txt_literal == FALSE)
             in_txt_literal = TRUE;
          else
             in_txt_literal = FALSE;
       }


       /*----------------------------------------------------------*/
       /* We can also ignore block delimiters embedded in comments */
       /*----------------------------------------------------------*/

       if((line[i] == '/' && line[i+1] == '*')    ||
          (line[i] == '/' && line[i+1] == '/')     )
       {  if(in_comment == FALSE)
             in_comment = TRUE;
          else if(line[i] == '*' && line[i+1] == '/')
          {  i += 2;
             in_comment = FALSE;
          }
       }

       if(line[i] == ch && in_txt_literal == FALSE && in_comment == FALSE)
          ++bra_cnt;
    }

    return(bra_cnt);
}




/*-------------------------------------------------------------------------------------------
    Count the number of brackets '{' [bras] in a line ...
-------------------------------------------------------------------------------------------*/

_PRIVATE int nbras(char *line)

{   return(chcnt(line,'{'));
}




/*-------------------------------------------------------------------------------------------
    Count the number of brackets '}' [kets] in a line ...
-------------------------------------------------------------------------------------------*/

_PRIVATE int nkets(char *line)

{   return(chcnt(line,'}'));
}




/*------------------------------------------------------------------------------------------
    Look for the occurence of strinposg s2 within string s1 ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

{   int i,
        cmp_size,
        chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
    {  if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);
    }

    return(FALSE);
}




/*------------------------------------------------------------------------------------------
    Look for the occurence of strinposg s2 within string s1 returnig indices of start
    and end of s2 within s1 ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strinpos(long *s_pos, long *e_pos, char *s1, char *s2)

{   int i,
        cmp_size,
        chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
    {  if(strncmp(&s1[i],s2,cmp_size) == 0)
       {  if(s_pos != (long *)NULL)
             *s_pos = i;

          if(e_pos != (long *)NULL)
             *e_pos = i + strlen(s2);

          return(TRUE);
       }
    }

    if(s_pos != (long *)NULL)
       *s_pos = (-1);

    if(e_pos != (long *)NULL)
       *e_pos = (-1);

    return(FALSE);
}





/*------------------------------------------------------------------------------------------
    Reverse strip character from strinposg ...
------------------------------------------------------------------------------------------*/

_PRIVATE char *rstrpch(char *s, char c)

{   int i;

    for(i=strlen(s); i>0; --i)
       if(s[i] == c)
       {  s[i] = '\0';
          return(s);
       }

    return(s);
}





/*------------------------------------------------------------------------------------------
    Adjust function block count ...
------------------------------------------------------------------------------------------*/

_PRIVATE void braket(int direction)

{   if(direction == UP)
       ++r_cnt;
    else
       --r_cnt;
}
  



/*------------------------------------------------------------------------------------------
    Strip string s2 from string s1 ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strinstrip(char *s1, char *s2)

{   int start_index,
        end_index;

    char tmp_str[SSIZE] = "";

    if(sscanf(s1,"%s",s2) != 1)
       return(FALSE);

    (void)strinpos((long *)&start_index,(long *)&end_index,s1,s2);
    (void)strlcpy(tmp_str,SSIZE,(char *)&s1[end_index]);
    (void)strlcpy(s1,tmp_str,SSIZE);

    return(TRUE);
}




/*------------------------------------------------------------------------------------------
    Define a user defined mutex ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN defined(char *mutex)

{   int i;

    if(n_mutexes == MTABLE_SIZE)
    {  (void)fprintf(stderr,"pc2c: mutex table full (max %d user defined mutexes)\n",MTABLE_SIZE);
       (void)fflush(stderr);

       exit(255);
    }


    /*-------------------------------------------------------------------*/
    /* Search list of defined mutexes to see if current mutex is defined */
    /*-------------------------------------------------------------------*/

    for(i=0; i<n_mutexes; ++i)
    {  if(strcmp(defined_mutex[i],mutex) == 0)
          return(FALSE);
    }


    /*----------------------------------------------*/
    /* Try to find a space for new mutex definition */
    /*----------------------------------------------*/

    for(i=0; i<n_mutexes; ++i)
    {  if(strcmp(defined_mutex[i],"none") == 0)
       {  (void)strlcpy(defined_mutex[i],mutex,SSIZE);
          return(TRUE);
       }
    }


    /*-------------------------------------*/
    /* Make space for new mutex definition */
    /*-------------------------------------*/

    (void)strlcpy(defined_mutex[n_mutexes++],mutex,SSIZE);
    return(TRUE);
}




/*------------------------------------------------------------------------------------------
    Undefine a user defined mutex ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN undefine(char *mutex)

{   int i;

    for(i=0; i<n_mutexes; ++i)
    {  if(strcmp(defined_mutex[i],mutex) == 0)
       {  (void)strlcpy(defined_mutex[i],"none",SSIZE);
          return(TRUE);
       }
    }

    return(FALSE);
}




/*------------------------------------------------------------------------------------------
    Initialise mutex table ...
------------------------------------------------------------------------------------------*/

_PRIVATE void init_mtable(void)

{   int i;

    for(i=0; i<MTABLE_SIZE; ++i)
       (void)strlcpy(defined_mutex[i],"none",SSIZE);
}




/*------------------------------------------------------------------------------------------
    Skip text which has already been substituted ...
------------------------------------------------------------------------------------------*/

_PRIVATE void skip_pre_substituted_text(char *line)

{   if(strin(line,">>>>") == TRUE)
    {  (void)xfputs(line,stdout);
       (void)fflush(stdout);


       /*-----------------------------------------------*/
       /* Adjust any counts which have been incremented */
       /*-----------------------------------------------*/

       --f_orifice_cnt;
       do {    (void)fgets(line,255,stdin);
               ++l_cnt;

               n_bras = nbras(line);
               n_kets = nkets(line);


               /*-------------------------------------------*/
               /* Check for bra-ket (function block) errors */
               /*-------------------------------------------*/

               if((n_bras >  0 && n_kets >  0)    ||
                  n_bras  >  1                    ||
                  n_kets  >  1                     )
               {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
                  (void)fflush(stderr);

                  exit(255);
               }


               /*-----------------------------*/
               /* Adjust function block count */
               /*-----------------------------*/

               if(n_bras == 1)
                  braket(UP);
               else if(n_kets == 1)
               {  braket(DOWN); 
                  if(r_cnt == 0)
                     in_function_body = FALSE;
               }
 
               (void)xfputs(line,stdout);
               (void)fflush(stdout);
          } while(strin(line,"<<<<") == FALSE);

          (void)fgets(line,255,stdin);
          ++l_cnt;

    }
}




/*------------------------------------------------------------------------------------------
    Skip text which has contains comments ...
------------------------------------------------------------------------------------------*/

_PRIVATE void skip_comment(char *line)

{   long c_s_pos,
         b_s_pos;

    char tmp_str[SSIZE] = "";


    /*----------------------------------------*/
    /* Check that we have a skippable comment */
    /*----------------------------------------*/

    one_line_comment = FALSE;
    (void)strlcpy(tmp_str,"",SSIZE);
    (void)sscanf(line,"%s",tmp_str);

    if(strncmp(tmp_str,"/*",2) == 0)
    {  (void)xfputs(line,stdout);
       (void)fflush(stdout);


       /*-------------------------*/
       /* Is comment a one-liner? */
       /*-------------------------*/

       if(strin(line,"*/") == FALSE)
       {  do {    (void)fgets(line,255,stdin);
                  ++l_cnt;

                  (void)xfputs(line,stdout);
                  (void)fflush(stdout);
              } while(strin(line,"*/") == FALSE);
       }
       else
          one_line_comment = TRUE;
    
       (void)fgets(line,255,stdin);
       ++l_cnt;
    }
}




/*------------------------------------------------------------------------------------------
    Extended fputs function ...
------------------------------------------------------------------------------------------*/

_PRIVATE void xfputs(char *line, FILE *stream)

{   if(ch_pos(line,'\n') == (-1))
       strlcat(line,"\n",SSIZE);

    (void)fputs(line,stream);
}




/*------------------------------------------------------------------------------------------
    Strip characters from string ...
------------------------------------------------------------------------------------------*/

_PRIVATE void strpach(char *s, char ch)

{   int i,
        cnt = 0;

    for(i=0; i<strlen(s); ++i)
    {  if(s[i] == ch)
          s[i] = ' ';
    }
}




/*------------------------------------------------------------------------------------------
    Check that if-else statements conform to PUPS-C syntax ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN check_if_else(char *line)

{   int i,
        cnt,
        s_cnt,
        e_cnt;

    char line_buf[4096];

    _BOOLEAN looper     = FALSE,
             in_comment = FALSE;

    _IMMORTAL _BOOLEAN in_if_else = FALSE;

    if(in_function_body == FALSE)
       return(TRUE);


    /*---------------------------------------------*/
    /* Strip any embedded comments out of the line */
    /*---------------------------------------------*/

    strip_line(line,line_buf);


    /*-------------------------------------------------------*/
    /* We now have a line stripped of all embedded comments  */
    /* if it begins with "if" or "else" it must end with ")" */
    /* to conform to PUPS-C syntax                           */
    /*-------------------------------------------------------*/

    s_cnt = 0;
    while(line_buf[s_cnt] == ' ')
          ++s_cnt;


    /*----------------------------------*/
    /* Check for and if, else construct */
    /*----------------------------------*/

    if(strncmp(&line_buf[s_cnt],"if",2)   == 0    || 
       strncmp(&line_buf[s_cnt],"else",4) == 0    ||
       in_if_else                         == TRUE  ) 
    {

      /*---------------------------------------*/
      /* And make sure that it is legal PUPS-C */
      /*---------------------------------------*/

       e_cnt = strlen(line_buf); 
       while(line_buf[e_cnt] == ' ' || line_buf[e_cnt] == '\n' || line_buf[e_cnt] == '\0')
           --e_cnt;


       /*-------------------------------------*/
       /* Check "if" and "else" PUPS-C syntax */
       /*-------------------------------------*/

       if(strncmp(&line_buf[s_cnt],"if",2) == 0 || in_if_else == TRUE)
       {  if(line_buf[e_cnt] == ')')
          {  in_if_else = FALSE;
             return(TRUE);
          }
          else
             in_if_else = TRUE;
       }
       else if(strncmp(&line_buf[s_cnt],"else",4) == 0)
       {  if(line_buf[e_cnt] == 'e' || line_buf[e_cnt] == ')')
             return(TRUE);
          else
             in_if_else = TRUE;
       }
   }


   /*---------------------------------------*/
   /* Statement is not an if-else construct */
   /*---------------------------------------*/

   return(TRUE);
}




/*------------------------------------------------------------------------------------------
    Check for empty lines in PUPS-C text ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN empty_line(char *line)

{   int  cnt = 0;
    char first_item[SSIZE] = "";


    /*------------------------------------------------------------*/
    /* From the viewpoint of pc2c macro directive are empty lines */
    /*------------------------------------------------------------*/

    if(sscanf(line,"%s",first_item) == 1)
    {  if(first_item[0] == '#')
          return(TRUE);
    }


    /*----------------------------*/   
    /* Look for "real" empty line */ 
    /*----------------------------*/   

    while(line[cnt] == ' ' || line[cnt] == '\n' || line[cnt] == '\0')
    {    if(line[cnt] == '\0')
         {  (void)xfputs(line,stdout);
            (void)fflush(stdout);

            return(TRUE);
         }

         ++cnt;
    }

    return(FALSE);
}



/*------------------------------------------------------------------------------------------
    Check that string is composed of upper case characters (and digits) only ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN is_upper(char *s)

{   int i,
        cnt = 0;

    for(i=0; i<strlen(s); ++i)
    {  if(isalpha(s[i]) == 1 && isupper(s[i]) == 0)
          return(FALSE);
    }

    return(TRUE);
}




/*------------------------------------------------------------------------------------------
    Parse PUPS-C macro. n_args is the number of arguments expected, arglist is a list of
    the parsed arguments, and line is the input line containing the macro. If we get
    any errors FALSE is returned, otherwise TRUE is returned ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN parse_pups_c_macro_function(char *line, int n_args, char arglist[32][SSIZE])

{   int i,
        s_index,
        e_index;

    _BOOLEAN looper        = FALSE;

    char line_buf[SSIZE]   = "",
         next_item[SSIZE]  = "",
         arg_str[SSIZE]    = "",
         macro_name[SSIZE] = "";


    /*----------------------------------------------------------------------*/
    /* Is this a macro function? By convention, a PUPS-C macro has the form */
    /* _<NAME>(args) where <NAME> MUST be in upper case.                    */
    /*----------------------------------------------------------------------*/

    (void)strlcpy(line_buf,line,SSIZE);
    do {   looper = strinstrip(line_buf,next_item);

           if(looper == TRUE)
           {  if(next_item[0] == '_'                         &&
                 (s_index = ch_pos(next_item,'(')) != (-1)   &&
                 (e_index = ch_pos(next_item,')')) != (-1)    )
              {

                 /*------------------------------------------------*/
                 /* We have a macro definition - extract arguments */
                 /* Check for syntax errors                        */
                 /* Cannot have comments on macro line             */            
                 /*------------------------------------------------*/

                 if(strin(line,"/*") == TRUE || strin(line,"*/") == TRUE || strin(line,"//") == TRUE)
                    return(FALSE);


                 /*--------------------------------------------------------------------*/
                 /* Cannot assign RHS initialiser in PUPS-C macro function definitions */
                 /*--------------------------------------------------------------------*/

                 if(strin(line,"=") == TRUE)
                    return(FALSE);

                 next_item[e_index] = '\0'; 
                 (void)strlcpy(arg_str,&next_item[s_index+1],SSIZE);
                 ch_rep(arg_str,',',' ');

                 next_item[s_index]  = '\0';
                 (void)strlcpy(macro_name,&next_item[1],SSIZE);


                 /*--------------------------------*/
                 /* Check macro name is upper case */
                 /*--------------------------------*/

                 if(is_upper(macro_name) == FALSE)
                    return(FALSE);

                 for(i=0; i<n_args; ++i)
                 {

                    if(strinstrip(arg_str,arglist[i]) == FALSE)
                    {

                       /*------------------------------------------------*/
                       /* Macro function found - does not match template */
                       /*------------------------------------------------*/

                       return(FALSE);
                    }
                 }


                 /*-----------------------------------------*/
                 /* Macro function found - matches template */
                 /*-----------------------------------------*/

                 return(TRUE);
              }
           }
        } while(looper == TRUE);


    /*--------------------------------------------------------*/
    /* This line does not contain a macro function definition */
    /*--------------------------------------------------------*/

    return(FALSE);
} 




/*------------------------------------------------------------------------------------------
    Replace character c_1 in string s by character c_2 ...
------------------------------------------------------------------------------------------*/

_PRIVATE void ch_rep(char *s, char c_1, char c_2)

{   int i;

    for(i=0; i<strlen(s); ++i)
       if(s[i] == c_1)
          s[i] = c_2;
}





/*------------------------------------------------------------------------------------------
    Translate a thread key create definition ...
------------------------------------------------------------------------------------------*/

_PRIVATE void tkey_transform(char *line)

{   char tkey[SSIZE]        = "",
         arglist[32][SSIZE] = { "" };



    /*----------------------------------------------------------*/
    /* Define a key variable in which per-thread data is stored */
    /*----------------------------------------------------------*/

    if(in_function_body == FALSE || parse_pups_c_macro_function(line,1,arglist) == FALSE)
    {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }


    /*-----------------------------------------------------------*/
    /* In the case of _TKEY_CREATE we must transform the macro   */
    /* into a line of the form _TKEY_CREATE(type,var,keyval);    */
    /* Better not to complicate things by permitting the user to */
    /* initialise the key here                                   */
    /*-----------------------------------------------------------*/

    (void)snprintf(tkey,SSIZE,"%s_tkey",arglist[0]);

    (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n");
    (void)fprintf(stdout,"/* Transformed automatically by pc2c                                 */\n");
    (void)fprintf(stdout,"    _TKEY(%s)\n",tkey);
    (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
    (void)fflush(stdout);

    (void)fprintf(stderr,"%d: creating TSD key for \"%s\" in PUPS function \"%s\"\n",l_cnt,arglist[0],fname);
    (void)fflush(stderr);

    ++tsd_cnt;
}





/*------------------------------------------------------------------------------------------
    Translate a thread key delete definition ...
------------------------------------------------------------------------------------------*/

_PRIVATE void tkey_bind_transform(char *line)

{   char tkey[SSIZE]        = "",
         arglist[32][SSIZE] = { "" };


    /*--------------------------------------------------*/
    /* Destroy a key in which per thread data is stored */
    /*--------------------------------------------------*/

    if(in_function_body == FALSE || parse_pups_c_macro_function(line,1,arglist) == FALSE)
    {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }

    (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n");
    (void)fprintf(stdout,"/* Transformed automatically by pc2c                                 */\n");
    (void)snprintf(tkey,SSIZE,"%s_tkey",arglist[0]);
    (void)fprintf(stdout,"    _TKEY_bind(%s,%s)\n",arglist[0],tkey);
    (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
    (void)fflush(stdout);

    (void)fprintf(stderr,"%d: binding \"%s\" to TSD key in PUPS function \"%s\"\n",l_cnt,arglist[0],fname);
    (void)fflush(stderr);
}






/*------------------------------------------------------------------------------------------
    Translate a thread key set value definition ...
------------------------------------------------------------------------------------------*/

_PRIVATE void tkey_free_transform(char *line)

{   char tkey[SSIZE]        = "",
         arglist[32][SSIZE] = { "" };

    if(in_function_body == FALSE || parse_pups_c_macro_function(line,1,arglist) == FALSE)
    {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }

    (void)fprintf(stdout,"\n\n/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/\n");
    (void)fprintf(stdout,"/* Transformed automatically by pc2c                                 */\n");
    (void)snprintf(tkey,SSIZE,"%s_tkey",arglist[0]);
    (void)fprintf(stdout,"    _TKEY_FREE(%s)\n",tkey);
    (void)fprintf(stdout,"/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/\n\n");
    (void)fflush(stdout);

    (void)fprintf(stderr,"%d: freeing \"%s\" from TSD key in PUPS function \"%s\"\n",l_cnt,arglist[0],fname);
    (void)fflush(stderr);
}





/*------------------------------------------------------------------------------------------
    Insert an _FMUTEX_DEFINE definition ...
------------------------------------------------------------------------------------------*/

_PRIVATE void use_fmutex_define(char *line)

{   char arglist[32][SSIZE] = { "" };

    if(in_function_body == TRUE || parse_pups_c_macro_function(line,1,arglist) == FALSE)
    {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }


    /*----------------------------------------------------------*/
    /* Keep a copy of the current mutex in case we are asked to */
    /* process a macro conditional funtion body                 */
    /*----------------------------------------------------------*/

    (void)strlcpy(mutex,arglist[0],SSIZE);
    (void)strlcpy(current_mutex,arglist[0],SSIZE);

    if(defined(mutex) == TRUE)
       (void)fprintf(stderr,"%d: current mutex is \"%s\" (defining)\n",l_cnt,mutex);
    else
       (void)fprintf(stderr,"%d: current mutex is \"%s\"\n",l_cnt,mutex);

    (void)fflush(stderr);

    (void)fputs(line,stdout);
    (void)fflush(stdout);
}





/*------------------------------------------------------------------------------------------
    Insert an _DEFAULT_FMUTEX_DEFINE definition ...
------------------------------------------------------------------------------------------*/

_PRIVATE void default_fmutex_define(char *line)

{   char arglist[32][SSIZE] = { "" };

    if(in_function_body == TRUE || parse_pups_c_macro_function(line,1,arglist) == FALSE)
    {  (void)fprintf(stderr,"%d: PUPS-C syntax error: %s\n",l_cnt,rstrpch(line,'\n'));
       (void)fflush(stderr);

       exit(255);
    }

    if(strcmp(arglist[0],"inbuilt") == 0)
       (void)strlcpy(default_mutex,"default",SSIZE);
    else
       (void)strlcpy(default_mutex,arglist[0],SSIZE);

    (void)fprintf(stderr,"%d: defining default mutex \"%s\"\n",l_cnt,default_mutex);
    (void)fflush(stderr);

    (void)fputs(line,stdout);
    (void)fflush(stdout);
}





/*------------------------------------------------------------------------------------------
    Strip comments from line ...
------------------------------------------------------------------------------------------*/

_PRIVATE void strip_line(char *line, char *stripped_line)

{   int      i;
    _BOOLEAN in_comment = FALSE;

    for(i=0; i<strlen(line); ++i)
    {  if(line[i] == '/' && line[i+1] == '*')
          in_comment = TRUE;
       else if(line[i] == '/' && line[i+1] == '/') 
       {  stripped_line[i] = '\0';
          return;
       }
       else if(line[i] == '*' && line[i+1] == '/')
       {  in_comment = FALSE;
          line[i] = ' ';
          line[i+1] = ' ';
       }

       if(in_comment == TRUE)
          stripped_line[i] = ' ';
       else
          stripped_line[i] = line[i];
    }

    stripped_line[i] = '\0';
}




/*------------------------------------------------------------------------------------------
    Check that modifiers (_ROOTTHREAD, _DLL_ORIFICE and _THREADSAFE) are correctly
    used ...
------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN check_function_head_modifiers(char *line)

{  char line_buf[4096] = "";

   strip_line(line,line_buf);

   if(strin(line_buf,"_ROOTTHREAD")  == TRUE    ||
      strin(line_buf,"_DLL_ORIFICE") == TRUE    ||
      strin(line_buf,"_THREADSAFE")  == TRUE     )
{


#ifdef DEBUG
(void)fprintf(stderr,"LINE CHECK %s\n",line);
void)fflush(stderr);
#endif /* DEBUG */

      return(FALSE);
}
   return(TRUE);
}

