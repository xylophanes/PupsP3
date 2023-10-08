/*-----------------------------------------------------------------------------
    Purpose: Cantor cellular database core library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.00 
    Dated:   4th January 2022
    E-mail:  mao@@tumblingdice.co.uk
-----------------------------------------------------------------------------*/


#include <stdio.h>
#include <me.h>
#include <utils.h>
#include <errno.h>
#include <vstamp.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef PERSISTENT_HEAP_SUPPORT
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#include <phmalloc.h>
#include <sys/stat.h>
#include <sys/types.h>


#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */

#undef   __NOT_LIB_SOURCE__
#include <cantor.h>
#define  __NOT_LIB_SOURCE__


/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void cantorlib_slot(int level)
{   (void)fprintf(stderr,"lib cantorlib %s: [ANSI C]\n",CANTOR_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1994-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 cellular database core library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*----------------------------------------------------------*/
/* Segment identification for cantor (set analysis) library */
/*----------------------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = cantorlib_slot;
#endif /* SLOT */




/*----------------------------------------------------------*/
/* Prototypes of functions which are private to this module */
/*----------------------------------------------------------*/

// Clear link
_PROTOTYPE _PRIVATE link_type *_clear_link(link_type *, int);

//Clear linklist
_PROTOTYPE _PRIVATE link_type **_clear_linklist(link_type **, int, int);

// Clear object
_PROTOTYPE _PRIVATE object_type *_clear_object(object_type *, int);

// Clear object
_PROTOTYPE _PRIVATE object_type **_clear_objectlist(object_type **, int, int);

// Clear node
_PROTOTYPE _PRIVATE node_type *_clear_node(node_type *, int);

// Clear nodelist
_PROTOTYPE _PRIVATE nodelist_type *_clear_nodelist(nodelist_type *, int);

// Add node to nodelist
_PROTOTYPE _PRIVATE int _add_node_to_nodelist(nodelist_type *, node_type *, int, char *);

// Remove node from nodelist
_PROTOTYPE _PRIVATE int _remove_node_from_nodelist(nodelist_type *, char *, int);

// Add object to node
_PROTOTYPE _PRIVATE int _add_object_to_node(node_type *, object_type *, int, char *);

// Remove object from node
_PROTOTYPE _PRIVATE int _remove_object_from_node(node_type *, char *, int);

// Add link to node
_PROTOTYPE _PRIVATE int _add_link_to_node(node_type *, link_type *, int, char *);

// Remove link from node
_PROTOTYPE _PRIVATE int _remove_link_from_node(node_type *, char *, int);

// Add node to link (routelist)
_PROTOTYPE _PRIVATE int _add_node_to_link(link_type *, node_type *, int, char *);




#ifdef PTHREAD_SUPPORT
/*------------------------------------------------------------------------------
    Initialises mutexes ...
------------------------------------------------------------------------------*/


/*------------*/
/* Node mutex */
/*------------*/

_PUBLIC void cantor_init_node_mutex(node_type *node)

{   pthread_mutexattr_t attr;

    if(node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)pthread_mutexattr_init(&attr);
    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void)pthread_mutex_init(&node->mutex, &attr);

    pups_set_errno(OK);
}


/*--------------*/
/* Object mutex */
/*--------------*/

_PUBLIC void cantor_init_object_mutex(object_type *object)

{    pthread_mutexattr_t attr;

    if(object == (object_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)pthread_mutexattr_init(&attr);
    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void)pthread_mutex_init(&object->mutex, &attr);

    pups_set_errno(OK);
}


/*------------*/
/* Link mutex */
/*------------*/

_PUBLIC void cantor_init_link_mutex(link_type *link)

{   pthread_mutexattr_t attr;

    if(link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)pthread_mutexattr_init(&attr);
    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void)pthread_mutex_init(&link->mutex, &attr);

    pups_set_errno(OK);
}


/*----------------*/
/* Nodelist mutex */
/*----------------*/

_PUBLIC void cantor_init_nodelist_mutex(nodelist_type *nodelist)

{   pthread_mutexattr_t attr;

    if(nodelist == (nodelist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)pthread_mutexattr_init(&attr);
    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    (void)pthread_mutex_init(&nodelist->mutex, &attr);

    pups_set_errno(OK);
}
#endif /* PTHREAD_SUPPORT */




/*------------------------------------------------------------------------------
    Get the index of node (given its name) ...
------------------------------------------------------------------------------*/

_PUBLIC int cantor_get_node_index_from_name(nodelist_type *nodelist, char *name)

{   int i;

    if(nodelist == (nodelist_type *)NULL || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<nodelist->node_alloc; ++i)
    { 

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_lock(&nodelist->nodes[i]->mutex);
       #endif /* PTHREAD_SUPPORT */
 
       if(nodelist->nodes[i] != (node_type *)NULL                                          &&
          (nodelist->nodes[i] = (node_type *)cantor_pointer_live(nodelist->nodes[i],TRUE)) &&
          strcmp(nodelist->nodes[i]->name,name) == 0                                        )
       {  

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&nodelist->nodes[i]->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&nodelist->nodes[i]->mutex);
       #endif /* PTHREAD_SUPPORT */

    }

    pups_set_errno(ESRCH);
    return(-1);
}




/*------------------------------------------------------------------------------
    Clear a node ...
------------------------------------------------------------------------------*/


/*---------------------------*/
/* Target node on local heap */
/*---------------------------*/

_PUBLIC node_type *cantor_clear_node(node_type *node)

{   return(_clear_node(node,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*--------------------------------*/
/* Target node on persistent heap */
/*--------------------------------*/

_PUBLIC node_type *cantor_phclear_node(node_type *node, int hdes)

{   return(_clear_node(node,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE node_type *_clear_node(node_type *node, int hdes)

{   if(node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((node_type *)NULL);
    } 

    #ifdef PTHREAD_DUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAS_DUPPORT */

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(hdes > 0 && hdes < appl_max_pheaps)
    {  (void)cantor_phclear_objectlist(node->objects,hdes,node->object_alloc);
       (void)cantor_phclear_linklist(node->links,hdes,node->link_alloc);
    }
    else if(hdes > 0)
    { 

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&node->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((node_type *)NULL);
    }
    else
    #endif /* PERSISTENT_HEAP_SUPPORT */
    {  (void)cantor_clear_objectlist(node->objects,node->object_alloc);
       (void)cantor_clear_linklist(node->links,node->link_alloc);
    }

    (void)strlcpy(node->name,"notset",SSIZE);
    (void)strlcpy(node->type,"notset",SSIZE);

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(hdes > 0 && hdes < appl_max_pheaps)
    {  (void)strlcpy(node->link_name,"notset",  SSIZE);
       (void)strlcpy(node->object_name,"notset",SSIZE); 
       node->links        = (link_type **)  phfree(hdes,(void *)node->links);
       node->objects      = (object_type **)phfree(hdes,(void *)node->objects);
    }
    else if(hdes > 0)
    { 

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&node->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((node_type *)NULL);
    }
    else
    #endif /* PERSISTENT_HEAP_SUPPORT */
    {  node->links        = (link_type **)  pups_free((void *)node->links);
       node->objects      = (object_type **)pups_free((void *)node->objects);
    }

    node->link_alloc   = 0;
    node->link_cnt     = 0;
    node->object_alloc = 0;
    node->object_cnt   = 0;

    #ifdef PTHREAD_DUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return((node_type *)NULL);
}




/*------------------------------------------------------------------------------
    Clear a nodelist ...
------------------------------------------------------------------------------*/


/*-------------------------------*/
/* Target nodelist on local heap */
/*-------------------------------*/

_PUBLIC nodelist_type *cantor_clear_nodelist(nodelist_type *nodelist)
{   return(_clear_nodelist(nodelist,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*------------------------------------*/
/* Target nodelist on persistent heap */
/*------------------------------------*/

_PUBLIC nodelist_type *cantor_phclear_nodelist(nodelist_type *nodelist, int hdes)
{   return(_clear_nodelist(nodelist,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE nodelist_type *_clear_nodelist(nodelist_type *nodelist, int hdes)

{   int i;

    if(nodelist == (nodelist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((nodelist_type *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<nodelist->node_alloc; ++i)
    {  if(nodelist->nodes[i] != (node_type *)NULL && (nodelist->nodes[i] = (node_type *)cantor_pointer_live(nodelist->nodes[i],TRUE)))

          #ifdef PERSISTENT_HEAP_SUPPORT
          if(hdes > 0 && hdes < appl_max_pheaps)
             (void)_clear_node(nodelist->nodes[i],hdes);
          else if(hdes > 0)
          { 

             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&nodelist->mutex);
             #endif /* PTHREAD_SUPPORT */

            pups_set_errno(EINVAL);
            return((nodelist_type *)NULL);
          }
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
             (void)_clear_node(nodelist->nodes[i],(-1));
    }

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(hdes > 0 && hdes < appl_max_pheaps)
    {  (void)strlcpy(nodelist->node_name,"notset",SSIZE);
       (void)phfree(hdes,nodelist->nodes);
    }
    else if(hdes > 0)
    { 

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&nodelist->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((nodelist_type *)NULL);
    }
    else
    #endif /* PERSISTENT_HEAP_SUPPORT */
       (void)pups_free((void *)nodelist->nodes);

    (void)strlcpy(nodelist->name,"notset",SSIZE);
    (void)strlcpy(nodelist->type,"notset",SSIZE);

    nodelist->node_alloc = 0;
    nodelist->node_cnt   = 0;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return((nodelist_type *)NULL);
}



/*------------------------------------------------------------------------------
    Add node to nodelist ...
------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Target nodelist (to add node to) on local heap */
/*------------------------------------------------*/

_PUBLIC int cantor_add_node_to_nodelist(nodelist_type *nodelist, node_type *node)
{   return(_add_node_to_nodelist(nodelist,node,(-1),(char *)NULL));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*----------------------------------------------------*/
/* Target nodelist (to add node to) on peristent heap */
/*----------------------------------------------------*/

_PUBLIC int cantor_phadd_node_to_nodelist(nodelist_type *nodelist, node_type *node, int hdes, char *name)
{   return(_add_node_to_nodelist(nodelist,node,hdes,name));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _add_node_to_nodelist(nodelist_type *nodelist, node_type *node, int hdes, char *name)

{   int node_index;

    if(nodelist == (nodelist_type *)NULL || node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(nodelist->nodes == (node_type **)NULL)
    {  nodelist->node_alloc = ALLOC_QUANTUM;



       #ifdef PERSISTENT_HEAP_SUPPORT
       /*-----------------------------------*/
       /* Is this node on a pesistent heap? */
       /*-----------------------------------*/

       if(hdes > 0 && hdes < appl_max_pheaps) 
       {  (void)snprintf(nodelist->node_name,SSIZE,"%s:nlist",name);
          nodelist->nodes = (node_type **)phcalloc(hdes,ALLOC_QUANTUM,sizeof(node_type *),name);
          if(nodelist->nodes == (node_type **)NULL)
             pups_error("[_add_node_to_nodelist] failed to create node list (on persistent heap)");
       }
       else if(hdes > 0)
       { 

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&nodelist->mutex);
          #endif /* PTHREAD_SUPPORT */

           pups_set_errno(EINVAL);
           return(-1);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */

       {
          nodelist->nodes = (node_type **)pups_calloc(ALLOC_QUANTUM,sizeof(node_type *));
          if(nodelist->nodes == (node_type **)NULL)
             pups_error("[_add_node_to_nodelist] failed to create node list (on local heap/cache)");
       }

      
       nodelist->nodes[nodelist->node_cnt] = node;
       node_index                          = 0;
       ++nodelist->node_cnt;
    }
    else if(nodelist->node_alloc == nodelist->node_cnt)
    {  nodelist->node_alloc                += ALLOC_QUANTUM;

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes > 0 && hdes < appl_max_pheaps)
       { 

          /*-----------------------------------------------------------------------------------*/
          /* We need to have locked the entire persistent heap before we do reallocation or we */
          /* may get race conditions                                                           */
          /*-----------------------------------------------------------------------------------*/

          nodelist->nodes = (node_type **)phrealloc(hdes,(void *)nodelist->nodes,nodelist->node_alloc*sizeof(node_type *),nodelist->node_name);
          if(nodelist->nodes == (node_type **)NULL)
             pups_error("[_add_node_to_nodelist] failed to extend node list (on persistent heap)");
       }
       else if(hdes > 0)
       { 
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&nodelist->mutex);
          #endif /* PTHREAD_SUPPORT */

           pups_set_errno(EINVAL);
           return(-1);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {
          nodelist->nodes = (node_type **)pups_realloc((void *)nodelist->nodes,nodelist->node_alloc*sizeof(node_type *));
          if(nodelist->nodes == (node_type **)NULL)
             pups_error("[_add_node_to_nodelist] failed to extend node list (on local heap/cache)");
       }

       nodelist->nodes[nodelist->node_cnt] = node;
       node_index                          = nodelist->node_cnt;
       ++nodelist->node_cnt;
    }
    else
    {  int i;


       /*---------------------------------*/
       /* Search nodelist for a free slot */
       /*---------------------------------*/

       for(i=0; i<nodelist->node_cnt; ++i)
       {  if(nodelist->nodes[i] == (node_type *)NULL)
             break;
       }

       nodelist->nodes[i] = node;
       node_index         = i;

       ++nodelist->node_cnt;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(node_index);
}




/*------------------------------------------------------------------------------------------
    Remove node from nodelist ...
------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------*/
/* Target nodelist (to remove node from) is on local heap */
/*--------------------------------------------------------*/

_PUBLIC int cantor_remove_node_from_nodelist(nodelist_type *nodelist, char *name)
{   return(_remove_node_from_nodelist(nodelist,name,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT 
/*-------------------------------------------------------------*/
/* Target nodelist (to remove node from) is on persistent heap */
/*-------------------------------------------------------------*/

_PUBLIC int cantor_phremove_node_from_nodelist(nodelist_type *nodelist, char *name, int hdes)
{   return(_remove_node_from_nodelist(nodelist,name,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _remove_node_from_nodelist(nodelist_type *nodelist, char *name, int hdes)

{   int i;

    if(nodelist == (nodelist_type *)NULL || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------------*/
    /* Search nodelist for node name */
    /*-------------------------------*/

    for(i=0; i<nodelist->node_alloc; ++i)
    {  if(nodelist->nodes[i] != (node_type *)NULL                                                 &&
          (nodelist->nodes[i] = (node_type *)cantor_pointer_live(nodelist->nodes[i]->name,TRUE))  &&
          strcmp(nodelist->nodes[i]->name,name) == 0                                               )
       {
          #ifdef PERSISTENT_HEAP_SUPPORT
          if(hdes > 0 && hdes < appl_max_pheaps) 
          {  _clear_node(nodelist->nodes[i],hdes);
             nodelist->nodes[i] = (node_type *)phfree(hdes,(void *)nodelist->nodes[i]);
          }
          else if(hdes > 0)
          { 
             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&nodelist->mutex);
             #endif /* PTHREAD_SUPPORT */

             pups_set_errno(EINVAL);
             return(-1);
          }
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
          {  _clear_node(nodelist->nodes[i],(-1));
             nodelist->nodes[i] = (node_type *)pups_free((void *)nodelist->nodes[i]);
          }

          --nodelist->node_cnt;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&nodelist->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(0);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(EEXIST);
    return(-1);
}




/*---------------------------------------------------------------------------------
    Set nodelist name ...
---------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_nodelist_name(nodelist_type *nodelist, char *nodelist_name)

{   if(nodelist == (nodelist_type *)NULL || nodelist_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(nodelist->name,nodelist_name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*---------------------------------------------------------------------------------
    Set nodelist type ...
---------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_nodelist_type(nodelist_type *nodelist, char *type)

{   if(nodelist == (nodelist_type *)NULL || type == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(nodelist->type,type,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*---------------------------------------------------------------------------------
    Add an object to a node ...
---------------------------------------------------------------------------------*/


/*----------------------------------------------*/
/* Target node (to add object to) on local heap */
/*----------------------------------------------*/

_PUBLIC int cantor_add_object_to_node(node_type *node, object_type *object)
{   return(_add_object_to_node(node,object,(-1),(char *)NULL));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*---------------------------------------------------*/
/* Target node (to add object to) on persistent heap */
/*---------------------------------------------------*/

_PUBLIC int cantor_phadd_object_to_node(node_type *node, object_type *object, int hdes, char *name)
{   return(_add_object_to_node(node,object,hdes,name));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _add_object_to_node(node_type *node, object_type *object, int hdes, char *name)

{   int object_index = 0;

    if(node == (node_type *)NULL || object == (object_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(node->objects == (void *)NULL)
    {  node->object_alloc = ALLOC_QUANTUM;


       #ifdef PERSISTENT_HEAP_SUPPORT
       /*------------------------------------*/
       /* Is this node on a persistent heap? */
       /*------------------------------------*/

       if(hdes > 0 && hdes < appl_max_pheaps)
       {  char object_name[SSIZE] = "";

          (void)snprintf(node->object_name,SSIZE,"%s:olist",node->name);
          node->objects = (object_type **)phcalloc(hdes,ALLOC_QUANTUM,sizeof(object_type **),node->object_name);
          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to create node object list (on persistent heap)");
       }
       else if(hdes > 0)
       { 

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(EINVAL);
          return(-1);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */

       {  node->objects = (object_type **)pups_calloc(ALLOC_QUANTUM,sizeof(object_type **));
          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to create node object list (on local heap/cache)");
       }

       node->objects[node->object_cnt] = object;
       ++node->object_cnt;
    }
    else if(node->object_cnt == node->object_alloc)
    {  node->object_alloc += ALLOC_QUANTUM;

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes >= 0)
       {  node->objects = (object_type **)phrealloc(hdes,node->objects,node->object_alloc*sizeof(object_type **),node->object_name); 
          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to extend object list (on persistent heap)");
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {  node->objects = (object_type **)pups_realloc(node->objects,node->object_alloc*sizeof(object_type **)); 
          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to extend object list (on local heap/cache)");
       }

       node->objects[node->object_cnt] = object;
       ++node->object_cnt;
    }
    else
    {  int i;


       /*-----------------------------------------*/
       /* Search node object list for a free slot */ 
       /*-----------------------------------------*/

       for(i=0; i<node->object_cnt; ++i)
       {  if(node->objects[i] == (object_type *)NULL)
             break;
       }

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes > 0 && hdes < appl_max_pheaps)
       {  node->objects[i] = (object_type *)phmalloc(hdes,sizeof(object_type),name);

          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to extend object list (on persistent heap)");
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {
          node->objects[i] = (object_type *)pups_malloc(sizeof(object_type));

          if(node->objects == (object_type **)NULL)
             pups_error("[_add_object_to_node] failed to extend object list (on local heap/cache)");
       }

       node->objects[i] = object;
       object_index     = i;
       ++node->object_cnt;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(object_index);
}




/*-----------------------------------------------------------------------------------
    Remove an object from a node ...
-----------------------------------------------------------------------------------*/


/*----------------------------------------------------*/
/* Target node (containing object) node on local heap */
/*----------------------------------------------------*/

_PUBLIC int cantor_remove_object_from_node(node_type *node, char *object_tag)
{   return(_remove_object_from_node(node,object_tag,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*---------------------------------------------------------*/
/* Target node (containing object) node on persistent heap */
/*---------------------------------------------------------*/

_PUBLIC int cantor_phremove_object_from_node(node_type *node, char *object_tag, int hdes)
{   return(_remove_object_from_node(node,object_tag,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _remove_object_from_node(node_type *node, char *object_tag, int hdes)

{   int i;

    if(node == (node_type *)NULL || object_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    for(i=0; i<node->object_alloc; ++i)
    {  if(node->objects[i] != (object_type *)NULL && strcmp(node->objects[i]->tag,object_tag) == 0)
       {  

          #ifdef PERSISTENT_HEAP_SUPPORT
          /*-------------------------------------------------------------*/
          /* If an object is on a persistent heap its name is its handle */
          /*-------------------------------------------------------------*/

          if(hdes > 0 && hdes < appl_max_pheaps)
          { cantor_clear_object(node->objects[i]);
             node->objects[i] = (object_type *)phfree(hdes,node->objects[i]); 
          }
          else if(hdes > 0)
          { 
              #ifdef PTHREAD_SUPPORT
              (void)pthread_mutex_unlock(&node->mutex);
              #endif /* PTHREAD_SUPPORT */

              pups_set_errno(EINVAL);
              return((link_type *)NULL);
          }
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
          {  cantor_clear_object(node->objects[i]);
             node->objects[i] = (object_type *)pups_free(node->objects[i]); 
          }

          --node->object_cnt;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(0);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*--------------------------------------------------------------------------------------
    Check to see if node name is unique (within given nodelist) ...
--------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN cantor_node_name_unique(nodelist_type *nodelist, char *name)

{   int i;

    if(nodelist == (nodelist_type *)NULL || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<nodelist->node_alloc; ++i)
    {  if(nodelist->nodes[i]  != (node_type *)NULL                                           &&
          (nodelist->nodes[i] != (node_type *)cantor_pointer_live(nodelist->nodes[i],TRUE))  &&
          strcmp(nodelist->nodes[i]->name,name) == 0                                          )
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&nodelist->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(TRUE);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*--------------------------------------------------------------------------------------
    Check to see if an object tag associated with given node is unique ...
--------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN cantor_object_tag_unique(node_type *node, char *object_tag)

{   int i;

    if(node == (node_type *)NULL || object_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<node->object_alloc; ++i)
    {  if(node->objects[i]  != (object_type *)NULL                                         && 
          (node->objects[i] != (object_type *)cantor_pointer_live(node->objects[i],TRUE))  &&
          strcmp(node->objects[i]->tag,object_tag) == 0                                     )
       {
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(TRUE);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(FALSE);
}




/*---------------------------------------------------------------------------------------
    Get index of an object (given its name) ...
---------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN cantor_get_object_index_from_tag(node_type *node, char *object_tag)

{   int i;

    if(node == (node_type *)NULL || object_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<node->object_alloc; ++i)
    {  if(node->objects[i] != (object_type *)NULL                                         &&
          (node->objects[i] = (object_type *)cantor_pointer_live(node->objects[i],TRUE))  &&
          strcmp(node->objects[i]->tag,object_tag) == 0                                    )
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*---------------------------------------------------------------------------------------
    Get tag of an object (given its index) ...
---------------------------------------------------------------------------------------*/

_PUBLIC int cantor_get_object_tag_from_index(char *tag, node_type *node, int object_index)

{   if(node == (node_type *)NULL || tag == (char *)NULL ||  object_index < 0 || object_index > node->object_alloc)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(tag,node->objects[object_index]->tag,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------------------------------------------------------------
    Associate a link with a node ...
---------------------------------------------------------------------------------------*/
/*--------------------*/
/* Link on local heap */
/*--------------------*/

_PUBLIC int cantor_add_link_to_node(node_type *node, link_type *link)
{   return(_add_link_to_node(node,link,(-1),(char *)NULL));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------*/
/* Link on persistent heap */
/*-------------------------*/

_PUBLIC int cantor_phadd_link_to_node(node_type *node, link_type *link, int hdes, char *name)
{   return(_add_link_to_node(node,link,hdes,name));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _add_link_to_node(node_type *node, link_type *link, int hdes, char *name)

{   int link_index;

    if(node == (node_type *)NULL || link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(node->links == (void *)NULL)
    {  node->link_alloc            = ALLOC_QUANTUM;

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes > 0 && hdes < appl_max_pheaps)
       {  (void)snprintf(node->link_name,SSIZE,"%s:link",name);
          node->links = (link_type **)phcalloc(hdes,ALLOC_QUANTUM,sizeof(link_type *),node->link_name);
          if(node->links == (link_type **)NULL)
             pups_error("[_add_link_to_node] failed to extend node link array (on persistent heap)");
       }
       else if(hdes > 0)
       { 

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&link->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(EINVAL);
          return((link_type *)NULL);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {  node->links = (link_type **)pups_calloc(ALLOC_QUANTUM,sizeof(link_type *));
          if(node->links == (link_type **)NULL)
             pups_error("[_add_link_to_node] failed to create node link array (on local heap/cache)");
       }

       node->links[node->link_cnt] = link;
       link_index = 0;
       ++node->link_cnt;
    }
    else if(node->link_alloc == node->link_cnt)
    {  node->link_alloc += ALLOC_QUANTUM;

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes > 0 && hdes < appl_max_pheaps)
       {  node->links = (link_type **)phrealloc(hdes,(void *)node->links,node->link_alloc*sizeof(link_type *),node->link_name);
          if(node->links == (link_type **)NULL)
             pups_error("[_add_link_to_node] failed to extend node link array (on persistent heap)");
       }
       else if(hdes > 0)
       { 
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&link->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(EINVAL);
          return(-1);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {  node->links = (link_type **)pups_realloc((void *)node->links,node->link_alloc*sizeof(link_type *));
          if(node->links == (link_type **)NULL)
             pups_error("[_add_link_to_node] failed to extend node link array (local heap/cache)");
       }

       node->links[node->object_cnt] = link;
       link_index                    = node->link_cnt;
       ++node->link_cnt;
    }
    else
    {  int i;


       /*---------------------------------------*/
       /* Search node link list for a free slot */
       /*---------------------------------------*/

       for(i=0; i<node->link_cnt; ++i)
       {  if(node->links[i] == (link_type *)NULL)
             break;
       }

       node->links[i] = link;
       link_index     = i;
       ++node->link_cnt;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(link_index);
}




/*------------------------------------------------------------------------------------------
    Dissociate a link from a node ...
------------------------------------------------------------------------------------------*/
/*---------------------------*/
/* Target link on local heap */
/*---------------------------*/

_PUBLIC int cantor_remove_link_from_node(node_type *node, char *link_tag)
{   return(_remove_link_from_node(node,link_tag,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*--------------------------------*/
/* Target link on persistent heap */
/*--------------------------------*/

_PUBLIC int cantor_phremove_link_from_node(node_type *node, char *link_tag, int hdes)
{   return(_remove_link_from_node(node,link_tag,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */

_PRIVATE int _remove_link_from_node(node_type *node, char *link_tag, int hdes)

{   int i;

    if(node == (node_type *)NULL || link_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    for(i=0; i<node->link_alloc; ++i)
    {  if(node->links[i] != (link_type *)NULL                                       &&
          (node->links[i] = (link_type *)cantor_pointer_live(node->links[i],TRUE))  &&
          strcmp(node->links[i]->tag,link_tag) == 0                                  )
       {  
          #ifdef PERSISTENT_HEAP_SUPPORT
          if(hdes > 0 && hdes < appl_max_pheaps)
          {  _clear_link(node->links[i],hdes);
             node->links[i] = (link_type *)phfree(hdes,(void *)node->links[i]);
          }
          else if(hdes > 0)
          { 

             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&node->mutex);
             #endif /* PTHREAD_SUPPORT */

             pups_set_errno(EINVAL);
             return(-1);
          }
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
          {  _clear_link(node->links[i],(-1));
             node->links[i] = (link_type *)pups_free((void *)node->links[i]);
          }

          --node->link_cnt;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(0);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}





/*-------------------------------------------------------------------------------------------
    Is a link tag associated with given node unique? ...
-------------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN cantor_link_tag_unique(node_type *node, char *link_tag)

{   int i;

    if(node == (node_type *)NULL || link_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    for(i=0; i<node->link_alloc; ++i)
    {  if(node->links[i] != (link_type *)NULL                                       &&
          (node->links[i] = (link_type *)cantor_pointer_live(node->links[i],TRUE))  &&
          strcmp(node->links[i]->tag,link_tag) == 0                                  )
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(TRUE);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(FALSE);
}




/*-------------------------------------------------------------------------------------------
    Get link index given link tag and node containing that tag ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_get_link_index_from_tag(node_type *node, char *link_tag)

{   int i;

    if(node == (node_type *)NULL || link_tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------*/
    /* Search object list for tag */
    /*----------------------------*/

    for(i=0; i<node->link_alloc; ++i)
    {  if(node->links[i] != (link_type *)NULL                                       &&
          (node->links[i] = (link_type *)cantor_pointer_live(node->links[i],TRUE))  &&
          strcmp(node->links[i]->tag,link_tag) == 0                                  )
       {
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&node->mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*-------------------------------------------------------------------------------------------
    Get link tag given link index ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_get_link_tag_from_index(char *tag, node_type *node, int link_index)

{  

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(node == (node_type *)NULL || tag == (char *)NULL || link_index < 0 || link_index > node->link_alloc)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&node->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(tag,node->links[link_index]->tag,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
    



/*-------------------------------------------------------------------------------------------
    Name a node ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_node_name(node_type *node, char *node_name)

{   if(node == (node_type *)NULL || node_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(node->name,node_name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------------------------------------------------------
    Set type of a node ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_node_type(node_type *node, char *type)

{   if(node == (node_type *)NULL || type  == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(node->type,type,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------------------------------------------------------
   Display (limited) information about node (print it to file) ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_show_node_info(FILE *stream, node_type *node)

{   int i;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_show_node_info] attempt by non root thread to perform PUPS/P3 utility operation");

    if(stream == (FILE *)NULL || node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(strcmp(node->name,"notset") == 0)
    {  pups_set_errno(EEXIST);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)fprintf(stream,"    Node \"%s\" (type %s)\n\n", node->name, node->type);
    if(node->object_alloc > 0)
       (void)fprintf(stream,"    objects:    %04d (%04d allocated)\n",node->object_cnt,node->object_alloc);
    else
       (void)fprintf(stream,"    objects:    non allocated\n");

    if(node->link_alloc > 0)
       (void)fprintf(stream,"    Links:   %04d (%04d allocated)\n",node->link_cnt,node->link_alloc);
    else
       (void)fprintf(stream,"    Links:   non allocated\n");

    if(node->object_alloc > 0)
    {  

       /*--------------------------------------------*/
       /* Print list of objects associated with node */
       /*--------------------------------------------*/

       (void)fprintf(stream,"\n\n    Object list\n");
       (void)fprintf(stream,"    ===========\n\n");
       for(i=0; i<node->object_alloc; ++i)
       {  if(node->objects[i] != (object_type *)NULL)
          {  if(node->objects[i] = (object_type *)cantor_pointer_live(node->objects[i],TRUE))
             {  if(node->objects[i]->olink != (void *)NULL)
                   (void)fprintf(stream,"    %4d object \"%s\" [type %-16s] attached to node (object pointer at 0x%010x virtual)\n",
                                                                                                                                  i,
                                                                                                              node->objects[i]->tag,
                                                                                                             node->objects[i]->type,
                                                                                         (unsigned long int)node->objects[i]->olink);
                else
                   (void)fprintf(stream,"    %4d object \"%s\" [type %-16s] attached to node (object pointer not set)\n",
                                                                                                                       i,
                                                                                                   node->objects[i]->tag,
                                                                                                  node->objects[i]->type);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"    %4d object: is stale (and has been removed)\n",i);
                (void)fflush(stream);
             }
          }
       }
    }

    if(node->link_alloc > 0)
    {

       /*------------------------------------------*/
       /* Print list of links associated with node */
       /*------------------------------------------*/

       (void)fprintf(stream,"\n\n    Link list\n");
       (void)fprintf(stream,"    =========\n\n");
       for(i=0; i<node->object_alloc; ++i)
       for(i=0; i<node->link_alloc; ++i)
       {  if(node->links[i] != (link_type *)NULL)
          {  if(node->links[i] = (link_type *)cantor_pointer_live(node->links[i],TRUE))
             {  (void)fprintf(stream,"    %4d link \"%-32s\" [nodes %04d, type %-16s] associated with node (length %6.3F, flux %6.3F)\n",
                                                                                                                                       i,
                                                                                                                     node->links[i]->tag,
                                                                                                              node->links[i]->node_alloc,
                                                                                                                    node->links[i]->type,
                                                                                                                  node->links[i]->length,
                                                                                                                    node->links[i]->flux);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"    %04d link: is stale (and has been removed)\n",i);
                (void)fflush(stream);
             }
          }
       }
    }

    (void)fprintf(stream,"\n\n");
    (void)fflush(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------------------------------------
    Clear a link ...
------------------------------------------------------------------------------*/
/*---------------------------*/
/* Target link on local heap */
/*---------------------------*/

_PUBLIC link_type *cantor_clear_link(link_type *link)
{   return(_clear_link(link,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*--------------------------------*/
/* Target link on persistent heap */
/*--------------------------------*/

_PUBLIC link_type *cantor_phclear_link(link_type *link, int hdes)
{   return(_clear_link(link,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE link_type *_clear_link(link_type *link, int hdes)

{   if(link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((link_type *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(hdes > 0 && hdes < appl_max_pheaps)
       link->routelist  = (node_type **)phfree(hdes,(void *)link->routelist);
    else if(hdes > 0)
    { 

        #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&link->mutex);
        #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((link_type *)NULL);
    }
    else if(hdes > 0)
    {
   
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&link->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((link_type *)NULL);
    }
    else
    #endif /* PERSISTENT_HEAP_SUPPORT */
       link->routelist  = (node_type **)pups_free((void *)link->routelist);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return((link_type *)NULL);
}




/*------------------------------------------------------------------------------
    Clear linklist ...
------------------------------------------------------------------------------*/
/*-------------------------------*/
/* Target linklist on local heap */
/*-------------------------------*/

_PUBLIC link_type **cantor_clear_linklist(link_type **link, int size)
{   return(_clear_linklist(link,size,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*------------------------------------*/
/* Target linklist on persistent heap */
/*------------------------------------*/

_PUBLIC link_type **cantor_phclear_linklist(link_type **link, int size, int hdes)
{   return(_clear_linklist(link,size,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE link_type **_clear_linklist(link_type **link, int size, int hdes)

{   int i;

    if(link == (link_type **)NULL || size <= 0 || hdes < 0 || hdes > appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return((link_type **)NULL);
    }

    for(i=0; i<size; ++i)
    {  if(link[i] == (link_type *)NULL || (link[i] == (link_type *)cantor_pointer_live(link[i],TRUE)))
       {

          #ifdef PERSISTENT_HEAP_SUPPORT
          if(hdes >= 0)
             (void)cantor_phclear_link(link[i],hdes);
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
             (void)cantor_clear_link(link[i]);
       }
    }

    pups_set_errno(OK);
    return((link_type **)NULL);
}




/*-------------------------------------------------------------------------------
    Add an object to a link ...
-------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Desitnation link on local heap */
/*--------------------------------*/

_PUBLIC int cantor_add_node_to_link(link_type *link, node_type *node)
{   return( _add_node_to_link(link,node,(-1),(char *)NULL));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------------------*/
/* Destination link on persistent heap */
/*-------------------------------------*/

_PUBLIC int cantor_phadd_node_to_link(link_type *link, node_type *node, int hdes, char *name)
{   return( _add_node_to_link(link,node,hdes,name));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE int _add_node_to_link(link_type *link, node_type *node, int hdes, char *name)

{   int i;

    if(link == (link_type *)NULL || node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(link->node_cnt == 0)
    {  link->node_alloc = ALLOC_QUANTUM;

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes > 0 && hdes < appl_max_pheaps)
       {  (void)snprintf(link->node_name,SSIZE,"%s:routelist",name);
          link->routelist = (node_type **)phcalloc(hdes,ALLOC_QUANTUM,sizeof(node_type **),link->node_name);
          if(link->routelist == (node_type **)NULL)
             pups_error("[_add_node_to_link] failed to create link routelist (on persistent heap)");
       }
       else if(hdes > 0)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&link->mutex);
          #endif /* PTHREAD_SUPPORT */

           pups_set_errno(EINVAL);
           return(-1);
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {  link->routelist = (node_type **)pups_calloc(ALLOC_QUANTUM,sizeof(node_type **));
          if(link->routelist == (node_type **)NULL)
             pups_error("[_add_node_to_link] failed to create link routelist (on local heap/cache)");
       }

       link->routelist[0] = node;
       ++link->node_cnt;
    }
    else if(link->node_cnt == link->node_alloc)
    {  link->node_alloc += ALLOC_QUANTUM; 

       #ifdef PERSISTENT_HEAP_SUPPORT
       if(hdes >= 0)
       {  link->routelist = (node_type **)phrealloc(hdes,(void *)link->routelist,ALLOC_QUANTUM*sizeof(node_type **),link->node_name);
          if(link->routelist == (node_type **)NULL)
             pups_error("[_add_node_to_link] failed to extend link routelist (on persistent heap)");
       }
       else
       #endif /* PERSISTENT_HEAP_SUPPORT */
       {  link->routelist                 = (node_type **)pups_realloc((void *)link->routelist,ALLOC_QUANTUM*sizeof(node_type **));
          if(link->routelist == (node_type **)NULL)
             pups_error("[_add_node_to_link] failed to extend link routelist (on local heap)");
       }

       link->routelist[link->node_cnt] = node;
       ++link->node_cnt;
    }
    else
    {  for(i=0; i<link->node_alloc; ++i)
       {  if(link->routelist[i] == (node_type *)NULL)
          {  link->routelist[i] = node;
             ++link->node_cnt;
             break;
          }
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------------------------
    Remove node from a link ...
--------------------------------------------------------------------------------*/

_PUBLIC int cantor_remove_node_from_link(link_type *link, char *name)

{   int i;

    if(link == (link_type *)NULL || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<link->node_alloc; ++i)
    {  if(link->routelist[i] != (node_type *)NULL                                            &&
          (link->routelist[i] != (node_type *)cantor_pointer_live(link->routelist[i],TRUE))  &&
          strcmp(link->routelist[i]->name,name) == 0                                          )
       {  link->routelist[i] = (node_type *)NULL;
          break;
       }
    }

    --link->node_cnt;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------------------------
    Show link information (print to file)) ...
--------------------------------------------------------------------------------*/

_PUBLIC void cantor_show_link_info(FILE *stream, link_type *link)

{   int i,
        cnt = 0;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_show_link_info] attempt by non root thread to perform PUPS/P3 utility operation");

    if(stream == (FILE *)NULL || link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(link->node_cnt > 0)
    {  (void)fprintf(stream,"\n\n    Link (for link \"%s\", [type %s])\n\n",link->tag,link->type);
       (void)fprintf(stream,"    length:  %6.3F\n",link->length);
       (void)fprintf(stream,"    flux:    %6.3F\n\n",link->flux);
       (void)fprintf(stream,"    ");
       (void)fflush(stream);

       for(i=0; i<link->node_alloc; ++i)
       {  if(link->routelist[i] != (node_type *)NULL)
          {  if((link->routelist[i] = (node_type *)cantor_pointer_live(link->routelist[i],TRUE)))
             {  (void)fprintf(stream,"%04d: [%-24s]  ",i,link->routelist[i]->name);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"%04d: [stale]  ",i);
                (void)fflush(stream);
             } 

             if(cnt == 5)
             {  cnt == 0;
                (void)fprintf(stream,"\n    ");
                (void)fflush(stream);
             }
             else
                ++cnt;
          }
       }

       (void)fprintf(stream,"\n\n");
       (void)fflush(stream);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
}




/*--------------------------------------------------------------------------------
    Copy link ...
--------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Destination link on local heap */
/*--------------------------------*/

_PUBLIC int cantor_copy_link(char *tag, link_type *from, link_type *to)

{   int i,
        cnt = 0;

    if(tag == (char *)NULL || from == (link_type *)NULL || to == (link_type *)NULL)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------*/
    /* Clear any existing link data */
    /*------------------------------*/

    cantor_clear_link(to);
    
    (void)strlcpy(to->tag,tag,SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->node_cnt   = from->node_cnt;
    to->node_alloc = from->node_alloc;
    to->flux       = from->flux;
    to->length     = from->length;

    to->routelist  = (node_type **)pups_realloc((void *)to->routelist,to->node_alloc*sizeof(node_type *));
    if(to->routelist == (node_type **)NULL)
       pups_error("[cantor_copy_link] could not extend \"to node\" route list (on local heap/cache)");

    for(i=0; i<from->node_alloc; ++i)
    {  if(from->routelist[i] != (node_type *)NULL && (from->routelist[i] = (node_type *)cantor_pointer_live(from->routelist[i],TRUE)))
       {  to->routelist[cnt] = from->routelist[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&from->mutex);
    (void)pthread_mutex_unlock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------------------*/
/* Destination link on persistent heap */
/*-------------------------------------*/

_PUBLIC int cantor_phcopy_link(int hdes, char *tag, link_type *from, link_type *to)

{   int i,
        cnt = 0;

    if(tag == (char *)NULL || from == (link_type *)NULL || to == (link_type *)NULL || hdes < 0 || hdes > appl_max_pheaps)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------*/
    /* Clear any existing link data */
    /*------------------------------*/

    cantor_phclear_link(to,hdes);

    (void)strlcpy(to->tag,tag,        SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->node_cnt   = from->node_cnt;
    to->node_alloc = from->node_alloc;
    to->flux       = from->flux;
    to->length     = from->length;

    to->routelist  = (node_type **)phrealloc(hdes,(void *)to->routelist,to->node_alloc*sizeof(node_type *),tag);
    if(to->routelist == (node_type **)NULL)
       pups_error("[cantor_phcopy_link] could not extend \"to node\" route list (on persistent heap)");

    for(i=0; i<from->node_alloc; ++i)
    {  if(from->routelist[i] != (node_type *)NULL && (from->routelist[i] = (node_type *)cantor_pointer_live(from->routelist[i],TRUE)))
       {  to->routelist[cnt] = from->routelist[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&from->mutex);
    (void)pthread_mutex_unlock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*-----------------------------------------------------------------------------------
    Copy a node ...
-----------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Destination node on local heap */
/*--------------------------------*/

_PUBLIC int cantor_copy_node(char *name, node_type *from, node_type *to)

{   int i,
        cnt = 0;

    if(name == (char *)NULL || from == (node_type *)NULL || to == (node_type *)NULL)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------------------------*/
    /* Remove any existing object data on "to" node */
    /*----------------------------------------------*/

    cantor_clear_node(to);


    /*---------------------*/
    /* Copy node variables */
    /*---------------------*/

    (void)strlcpy(to->name,name,      SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->object_cnt   = from->object_cnt;
    to->object_alloc = from->object_alloc;
    to->link_cnt     = from->link_cnt;
    to->link_alloc   = from->link_alloc;


    /*-----------------------------------------------------*/
    /* Make sure we have sufficient dataspace on "to" node */
    /*-----------------------------------------------------*/

    to->objects = (object_type **)pups_realloc((void *)to->objects,to->object_alloc*sizeof(object_type *));
    if(to->objects == (object_type **)NULL)
       pups_error("[cantor copy node] could not extend \"to node\" object array (on local heap/cache)");

    to->links   =  (link_type **) pups_realloc((void *)to->links,  to->link_alloc*sizeof(link_type *));
    if(to->objects == (object_type **)NULL)
       pups_error("[cantor copy node] could not extend \"to node\" link array (on local heap/cache)");


    /*------------------*/
    /* Copy object data */
    /*------------------*/

    for(i=0; i<from->object_alloc; ++i)
    {  if(from->objects[i] != (object_type *)NULL && (from->objects[i] == (object_type *)cantor_pointer_live(from->objects[i],TRUE)))
       {  to->objects[cnt] = from->objects[i];
          ++cnt;
       }
    }


    /*----------------*/
    /* Copy link data */
    /*----------------*/

    cnt = 0;
    for(i=0; i<from->link_alloc; ++i)
    {  if(from->links[i] != (link_type *)NULL)
       {  to->links[cnt] = from->links[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&from->mutex);
    (void)pthread_mutex_unlock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------------------*/
/* Destination node on persistent heap */
/*-------------------------------------*/

_PUBLIC int cantor_phcopy_node(int hdes, char *name, node_type *from, node_type *to)

{   int i,
        cnt = 0;

    if(from == (node_type *)NULL || to == (node_type *)NULL || hdes < 0 || hdes > appl_max_pheaps)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------------------------*/
    /* Remove any existing object data on "to" node */
    /*----------------------------------------------*/

    cantor_phclear_node(to,hdes);


    /*---------------------*/
    /* Copy node variables */
    /*---------------------*/

    (void)strlcpy(to->name,name,      SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->object_cnt   = from->object_cnt;
    to->object_alloc = from->object_alloc;
    to->link_cnt     = from->link_cnt;
    to->link_alloc   = from->link_alloc;


    /*-----------------------------------------------------*/
    /* Make sure we have sufficient dataspace on "to" node */
    /*-----------------------------------------------------*/

    to->objects = (object_type **)phrealloc(hdes,(void *)to->objects,to->object_alloc*sizeof(object_type *),name);
    if(to->objects == (object_type **)NULL)
       pups_error("[cantor_phcopy_node] could not extend \"to node\" object array (on persistent heap)");

    to->links   =  (link_type **) phrealloc(hdes,(void *)to->links,  to->link_alloc*sizeof(link_type *),name);
    if(to->links == (link_type **)NULL)
       pups_error("[cantor_phcopy_node] could not extend \"to node\" link array (on persistent heap)");


    /*------------------*/ 
    /* Copy object data */
    /*------------------*/ 

    for(i=0; i<from->object_alloc; ++i)
    {  if(from->objects[i] != (object_type *)NULL && (from->objects[i] == (object_type *)cantor_pointer_live(from->objects[i],TRUE)))
       {  to->objects[cnt] = from->objects[i];
          ++cnt;
       }
    }


    /*----------------*/
    /* Copy link data */
    /*----------------*/

    cnt = 0;
    for(i=0; i<from->link_alloc; ++i)
    {  if(from->links[i] != (link_type *)NULL)
       {  to->links[cnt] = from->links[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&from->mutex);
    (void)pthread_mutex_unlock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*-----------------------------------------------------------------------------------
     Copy a nodelist ...
-----------------------------------------------------------------------------------*/
/*------------------------------------*/
/* Destination nodelist on local heap */
/*------------------------------------*/

_PUBLIC int cantor_copy_nodelist(char *name, nodelist_type *from, nodelist_type *to)

{   int i,
        cnt = 0;

    if(name == (char *)NULL || from == (nodelist_type *)NULL || to == (nodelist_type *)NULL)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------------------------*/
    /* Remove any existing object data on "to" node */
    /*----------------------------------------------*/

    cantor_clear_nodelist(to);


    /*---------------------*/
    /* Copy node variables */
    /*---------------------*/

    (void)strlcpy(to->name,name,      SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->node_cnt   = from->node_cnt;
    to->node_alloc = from->node_alloc;


    /*-----------------------------------------------------*/
    /* Make sure we have sufficient dataspace on "to" node */
    /*-----------------------------------------------------*/

    to->nodes = (node_type **)pups_realloc((void *)to->nodes,to->node_alloc*sizeof(node_type *));
    if(to->nodes == (node_type **)NULL)
       pups_error("[cantor_copy_nodelist] failed to extend \"to nodelist\" (on local heap)");

    /*----------------*/ 
    /* Copy node data */
    /*----------------*/ 

    for(i=0; i<from->node_alloc; ++i)
    {  if(from->nodes[i] != (node_type *)NULL && (from->nodes[i] == (node_type *)cantor_pointer_live(from->nodes[i],TRUE)))
       {  to->nodes[cnt] = from->nodes[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------*/
/* Destination nodelist on persistent heap */
/*-----------------------------------------*/

_PUBLIC int cantor_phcopy_nodelist(int hdes, char *name, nodelist_type *from, nodelist_type *to)

{   int i,
        cnt = 0;

    if(from == (nodelist_type *)NULL || to == (nodelist_type *)NULL)
    {  pups_set_errno(OK);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------------------------*/
    /* Remove any existing_object data on "to" node */
    /*----------------------------------------------*/

    cantor_phclear_nodelist(to,hdes);


    /*---------------------*/
    /* Copy node variables */
    /*---------------------*/

    (void)strlcpy(to->name,name,SSIZE);
    (void)strlcpy(to->type,from->type,SSIZE);

    to->node_cnt   = from->node_cnt;
    to->node_alloc = from->node_alloc;


    /*-----------------------------------------------------*/
    /* Make sure we have sufficient dataspace on "to" node */
    /*-----------------------------------------------------*/

    to->nodes = (node_type **)phrealloc(hdes,(void *)to->nodes,to->node_alloc*sizeof(node_type *), name);
    if(to->nodes == (node_type **)NULL)
       pups_error("[cantor_phcopy_nodelist] failed to extend \"to nodelist\" (on persistent heap)");


    /*----------------*/
    /* Copy node data */
    /*----------------*/

    for(i=0; i<from->node_alloc; ++i)
    {  if(from->nodes[i] != (node_type *)NULL && (from->nodes[i] == (node_type *)cantor_pointer_live(from->nodes[i],TRUE)))
       {  to->nodes[cnt] = from->nodes[i];
          ++cnt;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&from->mutex);
    (void)pthread_mutex_lock(&to->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*-----------------------------------------------------------------------------------
    Set link tag ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_link_tag(link_type *link, char *tag)

{   if(link == (link_type *)NULL || tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(link->tag,tag,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------------
    Set link type ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_link_type(link_type *link, char *type)

{   if(link == (link_type *)NULL || type == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(link->type,type,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------------
    Set link flux ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_link_flux(link_type *link, FTYPE flux)

{   if(link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    link->flux = flux;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */


    pups_set_errno(OK);
    return(0);
}



/*-----------------------------------------------------------------------------------
    Set link length ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_link_length(link_type *link, FTYPE length)

{   if(link == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    link->length = length;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------------
    Compare a pair of nodes ...
-----------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN cantor_compare_nodes(node_type *n1, node_type *n2)

{   int i,
        matched = 0;

    if(n1 == (node_type *)NULL || n2 == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&n1->mutex);
    (void)pthread_mutex_lock(&n2->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(n1->object_cnt != n2->object_cnt    ||
       n1->link_cnt   != n2->link_cnt       )
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }


    /*-----------------*/
    /* Compare objects */
    /*-----------------*/

    for(i=0; i<n1->object_alloc; ++i)
    {  int j;

       for(j=i; j<n2->object_alloc; ++j)
       {  if(n1->objects[i] == n2->objects[j])
             ++matched;
       }
    }
    
    if(matched != n1->object_cnt && matched != n2->object_cnt)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&n1->mutex);
       (void)pthread_mutex_unlock(&n2->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(FALSE);
    }


    /*---------------*/
    /* Compare links */
    /*---------------*/

    matched = 0;
    for(i=0; i<n1->link_alloc; ++i)
    {  int j;

       for(j=i; j<n2->link_alloc; ++j)
       {  if(n1->links[i] == n2->links[j])
             ++matched;
       }
    }

    if(matched != n1->link_cnt && matched != n2->link_cnt)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&n1->mutex);
       (void)pthread_mutex_unlock(&n2->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&n1->mutex);
    (void)pthread_mutex_unlock(&n2->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(TRUE);
}




/*------------------------------------------------------------------------------
    Clear an object ...
------------------------------------------------------------------------------*/
/*----------------------*/
/* Object on local heap */
/*----------------------*/

_PUBLIC object_type *cantor_clear_object(object_type *object)
{   return(_clear_object(object,(-1)));
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*---------------------------*/
/* Object on persistent heap */
/*---------------------------*/

_PUBLIC object_type *cantor_phclear_object(object_type *object, int hdes)
{   return(_clear_object(object,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE object_type *_clear_object(object_type *object, int hdes)


{   if(object == (object_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((object_type *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(hdes > 0 && hdes < appl_max_pheaps)
       (void)phfree(hdes,(void *)object);
    else if(hdes > 0)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&object->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return((object_type *)NULL);
    }
    else
    #endif /* PERSISTENT_HEAP_SUPPORT */
       (void)pups_free((void *)object);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return((object_type *)NULL);
}




/*------------------------------------------------------------------------------
    Clear an object list ...
------------------------------------------------------------------------------*/
/*---------------------------*/
/* Object list on local heap */
/*---------------------------*/

_PUBLIC object_type **cantor_clear_objectlist(object_type **object, int size)
{   return(_clear_objectlist(object,size,(-1)));
}

#ifdef PERSISTENT_HEAP_SUPPORT
/*--------------------------------*/
/* Object list on persistent heap */
/*--------------------------------*/

_PUBLIC  object_type **cantor_phclear_objectlist(object_type **object, int size, int hdes)
{   return(_clear_objectlist(object,size,hdes));
}
#endif /* PERSISTENT_HEAP_SUPPORT */


_PRIVATE object_type **_clear_objectlist(object_type **object, int size, int hdes)

{   int i;

    if(object == (object_type **)NULL || size <= 0)
    {  pups_set_errno(EINVAL);
       return((object_type **)NULL);
    }

    for(i=0; i<size; ++i)
    {  if(object[i] != (object_type *)NULL && (object[i] = (object_type *)cantor_pointer_live(object[i],TRUE)))

          #ifdef PERSISTENT_HEAP_SUPPORT
          if(hdes > 0 && hdes < appl_max_pheaps)
             (void)cantor_phclear_object(object[i],hdes);
          else if(hdes > 0)
          {  pups_set_errno(EINVAL);
             return((object_type **)NULL);
          }
          else
          #endif /* PERSISTENT_HEAP_SUPPORT */
             (void)cantor_clear_object(object[i]);
    }

    pups_set_errno(OK);
    return((object_type **)NULL);
}




/*-----------------------------------------------------------------------------------
    Set object name ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_object_tag(object_type *object, char *tag)

{   if(object == (object_type *)NULL || tag == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(object->tag,tag,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------------
    Set object type ...
-----------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_object_type(object_type *object, char *type)

{   if(object == (object_type *)NULL || type == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(object->type,type,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*-------------------------------------------------------------------------------------
    Add a databag or DLL to an object ...
-------------------------------------------------------------------------------------*/

_PUBLIC int cantor_set_olink(object_type *object,  // Object
                             void        *olink)   // Link data or DLL

{   if(object == (object_type *)NULL || olink == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    object->olink = olink;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&object->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------------------------------------------------
    Is the pointer live?  ...
-------------------------------------------------------------------------------------*/

_PUBLIC void *cantor_pointer_live(void     *pointer,                // Pointer to test (returns NULL if pointer invalid)
                                  _BOOLEAN restore_default_handler) // Restore default handlers post test

{   void  *ret = (void *)NULL;
    _BYTE test;

    if(pointer == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }


    /*-----------------------------------------------------------*/
    /* Set a (temporary) handler for SIGSEGV. If pointer is dead */
    /* it will cause SIGSEGV to be raised, in which case we need */
    /* to clear the pointer and return                           */
    /*-----------------------------------------------------------*/

    if(pups_backtrack(TRUE) == PUPS_BACKTRACK)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@@%s:%s): stale pointer (at 0x%010x virtual) -- clearing\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner,(unsigned long)pointer);
          (void)fflush(stderr);
       }

       ret = (void *)NULL;
       pups_set_errno(EACCES);
    }
    else
    {  test = *(_BYTE *)pointer;
       ret  =  (void  *)pointer;
    }


    /*------------------------*/
    /* Restore signal handler */
    /*------------------------*/

    if(restore_default_handler == TRUE)
       (void)pups_backtrack(FALSE);

    pups_set_errno(OK);
    return(ret);
}



/*-------------------------------------------------------------------------------------------
   Show (limited) node information (HTML) ...
-------------------------------------------------------------------------------------------*/

_PUBLIC int cantor_vhtml_node_info(_BOOLEAN  full_node_info,  // TRUE if full link information required 
                                   char      *dir,            // Directory containing (virtual) HTML file
                                   char      *filename,       // Name of virtual HTML file
                                   node_type *node)           // Node containing data

{   int i;

    char current_dir[SSIZE] = "";
    FILE *stream            = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_vhtml_node_info] attempt by non root thread to perform PUPS/P3 cantor operation");


    if(filename == (char *)NULL || strin(filename,".html") == FALSE)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(node == (node_type *)NULL || strcmp(node->name,"notset") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(dir != (char *)NULL)
    {  (void)getcwd(current_dir,SSIZE);
       if(chdir(dir) == (-1))
       {  pups_set_errno(ENOENT);
          return(-1);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(filename,0600);

    stream = pups_fopen(filename,"w",LIVE);
    (void)fprintf(stream,"<html><head><title>Node information page</title></head><body>\n");

    if(node == (node_type *)NULL || node->object_cnt == 0)
    {  if(node->object_cnt == 0)
       {  (void)fprintf(stream,"<hr><center><h3>Cantor version %s<br>Node \"%s\" [type %s] is empty<hr><br><br>\n",
                                                                             CANTOR_VERSION,node->name, node->type);

          (void)fprintf(stream,"<center><h5>");

          if(node->object_alloc > 0)
             (void)fprintf(stream,"objects:    %04d (%04d allocated)<br>\n",node->object_cnt,node->object_alloc);
          else
             (void)fprintf(stream,"objects:    non allocated<br>");

          if(node->link_alloc > 0)
             (void)fprintf(stream,"links:   %04d (%04d allocated)<br>\n",node->link_cnt,node->link_alloc);
          else
             (void)fprintf(stream,"links:   non allocated<br>\n");

          (void)fprintf(stream,"</h5></center>");
       }
       else
          (void)fprintf(stream,"<hr><center><h3>Cantor version %s<br>Node is NULL</h3></center><hr><br><br>\n",CANTOR_VERSION);

       (void)fprintf(stream,"<br><hr><br><center><h6>Web page generated automatically by <b>cantor</b> vhtml generator (version 1.00)\n");
       (void)fprintf(stream,"(C) M.A. O'Neill, Tumbling Dice, 2022 (mao@@tumblingdice.co.uk)</center><br><br>\n");
       (void)fprintf(stream,"</body></html>\n");
       (void)fflush(stream);

       (void)pups_fclose(stream);

       if(dir != (char *)NULL)
          (void)chdir(current_dir);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&node->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EEXIST);
       return(-1);
    }

    (void)fprintf(stream,"<hr><center><h3>Cantor version %s)<br>Node <b>%s</b> [type %s]</h3></center><hr><br><br>\n",
                                                                                CANTOR_VERSION,node->name, node->type);
    (void)fprintf(stream,"<center><h5>");
    if(node->object_alloc > 0)
       (void)fprintf(stream,"objects:    %04d (%04d allocated)<br>\n",node->object_cnt,node->object_alloc);
    else
       (void)fprintf(stream,"objects:    non allocated");

    if(node->link_alloc > 0)
       (void)fprintf(stream,"links:   %04d (%04d allocated)<br>\n",node->link_cnt,node->link_alloc);
    else
       (void)fprintf(stream,"links:   non allocated<br>\n");
    (void)fprintf(stream,"</h5></center>");

    if(node->object_alloc > 0)
    {  

       /*--------------------------------------------*/
       /* Print list of objects associated with node */
       /*--------------------------------------------*/

       (void)fprintf(stream,"<br><hr><h4><center><b>Object list</b></center><hr><br>\n");

       (void)fprintf(stream,"<h5>");
       for(i=0; i<node->object_alloc; ++i)
       {  if(node->objects[i] != (object_type *)NULL)
          {  if(node->objects[i] = (object_type *)cantor_pointer_live(node->objects[i],TRUE))
             {  if(node->objects[i]->olink != (void *)NULL)
                {  char object_info[SSIZE] = "";


                   /*-------------------------------------------------*/
                   /* Can we produce a live link to specified object? */
                   /*-------------------------------------------------*/

                   if(node->objects[i]->infopage != (char *)NULL && access(node->objects[i]->infopage,F_OK | R_OK | W_OK) != (-1))
                      (void)snprintf(object_info,SSIZE,", (see <a href=%s>%s</a>)",node->objects[i]->infopage,node->objects[i]->infopage);
                   else
                      (void)snprintf(object_info,SSIZE,"");

                  
                   (void)fprintf(stream,"%04d object <b>%s</b> [type %s%s] attached to node object pointer at 0x%010x virtual)<br>\n",
                                                                                                                                    i,
                                                                                                                node->objects[i]->tag,
                                                                                                                          object_info,
                                                                                                               node->objects[i]->type,
                                                                                               (unsigned long)node->objects[i]->olink);
                }
                else
                   (void)fprintf(stream,"%04d object <b>%s</b> [type %-16s] attached to node (object pointer not set)<br>\n",
                                                                                                                           i,
                                                                                                       node->objects[i]->tag,
                                                                                                      node->objects[i]->type);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"%04d object: is stale (and has been removed)\n",i);
                (void)fflush(stream);
             }
          }
       }
       (void)fprintf(stream,"</h5>");
    }

    if(node->link_alloc > 0)
    {  

       /*------------------------------------------*/
       /* Print list of links associated with node */
       /*------------------------------------------*/

       (void)fprintf(stream,"<br><br><hr><center><h4>Link list<br></center><hr><br><br>\n");
       (void)fprintf(stream,"<h5>");
       for(i=0; i<node->link_alloc; ++i)
       {  if(node->links[i] != (link_type *)NULL)
          {  if(node->links[i] = (link_type *)cantor_pointer_live(node->links[i],TRUE))
             {  char link_info[SSIZE]          = "",
                     link_info_filename[SSIZE] = "";


                /*----------------------------------------------------*/
                /* Generate full link info (if user has asked for it) */
                /*----------------------------------------------------*/

                if(full_node_info == TRUE)
                {  (void)snprintf(link_info_filename,SSIZE,"link.%s.html",node->links[i]->tag); 
                   (void)cantor_vhtml_link_info((char *)NULL,link_info_filename,node->links[i]);
                   (void)snprintf(link_info,SSIZE,"<a href=%s>%s</a>",link_info_filename,node->links[i]->tag);
                }
                else
                   (void)snprintf(link_info,SSIZE,"<b>%s</b>",node->links[i]->tag);

                (void)fprintf(stream,"%04d link %s [nodes %04d, type %-16s] associated with node (length %6.3F, flux %6.3F)<br>\n",
                                                                                                                                 i,
                                                                                                                         link_info,
                                                                                                        node->links[i]->node_alloc,
                                                                                                              node->links[i]->type,
                                                                                                            node->links[i]->length,
                                                                                                              node->links[i]->flux);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"%04d link: is stale (and has been removed)<br>\n",i);
                (void)fflush(stream);
             }
          }
       }
       (void)fprintf(stream,"</h5>");
    }

    (void)fprintf(stream,"<br><hr><br><center><h6>Web page generated automatically by <b>cantor</b> vhtml generator (version 1.00)\n");
    (void)fprintf(stream,"(C) M.A. O'Neill, Tumbling Dice, 2022 (mao@@tumblingdice.co.uk)</center><br><br>\n");
    (void)fprintf(stream,"</body></html>\n");
    (void)fflush(stream);

    (void)pups_fclose(stream);

    if(dir != (char *)NULL)
       (void)chdir(current_dir);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------------------------
    Show link information (HTML) ...
--------------------------------------------------------------------------------*/

_PUBLIC int cantor_vhtml_link_info(char *dir,         // Directory of generated (virtual) HTML file
                                   char *filename,    // Filename of (virtual) HTML file
                                   link_type *link)   // Link data

{   int i,
        cnt = 0;

    char current_dir[SSIZE] = "";
    FILE *stream            = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_vhtml_link_info] attempt by non root thread to perform PUPS/P3 cantor operation");

    if(filename == (char *)NULL || strin(filename,".html") == FALSE)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(dir != (char *)NULL)
    {  (void)getcwd(current_dir,SSIZE);
       if(chdir(dir) == (-1))
       {  pups_set_errno(ENOENT);
          return(-1);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(filename,0600);

    stream = pups_fopen(filename,"w",LIVE);

    if(link == (link_type *)NULL || link->node_cnt == 0)
    {  (void)fprintf(stream,"<html><head><title>Link information page</title></head><body>\n");

       if(link->node_cnt == 0)
       {  (void)fprintf(stream,"<hr><center><h3><b>Cantor version %s<br>Link \"%s\" [type %s] is empty</center></b><hr><br>\n",CANTOR_VERSION,link->tag,link->type);
          if(link->node_alloc > 0)
             (void)fprintf(stream,"<center><h5>Link nodes:    (%04d allocated)</h5></center>\n",link->node_alloc);
          else
             (void)fprintf(stream,"<center><h5>Link nodes:    non allocated</h5></center>");
       }
       else
          (void)fprintf(stream,"<hr><center><h3><b>Cantor version %s<br>Link is NULL</h3></center></b><hr><br>\n",CANTOR_VERSION);

       (void)fprintf(stream,"<br><hr><br>\n");
       (void)fprintf(stream,"<center><h6>Web page generated automatically by <b>cantor</b> vhtml generator version 1.00<br>\n");
       (void)fprintf(stream,"(C) M.A. O'Neill, Tumbling Dice, 2022 (mao@@tumblingdice.co.uk)</center><br><br>\n");
       (void)fprintf(stream,"</body></html>\n");
       (void)fflush(stream);

       (void)pups_fclose(stream);
       if(dir != (char *)NULL)
          (void)chdir(current_dir);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&link->mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EEXIST);
       return(-1);
    }

    if(link->node_cnt > 0)
    {  (void)fprintf(stream,"<html><head><title>Link information page</title></head><body>\n"); 
       (void)fprintf(stream,"<hr><center><h3><b>Cantor version %s<br>Link (for link %s, [type %s])</center></b><hr><br>\n",CANTOR_VERSION,link->tag,link->type);

       (void)fprintf(stream,"<center><h5>");
       if(link->node_alloc > 0)
          (void)fprintf(stream,"Link nodes:    %04d (%04d allocated)<br><br>\n",link->node_cnt,link->node_alloc);
       else
          (void)fprintf(stream,"Link nodes:    non allocated<br><br>\n");

       (void)fprintf(stream,"length:  %6.3F<br>\n",link->length);
       (void)fprintf(stream,"flux:    %6.3F<br><br><hr><br>\n",link->flux);
       (void)fprintf(stream,"</h5></center>\n");

       (void)fprintf(stream,"<center><h4>Nodes associated with link</center><hr><br>\n");
       (void)fflush(stream);

       (void)fprintf(stream,"<h5>");
       for(i=0; i<link->node_alloc; ++i)
       {  if(link->routelist[i] != (node_type *)NULL)
          {  if((link->routelist[i] = (node_type *)cantor_pointer_live(link->routelist[i],TRUE)))
             {  char node_info[SSIZE]          = "",
                     node_info_filename[SSIZE] = ""; 


                /*------------------------------------------*/
                /* Do we have a live link to the link node? */
                /*------------------------------------------*/

                (void)snprintf(node_info_filename,SSIZE,"node.%s.html",link->routelist[i]->name);

                if(access(node_info_filename,F_OK | R_OK | W_OK) != (-1))
                   (void)snprintf(node_info,SSIZE,"<a href=%s>%s</a>",node_info_filename,link->routelist[i]->name);
                else
                   (void)snprintf(node_info,SSIZE,"<b>%s</b>",link->routelist[i]->name);

                (void)fprintf(stream,"[%d: %s]    ",i,node_info);
                (void)fflush(stream);
             }
             else
             {  (void)fprintf(stream,"[%d: stale]    ",i);
                (void)fflush(stream);
             }

             if(cnt == 5)
             {  cnt == 0;
                (void)fprintf(stream,"\n");
                (void)fflush(stream);
             }
             else
                ++cnt;
          }
       }
       (void)fprintf(stream,"</h5>");

       if(cnt != 0)
       {  (void)fprintf(stream,"\n");
          (void)fflush(stream);
       }

       (void)fprintf(stream,"<br><hr><br>\n");
       (void)fprintf(stream,"<center><h6>Web page generated automatically by <b>cantor</b> vhtml generator version 1.00<br>\n");
       (void)fprintf(stream,"(C) M.A. O'Neill, Tumbling Dice, 2022 (mao@@tumblingdice.co.uk)</center><br><br>\n");
       (void)fprintf(stream,"</body></html>\n");
       (void)fflush(stream);
    }

    if(dir != (char *)NULL)
       (void)chdir(current_dir);

    (void)pups_fclose(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*----------------------------------------------------------------------------------
    Display node (as VHTML page) ...
----------------------------------------------------------------------------------*/

_PUBLIC int cantor_vhtml_node_browse(_BOOLEAN   full_node_info,  // TRUE if full node data required 
                                     char      *browser,         // Browser to display (virtual) HTML
                                     node_type *node)            // Node containing data

{   FILE *stream = (FILE *)NULL;

    char vhtml_filename[SSIZE]  = "",
         vhtml_dirname[SSIZE]   = "",
         browser_command[SSIZE] = "",
         rm_command[SSIZE]      = "";
  
    static _BOOLEAN in_vhtml_node_browse = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_vhtml_node_browse] attempt by non root thread to perform PUPS/P3 cantor operation");

    if(browser == (char *)NULL || node == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else if(in_vhtml_node_browse == TRUE)
    {  pups_set_errno(EBUSY);
       return(-1);
    }
    else
       in_vhtml_node_browse = TRUE;


    /*---------------*/
    /* Generate HTML */
    /*---------------*/

    (void)snprintf(vhtml_dirname,SSIZE,"vhtml.%s.%d.%s.dir.tmp",appl_name,appl_pid,appl_host);
    (void)mkdir(vhtml_dirname,0770);
    (void)snprintf(vhtml_filename,SSIZE,"node.%s.html",node->name);
    (void)cantor_vhtml_node_info(full_node_info,vhtml_dirname,vhtml_filename,node);


    /*--------------------------------------------------*/
    /* Run browser (and display HTML we have generated) */
    /*--------------------------------------------------*/

    (void)snprintf(browser_command,SSIZE,"%s -geometry 600x800 %s/%s",browser,vhtml_dirname,vhtml_filename);     
    if(WEXITSTATUS(system(browser_command)) < 0)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }


    /*-------------*/
    /* Remove junk */
    /*-------------*/

    (void)snprintf(rm_command,SSIZE,"rm -rf %s",vhtml_dirname);
    if(WEXITSTATUS(system(rm_command)) < 0)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }

    in_vhtml_node_browse = FALSE;
    pups_set_errno(OK);

    return(0);
}




/*---------------------------------------------------------------------------------------
    Display a list of nodes (with their associated links) as HTML ...
---------------------------------------------------------------------------------------*/

_PUBLIC int cantor_vhtml_nodelist_browse(_BOOLEAN        full_node_info,  // TRUE if full node information required
                                          char          *browser,         // Browser to display (virtual) HTML
                                          nodelist_type *nodelist)        // Nodelist containing data nodes

{   FILE *stream = (FILE *)NULL;

    char vhtml_filename[SSIZE]          = "",
         vhtml_nodelist_filename[SSIZE] = "",
         vhtml_dirname[SSIZE]           = "",
         browser_command[SSIZE]         = "",
         rm_command[SSIZE]              = "",
         current_dir[SSIZE]             = "";

    int i,
        ret,
        cnt          = 0,
        active_nodes = 0;

    static _BOOLEAN in_vhtml_node_browse = FALSE;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cantor_vhtml_nodelist_browse] attempt by non root thread to perform PUPS/P3 cantor operation");

    if(browser == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else if(in_vhtml_node_browse == TRUE)
    {  pups_set_errno(EBUSY);
       return(-1);
    }
    else
       in_vhtml_node_browse = TRUE;

    (void)snprintf(vhtml_dirname,SSIZE,"vhtml.%s.%d.%s.dir.tmp",appl_name,appl_pid,appl_host);
    (void)mkdir(vhtml_dirname,0770);
    (void)getcwd(current_dir,SSIZE);
    (void)chdir(vhtml_dirname);

    (void)snprintf(vhtml_nodelist_filename,SSIZE,"nodelist.%d.%s.html",appl_pid,appl_host);
    if(access(vhtml_nodelist_filename,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(vhtml_nodelist_filename,0600);
    stream = pups_fopen(vhtml_nodelist_filename,"w",LIVE);


    /*---------------*/
    /* Generate HTML */
    /*---------------*/

    if(nodelist == (nodelist_type *)NULL)
    {  (void)fprintf(stream,"<html><head><title>Nodelist</title></head><body>\n");
       (void)fprintf(stream,"<hr><center><h3><b>Cantor version %s<br>Nodelist is empty</b></h3></center><hr><br><br>\n",CANTOR_VERSION);
       (void)fflush(stream);
    }
    else
    {  (void)fprintf(stream,"<html><head><title>Nodelist</title></head><body>\n");
       (void)fprintf(stream,"<hr><center><h3><b>Cantor version %s<br>Nodelist \"%s\" [type %s] Information Page</b></center><hr><br><br>\n",
                                                                                               CANTOR_VERSION,nodelist->name,nodelist->type);
       (void)fflush(stream);


       (void)fprintf(stream,"<h5>");
       (void)fflush(stream);


       /*----------------------------------------------------------------*/
       /* Create (empty) files for all nodes -- we do this to ensure all */
       /* active nodes are properly linked within HTML                   */
       /*----------------------------------------------------------------*/

       for(i=0; i<nodelist->node_alloc; ++i)
       {  (void)snprintf(vhtml_filename,SSIZE,"node.%s.html",nodelist->nodes[i]->name);

          if(access(vhtml_filename,F_OK | R_OK | W_OK) == (-1))
             (void)pups_creat(vhtml_filename,0600);
       }


       /*--------------------------------*/
       /* Create actual HTML information */
       /*--------------------------------*/

       for(i=0; i<nodelist->node_alloc; ++i)
       {

          /*------------------------------------*/
          /* Does next node contain valid data? */
          /*------------------------------------*/

          if(nodelist->nodes[i] != (node_type *)NULL                                           &&
             (nodelist->nodes[i] = (node_type *)cantor_pointer_live(nodelist->nodes[i],TRUE))  &&
             strcmp(nodelist[i].name,"notset") != 0                                             )
          {  (void)snprintf(vhtml_filename,SSIZE,"node.%s.html",nodelist->nodes[i]->name);
             (void)cantor_vhtml_node_info(full_node_info,(char *)NULL,vhtml_filename,nodelist->nodes[i]);


             /*----------------------------*/
             /* Generate link to next node */
             /*----------------------------*/

             (void)fprintf(stream,"[%d <a href=%s>%s</a>]  ",i,vhtml_filename,nodelist->nodes[i]->name);

             if(cnt == 5)
             {  (void)fprintf(stream,"\n");
                (void)fflush(stream);

                cnt = 0;
             }
             else
                cnt = 0;

             ++active_nodes;
          }
       }

       if(cnt != 0)
       {  (void)fprintf(stream,"\n");
          (void)fflush(stream);
       }

       (void)fprintf(stream,"</h5>");
       (void)fflush(stream);

       if(active_nodes == 0)
          (void)fprintf(stream,"<h5>No active nodes (%d nodes allocated)\n",nodelist->node_alloc);
        else
          (void)fprintf(stream,"<h5>%d active nodes (%d nodes allocated)\n",active_nodes,nodelist->node_alloc);
        (void)fflush(stream);
    }

    (void)fprintf(stream,"<br><hr><br>\n");
    (void)fprintf(stream,"<center><h6>Web page generated automatically by <b>cantor</b> vhtml generator version 1.00<br>\n");
    (void)fprintf(stream,"(C) M.A. O'Neill, Tumbling Dice, 2022 (mao@@tumblingdice.co.uk)</center><br><br>\n");
    (void)fprintf(stream,"</body></html>\n");
    (void)fflush(stream);

    if(vhtml_dirname != (char *)NULL)
       (void)chdir(current_dir);

    (void)pups_fclose(stream);


    /*--------------------------------------------------*/
    /* Run browser (and display HTML we have generated) */
    /*--------------------------------------------------*/

    (void)snprintf(browser_command,SSIZE,"%s -geometry 600x800 %s/%s",browser,vhtml_dirname,vhtml_nodelist_filename);
    ret = system(browser_command);
    if(WEXITSTATUS(ret) < 0)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    } 


    /*-------------*/
    /* Remove junk */
    /*-------------*/

    (void)snprintf(rm_command,SSIZE,"rm -rf %s",vhtml_dirname);
    ret = system(rm_command);
    if(WEXITSTATUS(ret) < 0)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    } 

    in_vhtml_node_browse = FALSE;
    pups_set_errno(OK);

    return(0);
}




/*---------------------------------------------------------------------------------------
    Convert a link to a set of pairwise rules. These are passed to an enslaved
    rgen ruleset convertor and hence to a subslaved cluster object clusterer with
    sybiotic xtopol network visualiser ...
---------------------------------------------------------------------------------------*/

_PUBLIC int cantor_rulefile_from_clustered_link(char      *filename,  // Filename for rgen file
                                                int       n_links,    // Number of link lists
                                                link_type **links)    // List of link lists

{  int i,
       j,
       k;

   node_type **routelist = (node_type **)NULL;

   FILE *stream = (FILE *)NULL;

   char rule_filename[SSIZE] = "",
        *rgen_pathname       = (char *)NULL,
        *cluster_pathname    = (char *)NULL;
  
   if(filename == (char *)NULL || n_links == 0 || links == (link_type **)NULL)
   {  pups_set_errno(EINVAL);
      return(-1);
   }


   /*---------------------------*/
   /* Filename (pairwise rules) */
   /*---------------------------*/
 
   if(filename == (char *)NULL)
      (void)snprintf(rule_filename,SSIZE,"cantor.%d.%s.rules",appl_pid,appl_host);
   else
      (void)strlcpy(rule_filename,filename,SSIZE);

   if(access(rule_filename,F_OK | R_OK | W_OK) == (-1))
     (void)pups_creat(rule_filename,0600);

   stream = pups_fopen(rule_filename,"w",LIVE);
 
   (void)fprintf(stream,"#----------------------------------------------------------------------------------\n");
   (void)fprintf(stream,"#  Rgen (binary) rule file automatically generated from link data by\n");
   (void)fprintf(stream,"#  Cantor version %s, (C) M.A. O'Neill, Tumbling Dice, 2022\n",CANTOR_VERSION);
   (void)fprintf(stream,"#-----------------------------------------------------------------------------------\n\n");
   (void)fprintf(stream,"\n\n#  Format is node1, node2, link, link_flux, link_length\n\n");
   (void)fflush(stream);


   /*--------------------------------------------------------------------*/
   /* Process each link generating a set of binary connection rule pairs */
   /* for rgen rule convertor                                            */
   /*--------------------------------------------------------------------*/

   for(i=0; i<n_links; ++i)
   { 

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_lock(&links[i]->mutex);
      #endif /* PTHREAD_SUPPORT */

      if(links[i] != (link_type *)NULL && (links[i] = (link_type *)cantor_pointer_live(links[i],TRUE)) != (link_type *)NULL)
      {  for(j=0; j<links[i]->node_alloc; ++j)
         {

            routelist = links[i]->routelist;

            if(routelist[j] != (node_type *)NULL && (routelist[j] = (node_type *)cantor_pointer_live(routelist[j],TRUE)) != (node_type *)NULL)
            {  for(k=j+1; k<links[i]->node_alloc; ++k) 
               {  if(routelist[k] != (node_type *)NULL && (routelist[k] = (node_type *)cantor_pointer_live(routelist[k],TRUE)) != (node_type *)NULL)


                  /*--------------------------------------*/
                  /* We have live data -- lets extract it */
                  /*--------------------------------------*/

                  (void)fprintf(stream,"%-16s %-16s %4d %6.3F %6.3F\n",routelist[j],routelist[k],(i+1),links[i]->flux,links[i]->length);
                  (void)fflush(stream);
               }
            }
         }
      }

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&links[i]->mutex);
      #endif /* PTHREAD_SUPPORT */

    }
    (void)pups_fclose(stream);

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------------------------------------
    Intersection of a pair of links ...
------------------------------------------------------------------------------*/


/*-------------------------------------*/
/* Desintation linklists on local heap */
/*-------------------------------------*/

_PUBLIC int cantor_link_intersection(char      *name,     // Name of intersection set1
                                     link_type *link_1,   // Insersection of link set2 and set3
                                     link_type *link_2,   // Set2
                                     link_type *link_3)   // Set3

{   int i;

    if(name == (char *)NULL ||
       link_1 == (link_type *)NULL ||
       link_2 == (link_type *)NULL ||
       link_3 == (link_type *)NULL  )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_link  (link_1);
    (void)cantor_set_link_tag(link_1, name);
    (void)cantor_set_link_type(link_1,"intersection");

    for(i=0; i<link_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_3->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL      &&
             link_3->routelist[j] != (node_type *)NULL      &&
             link_2->routelist[i] == link_3->routelist[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)       
          (void)cantor_add_node_to_link(link_1,link_2->routelist[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link_1->mutex);
    (void)pthread_mutex_unlock(&link_2->mutex);
    (void)pthread_mutex_unlock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------*/
/* Destination linklist on persistent heap */
/*-----------------------------------------*/

_PUBLIC int cantor_phlink_intersection(int       hdes,     // Heap descriptor set1
                                       char      *name,    // name of intersection set1
                                       link_type *link_1,  // Insersection of set2 and set3
                                       link_type *link_2,  // Set2
                                       link_type *link_3)  // Set3

{   int i;

    if(name   == (char *)NULL       ||
       link_1 == (link_type *)NULL  ||
       link_2 == (link_type *)NULL  ||
       link_3 == (link_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_link(link_1,hdes);
    (void)cantor_set_link_tag(link_1,name);
    (void)cantor_set_link_type(link_1,"intersection");

    for(i=0; i<link_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_3->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL      &&
             link_3->routelist[j] != (node_type *)NULL      &&
             link_2->routelist[i] == link_3->routelist[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          (void)cantor_phadd_node_to_link(link_1,link_2->routelist[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link_1->mutex);
    (void)pthread_mutex_unlock(&link_2->mutex);
    (void)pthread_mutex_unlock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    Union of a pair of links ...
------------------------------------------------------------------------------*/


/*-----------------------------------*/
/* Desination linklist on local heap */
/*-----------------------------------*/

_PUBLIC int cantor_link_union(char      *name,    // Name of union set1
                              link_type *link_1,  // Union of set2 & set3
                              link_type *link_2,  // Set2
                              link_type *link_3)  // Set3

{   int i;

    if(name == (char *)NULL || link_1 == (link_type *)NULL || link_2 == (link_type *)NULL || link_3 == (link_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_link(link_1);
    (void)cantor_set_link_tag(link_1,name);
    (void)cantor_set_link_type(link_1,"union");

    for(i=0; i<link_2->node_alloc; ++i)
    {  if(link_2->routelist[i] != (node_type *)NULL)
          (void)cantor_add_node_to_link(link_1,link_2->routelist[i]);
    }

    for(i=0; i<link_2->node_alloc; ++i)
    {  if(link_3->routelist[i] != (node_type *)NULL)
          (void)cantor_add_node_to_link(link_1,link_3->routelist[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link_1->mutex);
    (void)pthread_mutex_unlock(&link_2->mutex);
    (void)pthread_mutex_unlock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*------------------------------------------*/
/* Destination rlLinklist on peristent heap */
/*------------------------------------------*/

_PUBLIC int cantor_phlink_union(int          hdes,  // Heap descriptor set1
                                char        *name,  // Name of set1
                                link_type *link_1,  // Unions of set2 and set3
                                link_type *link_2,  // Set2
                                link_type *link_3)  // Set3

{   int i;

    if(name == (char *)NULL         ||
       link_1 == (link_type *)NULL  ||
       link_2 == (link_type *)NULL  ||
       link_3 == (link_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_link(link_1,hdes);
    (void)cantor_set_link_tag(link_1,name);
    (void)cantor_set_link_type(link_1,"union");

    for(i=0; i<link_2->node_alloc; ++i)
    {  if(link_2->routelist[i] != (node_type *)NULL)
          (void)cantor_phadd_node_to_link(link_1,link_2->routelist[i],hdes,name);
    }

    for(i=0; i<link_2->node_alloc; ++i)
    {  if(link_3->routelist[i] != (node_type *)NULL)
          (void)cantor_phadd_node_to_link(link_1,link_3->routelist[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link_1->mutex);
    (void)pthread_mutex_unlock(&link_2->mutex);
    (void)pthread_mutex_unlock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    Difference of a pair of links ...
------------------------------------------------------------------------------*/


/*-------------------------------------*/
/* Destination linklists on local heap */
/*-------------------------------------*/

_PUBLIC int cantor_link_difference(char      *name,    // Name of difference set1
                                   link_type *link_1,  // Difference of set2 and set3
                                   link_type *link_2,  // Set2
                                   link_type *link_3)  // Set3

{   int i;

    if(name   == (char *)NULL       ||
       link_1 == (link_type *)NULL  ||
       link_2 == (link_type *)NULL  ||
       link_3 == (link_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_link(link_1);
    (void)cantor_set_link_tag(link_1,name);
    (void)cantor_set_link_type(link_1,"difference");

    for(i=0; i<link_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_3->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL                  &&
             link_3->routelist[j] != (node_type *)NULL                  &&
             link_2->routelist[i]->name == link_3->routelist[j]->name    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_node_to_link(link_1,link_2->routelist[i]);
    }

    for(i=0; i<link_3->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_2->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL                  &&
             link_3->routelist[j] != (node_type *)NULL                  &&
             link_2->routelist[i]->name == link_3->routelist[j]->name    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_node_to_link(link_1,link_3->routelist[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




#ifdef PERSISTENT_HEAP_SUPPORT
/*----------------------------------------*/
/* Destination linklist on pesistent heap */
/*----------------------------------------*/

_PUBLIC int cantor_phlink_difference(int       hdes,     // Heap descriptor for set1
                                     char      *name,    // Name of set1
                                     link_type *link_1,  // Difference of set2 and set3
                                     link_type *link_2,  // Set2
                                     link_type *link_3)  // Set3

{   int i;

    if(name   == (char *)NULL       ||
       link_1 == (link_type *)NULL  ||
       link_2 == (link_type *)NULL  ||
       link_3 == (link_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&link_1->mutex);
    (void)pthread_mutex_lock(&link_2->mutex);
    (void)pthread_mutex_lock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_link(link_1,hdes);
    (void)cantor_set_link_tag(link_1,name);
    (void)cantor_set_link_type(link_1,"difference");

    for(i=0; i<link_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_3->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL      &&
             link_3->routelist[j] != (node_type *)NULL      &&
             link_2->routelist[i] == link_3->routelist[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_phadd_node_to_link(link_1,link_2->routelist[i],hdes,name);
    }

    for(i=0; i<link_3->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<link_2->node_alloc; ++j)
       {  if(link_2->routelist[i] != (node_type *)NULL                  &&
             link_3->routelist[j] != (node_type *)NULL                  &&
             link_2->routelist[i]->name == link_3->routelist[j]->name    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_phadd_node_to_link(link_1,link_3->routelist[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&link_1->mutex);
    (void)pthread_mutex_unlock(&link_2->mutex);
    (void)pthread_mutex_unlock(&link_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Object) intersection of a pair of nodes ...
------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Destination node on local heap */
/*--------------------------------*/

_PUBLIC int cantor_object_intersection(char      *name,    // Name of intersection node1      
                                       node_type *node_1,  // Intersection of nodes1 and node2
                                       node_type *node_2,  // Node2
                                       node_type *node_3)  // Node3

{   int i;

    if(name   == (char *)NULL       ||
       node_1 == (node_type *)NULL  ||
       node_2 == (node_type *)NULL  ||
       node_3 == (node_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_node(node_1);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"intersection");

    for(i=0; i<node_2->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_3->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          (void)cantor_add_object_to_node(node_1,node_2->objects[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node_1->mutex);
    (void)pthread_mutex_unlock(&node_2->mutex);
    (void)pthread_mutex_unlock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------------------*/
/* Desitnation node on persistent heap */
/*-------------------------------------*/

_PUBLIC int cantor_phobject_intersection(int        hdes,    // Heap descriptor for node1
                                         char      *name,    // Name of node1
                                         node_type *node_1,  // Intersection of node2 and node3
                                         node_type *node_2,  // Node2
                                         node_type *node_3)  // Node3

{   int i;

    if(name   == (char *)NULL       ||
       node_1 == (node_type *)NULL  ||
       node_2 == (node_type *)NULL  ||
       node_3 == (node_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_node(node_1,hdes);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"union");

    for(i=0; i<node_2->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_3->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          (void)cantor_phadd_object_to_node(node_1,node_2->objects[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node_1->mutex);
    (void)pthread_mutex_unlock(&node_2->mutex);
    (void)pthread_mutex_unlock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Object) union  of a pair of nodes ...
------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Destination node on local heap */
/*--------------------------------*/

_PUBLIC int cantor_object_union(char      *name,    // Name of node1
                                node_type *node_1,  // Union of node1 and node2
                                node_type *node_2,  // Node2
                                node_type *node_3)  // Node3

{   int i;

    if(name   == (char *)NULL       ||
       node_1 == (node_type *)NULL  ||
       node_2 == (node_type *)NULL  ||
       node_3 == (node_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_node(node_1);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"union");

    for(i=0; i<node_2->object_alloc; ++i)
    {  if(node_2->objects[i] != (object_type *)NULL)
          (void)cantor_add_object_to_node(node_1,node_2->objects[i]);
    }

    for(i=0; i<node_3->object_alloc; ++i)
    {  if(node_3->objects[i] != (object_type *)NULL)
          (void)cantor_add_object_to_node(node_1,node_3->objects[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node_1->mutex);
    (void)pthread_mutex_unlock(&node_2->mutex);
    (void)pthread_mutex_unlock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*--------------------------------------*/
/* Desintantion node on persistent heap */
/*--------------------------------------*/

_PUBLIC int cantor_phobject_union(int        hdes,    // Heap descriptor for node1
                                  char      *name,    // Name of node1
                                  node_type *node_1,  // Union of node2 and node3
                                  node_type *node_2,  // Node2
                                  node_type *node_3)  // Node3

{   int i,
        j;

    _BOOLEAN  matched;
    link_type *ret = (link_type *)NULL;

    if(name == (char *)NULL || node_1 == (node_type *)NULL || node_2 == (node_type *)NULL || node_3 == (node_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_node(node_1,hdes);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"union");

    for(i=0; i<node_2->object_alloc; ++i)
    {  if(node_2->objects[i] != (object_type *)NULL)
          (void)cantor_phadd_object_to_node(node_1,node_2->objects[i],hdes,name);
    }

    for(i=0; i<node_3->object_alloc; ++i)
    {  if(node_3->objects[i] != (object_type *)NULL)
          (void)cantor_phadd_object_to_node(node_1,node_3->objects[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node_1->mutex);
    (void)pthread_mutex_unlock(&node_2->mutex);
    (void)pthread_mutex_unlock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Object) difference of a pair of nodes ...
------------------------------------------------------------------------------*/


/*--------------------------------*/
/* Destination node on local heap */
/*--------------------------------*/

_PUBLIC int cantor_object_difference(char      *name,    // Name of difference node1
                                     node_type *node_1,  // Difference of node2 and node3
                                     node_type *node_2,  // Node2
                                     node_type *node_3)  // Node3

{   int i;

    if(name   == (char *)NULL       ||
       node_1 == (node_type *)NULL  ||
       node_2 == (node_type *)NULL  ||
       node_3 == (node_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_node(node_1);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"difference");

    for(i=0; i<node_2->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_3->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_object_to_node(node_1,node_2->objects[i]);
    }

    for(i=0; i<node_3->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_2->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_object_to_node(node_1,node_3->objects[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&node_1->mutex);
    (void)pthread_mutex_unlock(&node_2->mutex);
    (void)pthread_mutex_unlock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-------------------------------------*/
/* Destination node on persistent heap */
/*-------------------------------------*/

_PUBLIC int cantor_phobject_difference(int        hdes,    // Heap descriptor for node1
                                       char      *name,    // Name of node1
                                       node_type *node_1,  // Difference of node2 and node3
                                       node_type *node_2,  // Node2
                                       node_type *node_3)  // Node3

{   int i;

    if(name   == (char *)NULL       ||
       node_1 == (node_type *)NULL  ||
       node_2 == (node_type *)NULL  ||
       node_3 == (node_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_node(node_1,hdes);
    (void)cantor_set_node_name(node_1,name);
    (void)cantor_set_node_type(node_1,"difference");

    for(i=0; i<node_2->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_3->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_phadd_object_to_node(node_1,node_2->objects[i],hdes,name);
    }

    for(i=0; i<node_3->object_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<node_2->object_alloc; ++j)
       {  if(node_2->objects[i] != (object_type *)NULL   &&
             node_3->objects[j] != (object_type *)NULL   &&
             node_2->objects[i] ==  node_3->objects[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_phadd_object_to_node(node_1,node_3->objects[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&node_1->mutex);
    (void)pthread_mutex_lock(&node_2->mutex);
    (void)pthread_mutex_lock(&node_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Node) intersection of a pair of nodelists ...
------------------------------------------------------------------------------*/


/*------------------------------------*/
/* Destination nodelist on local heap */
/*------------------------------------*/

_PUBLIC int nodelist_intersection(char          *name,        // Name of nodelist1
                                  nodelist_type *nodelist_1,  // Intersection of nodelist1 and nodelist2
                                  nodelist_type *nodelist_2,  // Nodelist2
                                  nodelist_type *nodelist_3)  // Nodelist3

{   int i;

    if(name       == (char *)NULL           ||
       nodelist_1 == (nodelist_type *)NULL  ||
       nodelist_2 == (nodelist_type *)NULL  ||
       nodelist_3 == (nodelist_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_nodelist(nodelist_1);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"intersection");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<nodelist_3->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL        &&
             nodelist_3->nodes[j] != (node_type *)NULL        &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          (void)cantor_add_node_to_nodelist(nodelist_1,nodelist_2->nodes[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------*/
/* Desitnation nodelist on persistent heap */
/*-----------------------------------------*/

_PUBLIC int cantor_phnodelist_intersection(int            hdes,        // Heap descriptor for nodelist1
                                           char          *name,        // Name of nodelist1
                                           nodelist_type *nodelist_1,  // Intersection of nodelist2 and nodelist3
                                           nodelist_type *nodelist_2,  // Nodelist2
                                           nodelist_type *nodelist_3)  // Nodelist3

{   int i;

    if(name       == (char *)NULL           ||
       nodelist_1 == (nodelist_type *)NULL  ||
       nodelist_2 == (nodelist_type *)NULL  ||
       nodelist_3 == (nodelist_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_nodelist(nodelist_1,hdes);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"intersection");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<nodelist_3->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL        &&
             nodelist_3->nodes[j] != (node_type *)NULL        &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          cantor_phadd_node_to_nodelist(nodelist_1,nodelist_2->nodes[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Node) union of a pair of nodelists ...
------------------------------------------------------------------------------*/


/*-------------------------*/
/* Nodelists on local heap */
/*-------------------------*/

_PUBLIC int nodelist_union(char          *name,        // Name of nodelist1
                           nodelist_type *nodelist_1,  // Intersection of nodelist2 and nodelist3
                           nodelist_type *nodelist_2,  // Nodelist2
                           nodelist_type *nodelist_3)  // Nodelist3

{   int i,
        j;

    if(name == (char *)NULL || nodelist_1 == (nodelist_type *)NULL || nodelist_2 == (nodelist_type *)NULL || nodelist_3 == (nodelist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_nodelist(nodelist_1);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"union");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  if(nodelist_2->nodes[i] != (node_type *)NULL)
          (void)cantor_add_node_to_nodelist(nodelist_1,nodelist_2->nodes[i]);
    }

    for(i=0; i<nodelist_3->node_alloc; ++i)
    {  if(nodelist_3->nodes[i] != (node_type *)NULL)
          (void)cantor_add_node_to_nodelist(nodelist_1,nodelist_3->nodes[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------*/
/* Destination nodelist on persistent heap */
/*-----------------------------------------*/

_PUBLIC int cantor_phnodelist_union(int            hdes,        // Heap descriptor for nodelist1
                                    char          *name,        // Name of nodelist1
                                    nodelist_type *nodelist_1,  // Union of nodelist2 and nodelist3
                                    nodelist_type *nodelist_2,  // Nodelist2
                                    nodelist_type *nodelist_3)  // Nodelist3

{   int i;

    if(name       == (char *)NULL           ||
       nodelist_1 == (nodelist_type *)NULL  ||
       nodelist_2 == (nodelist_type *)NULL  ||
       nodelist_3 == (nodelist_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_nodelist(nodelist_1,hdes);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"union");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  if(nodelist_2->nodes[i] != (node_type *)NULL)
          (void)cantor_phadd_node_to_nodelist(nodelist_1,nodelist_2->nodes[i],hdes,name);
    }

    for(i=0; i<nodelist_3->node_alloc; ++i)
    {  if(nodelist_3->nodes[i] != (node_type *)NULL)
          (void)cantor_phadd_node_to_nodelist(nodelist_1,nodelist_3->nodes[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------------------------------
    (Node) difference of a pair of nodelists ...
------------------------------------------------------------------------------*/


/*-----------------------------------*/
/* Desination Nodelist on local heap */
/*-----------------------------------*/

_PUBLIC int cantor_nodelist_difference(char          *name,        // Name of nodelist1
                                       nodelist_type *nodelist_1,  // Difference of nodelist2 and nodelist3
                                       nodelist_type *nodelist_2,  // Nodelist2
                                       nodelist_type *nodelist_3)  // Nodelist3

{   int i;

    if(name       == (char *)NULL           ||
       nodelist_1 == (nodelist_type *)NULL  ||
       nodelist_2 == (nodelist_type *)NULL  ||
       nodelist_3 == (nodelist_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_clear_nodelist(nodelist_1);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"difference");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<nodelist_3->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL        &&
             nodelist_3->nodes[j] != (node_type *)NULL        &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_node_to_nodelist(nodelist_1,nodelist_2->nodes[i]);
    }

    for(i=0; i<nodelist_3->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<nodelist_2->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL        &&
             nodelist_3->nodes[j] != (node_type *)NULL        &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_add_node_to_nodelist(nodelist_1,nodelist_2->nodes[i]);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}


#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------*/
/* Destination nodelist on persistent heap */
/*-----------------------------------------*/

_PUBLIC int cantor_phnodelist_difference(int            hdes,        // Heap descriptor nodelist1
                                         char          *name,        // Name of nodelist1
                                         nodelist_type *nodelist_1,  // Difference of nodelist2 and nodelist3
                                         nodelist_type *nodelist_2,  // Nodelist2
                                         nodelist_type *nodelist_3)  // Nodelist3

{   int i;

    if(name       == (char *)NULL           ||
       nodelist_1 == (nodelist_type *)NULL  ||
       nodelist_2 == (nodelist_type *)NULL  ||
       nodelist_3 == (nodelist_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&nodelist_1->mutex);
    (void)pthread_mutex_lock(&nodelist_2->mutex);
    (void)pthread_mutex_lock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)cantor_phclear_nodelist(nodelist_1,hdes);
    (void)cantor_set_nodelist_name(nodelist_1,name);
    (void)cantor_set_nodelist_type(nodelist_1,"union");

    for(i=0; i<nodelist_2->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=i; j<nodelist_3->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL       &&
             nodelist_3->nodes[j] != (node_type *)NULL       &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == TRUE)
          (void)cantor_phadd_node_to_nodelist(nodelist_1,nodelist_2->nodes[i],hdes,name);
    }

    for(i=0; i<nodelist_3->node_alloc; ++i)
    {  int      j;
       _BOOLEAN matched = FALSE;

       for(j=0; j<nodelist_2->node_alloc; ++j)
       {  if(nodelist_2->nodes[i] != (node_type *)NULL       &&
             nodelist_3->nodes[j] != (node_type *)NULL       &&
             nodelist_2->nodes[i] ==  nodelist_3->nodes[j]    )
             matched = TRUE;
       }

       if(matched == FALSE)
          (void)cantor_phadd_node_to_nodelist(nodelist_1,nodelist_2->nodes[i],hdes,name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&nodelist_1->mutex);
    (void)pthread_mutex_unlock(&nodelist_2->mutex);
    (void)pthread_mutex_unlock(&nodelist_3->mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
#endif /* PERSISTENT_HEAP_SUPPORT */
