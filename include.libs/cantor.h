/*------------------------------------------------------------------------------
    Purpose: Header for Cantor cellular database library. 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.02 
    Dated:   27th February 2024
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef CANTOR_H
#define CANTOR_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/*-------------------------------------------------------------------------------
    Defines usd by the Cantor core library ...
-------------------------------------------------------------------------------*/
/***********/
/* Version */
/***********/

#define CANTOR_VERSION        "3.02"


/*----------------------------------*/
/* Header which need to be included */
/*----------------------------------*/

#include <xtypes.h>
#include <ftype.h>


/*---------------------------------------*/
/* Datastructures defined by this module */
/*---------------------------------------*/
/*-------------*/
/* String size */
/*-------------*/

#define SSIZE 2048


/*-------------------------------------*/
/* Definition of a node in the network */
/*-------------------------------------*/

typedef struct ltype link_type;
typedef struct otype object_type;

typedef struct {   char         name[SSIZE];         // Name of network node
                   char         type[SSIZE];         // Type of node
                   uint32_t     object_alloc;        // Number of object slots allocated to this node
                   uint32_t     object_cnt;          // Number of objects associated with this node
                   object_type  **objects;           // Node objects (if any)
                   uint32_t     link_alloc;          // Number of link slots allocated to this node 
                   uint32_t     link_cnt;            // Number of links associated with this node/
                   link_type    **links;             // Connected objects which reference this node

                   #ifdef PERSISTENT_HEAP_SUPPORT
                   char         link_name[SSIZE];    // Handle to links array on persistent heap
                   char         object_name[SSIZE];  // Handle to objects array on persistent heap 
                   #endif /* pupsp3-4.0.0.src.old     _HEAP_SUPPORT */

                   #ifdef PTHREAD_SUPPORT
                   pthread_mutex_t mutex;            // Node access mutex 
                   #endif /* PTHREAD_SUPPORT */
               } node_type;




/*-----------------------------*/
/* Definition of list of nodes */
/*-----------------------------*/

typedef struct {   char         name[SSIZE];         // Name of nodelist
                   char         type[SSIZE];         // Type of nodelist
                   uint32_t     node_alloc;          // Number of node slots in nodelist
                   uint32_t     node_cnt;            // Number of nodes in nodelist
                   node_type    **nodes;             // Node list

                   #ifdef PERSISTENT_HEAP_SUPPORT
                   char         node_name[SSIZE];    // Handle to nodes array on persistent heap 
                   #endif /* PERSISTENT_HEAP_SUPPORT */

                   #ifdef PTHREAD_SUPPORT
                   pthread_mutex_t mutex;            // Node list access mutex 
                   #endif /* PTHREAD_SUPPORT */
               } nodelist_type;




/*------------------------------------------*/
/* Definition of a link through the network */
/*------------------------------------------*/

struct ltype {  char         tag[SSIZE];             // Label identifying link
                char         type[SSIZE];            // Type of link 
                uint32_t     hdes;                   // Shared heap handle of link 
                FTYPE        length;                 // Physical size of link 
                FTYPE        flux;                   // Flux through this link from source 
                uint32_t     node_alloc;             // Slots allocated to routelist 
                uint32_t     node_cnt;               // Length of routelist (in nodes) 
                node_type    **routelist;            // Nodes in this link 

                #ifdef PERSISTENT_HEAP_SUPPORT
                char         node_name[SSIZE];       // Handle to routelist array on persistent heap 
                #endif /* PERSISTENT_HEAP_SUPPORT */

                #ifdef PTHREAD_SUPPORT
                pthread_mutex_t mutex;               // Link access mutex 
                #endif /* PTHREAD_SUPPORT */
             };   




/*---------------------------------------------------------*/
/* Definition of an object which may be attached to a node */
/*---------------------------------------------------------*/

struct otype {   char          tag[SSIZE];         // Tag which identifies this object 
                 char          type[SSIZE];        // Type of object (data function etc)
                 uint32_t      hdes;               // Shared heap handle of object 
                 char          infopage[SSIZE];    // HTML page which described object (if any) 
                 void          **olink;            // Link to the object (function or data) 

                 #ifdef PERSISTENT_HEAP_SUPPORT
                 char          object_name[SSIZE]; // Handle to object array on persistent heap
                 #endif /* PERSISTENT_HEAP_SUPPORT */

                 #ifdef PTHREAD_SUPPORT
                 pthread_mutex_t mutex;            // Object access mutex 
                 #endif /* PTHREAD_SUPPORT */
             };


#ifdef _NOT_LIB_SOURCE_                   

#else
#   undef  _EXPORT
#   define _EXPORT
#endif /* _NOT_LIB_SOURCE_ */



/*-----------------------------------*/
/* Functions exported by this module */
/*-----------------------------------*/

#ifdef PTHREAD_SUPPORT
// Initialise node mutex (recursive) 
_PROTOTYPE _EXPORT void cantor_init_node_mutex(node_type *);

// Initialise object mutex (recursive)
_PROTOTYPE _EXPORT void cantor_init_object_mutex(object_type *);

// Initialise list mutex (recursive)
_PROTOTYPE _EXPORT void cantor_init_link_mutex(link_type *);

// Initialise nodelist mutex (recursive)
_PROTOTYPE _EXPORT void cantor_init_nodelist_mutex(nodelist_type *);
#endif /* PTHREAD_SUPPORT */


// Get node index (given node ame)
_PROTOTYPE _EXPORT int32_t cantor_get_node_index_from_name(nodelist_type *, const char *);

// Clear node
_PROTOTYPE _EXPORT node_type *cantor_clear_node(node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear node
_PROTOTYPE _EXPORT node_type *cantor_phclear_node(node_type *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Clear node list 
_PROTOTYPE _EXPORT nodelist_type *cantor_clear_nodelist(nodelist_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear node list
_PROTOTYPE _EXPORT nodelist_type *cantor_phclear_nodelist(nodelist_type *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Add node to nodelist
_PROTOTYPE _EXPORT int32_t cantor_add_node_to_nodelist(nodelist_type *, const node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Add node to nodelist
_PROTOTYPE _EXPORT int32_t cantor_phadd_node_to_nodelist(nodelist_type *, const node_type *, const uint32_t, const char *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Remove node from nodelist
_PROTOTYPE _EXPORT int32_t cantor_remove_node_from_nodelist(nodelist_type *, const  char *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Remove node from nodelist
_PROTOTYPE _EXPORT int32_t cantor_phremove_node_from_nodelist(nodelist_type *, const char *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Set nodelist name
_PROTOTYPE _EXPORT int32_t cantor_set_nodelist_name(nodelist_type *, const char *);

// Set nodelist type
_PROTOTYPE _EXPORT int32_t cantor_set_nodelist_type(nodelist_type *, const char *);

// Add object to node list
_PROTOTYPE _EXPORT int32_t cantor_add_object_to_node(node_type *, const object_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Add object to node list
_PROTOTYPE _EXPORT int32_t cantor_phadd_object_to_node(node_type *, const object_type *, const uint32_t, const char *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Remove object from node
_PROTOTYPE _EXPORT int32_t remove_object_from_node(node_type *, const char *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Remove object from node
_PROTOTYPE _EXPORT int32_t cantor_phremove_object_from_node(node_type *, const char *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Is node name unique?
_PROTOTYPE _EXPORT _BOOLEAN cantor_node_name_unique(nodelist_type *, const char *);

// Is object tag unique?
_PROTOTYPE _EXPORT _BOOLEAN cantor_object_tag_unique(node_type *, const char *);

// Get index of object (in node object table) given object name 
_PROTOTYPE _EXPORT int32_t cantor_get_object_index_from_tag(node_type *, const char *);

// Get object tag (given index in node object table)
_PROTOTYPE _EXPORT int32_t cantor_get_object_tag_from_index(char *, const node_type *, const uint32_t);

// Add a link (inter-node link) to a node
_PROTOTYPE _EXPORT int32_t cantor_add_link_to_node(node_type *, const link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Add a link (inter-node link) to a node
_PROTOTYPE _EXPORT  int32_t cantor_phadd_link_to_node(node_type *, const link_type *, const uint32_t, const char *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Remove a link from a node (using linkweay tag to identify linkway to be removed)
_PROTOTYPE _EXPORT int32_t cantor_remove_link_from_node(node_type *, const char *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Remove a link from a node (using linkweay tag to identify link to be removed)
_PROTOTYPE _EXPORT int32_t cantor_phremove_link_from_node(node_type *, const char *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Is link i.d. tag unique
_PROTOTYPE _EXPORT _BOOLEAN cantor_link_tag_unique(node_type *, const char *);

// Get index  into node link table (using link tag to ientify link required)
_PROTOTYPE _EXPORT int32_t cantor_get_link_index_from_tag(node_type *, const char *);

// Get link tag (given index into node link table)
_PROTOTYPE _EXPORT int32_t cantor_get_link_tag_from_index(char *, node_type *, const uint32_t);

// Set node name
_PROTOTYPE _EXPORT int32_t cantor_set_node_name(node_type *, const char *);

// Set node type
_PROTOTYPE _EXPORT int32_t cantor_set_node_type(node_type *, const char *);

// Print (limited) information about a node [root thread]
_PROTOTYPE _EXPORT int32_t cantor_print_node_info(const FILE *, node_type *);

// Generate VHTML page describing node [root thread]
_PROTOTYPE _EXPORT int32_t cantor_vhtml_node_info(const _BOOLEAN, const char *, const char *, node_type *);

// Clear a link (local process heap)
_PROTOTYPE _EXPORT link_type *cantor_clear_link(link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear a link (persistent heap)
_PROTOTYPE _EXPORT link_type *cantor_phclear_link(link_type *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Clear a list of links
_PROTOTYPE _EXPORT link_type **cantor_clear_linklist(link_type **, const uint32_t);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear a link (persistent heap)
_PROTOTYPE _EXPORT link_type **cantor_phclear_linklist(link_type **, const uint32_t, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Add a node to an existing link
_PROTOTYPE _EXPORT int32_t cantor_add_node_to_link(link_type *, const node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Add a node to an existing link
_PROTOTYPE _EXPORT int32_t cantor_phadd_node_to_link(link_type *, const node_type *, const uint32_t, const char *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Remove node (from existing link)
_PROTOTYPE _EXPORT int32_t cantor_remove_node_from_link(link_type *, const char *);

// Show link information (print to file) [root thread]
_PROTOTYPE _EXPORT  void cantor_show_link_info(const FILE *, link_type *);

// Copy (process heap) nodelist
_PROTOTYPE _EXPORT int32_t cantor_copy_nodelist(const char *, nodelist_type *, nodelist_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Copy (persistent heap) nodelist
_PROTOTYPE _EXPORT int32_t cantor_phcopy_nodelist(const uint32_t, const char *, nodelist_type *, nodelist_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */:/show_link_info


// Copy (process heap) node
_PROTOTYPE _EXPORT int32_t cantor_copy_node(const char *, node_type *, node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Copy (persistent heap) node
_PROTOTYPE _EXPORT int32_t cantor_phcopy_node(const uint32_t, const char *, node_type *, node_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Copy (process heap) link
_PROTOTYPE _EXPORT int32_t cantor_copy_link(const char *, link_type *, link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Copy a (persistent heap) link
_PROTOTYPE _EXPORT int32_t cantor_phcopy_link(const uint32_t, const char *, link_type *, link_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Set link tag
_PROTOTYPE _EXPORT int32_t cantor_set_link_tag(link_type *, const char *);

// Set link type
_PROTOTYPE _EXPORT int32_t cantor_set_link_type(link_type *, const char *);

// Set link flux
_PROTOTYPE _EXPORT int32_t cantor_set_link_flux(link_type *, const FTYPE);

// Set link length
_PROTOTYPE _EXPORT int32_t cantor_set_link_length(link_type *,const  FTYPE);

// Compare a pair of nodes
_PROTOTYPE _EXPORT _BOOLEAN cantor_compare_nodes(node_type *, node_type *);

// Clear object (removing it node objectlist)
_PROTOTYPE _EXPORT object_type *cantor_clear_object(object_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear object (removing it node objectlist)
_PROTOTYPE _EXPORT object_type *cantor_phclear_object(object_type *, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Clear an object list (removing it from node)
_PROTOTYPE _EXPORT object_type **cantor_clear_objectlist(object_type **, const uint32_t);


#ifdef PERSISTENT_HEAP_SUPPORT
// Clear an object list (removing it from node)
_PROTOTYPE _EXPORT object_type **cantor_phclear_objectlist(object_type **, const uint32_t, const uint32_t);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Set object name
_PROTOTYPE _EXPORT int32_t cantor_set_object_name(object_type *, const char *);

// Set object type
_PROTOTYPE _EXPORT int32_t cantor_set_object_type(object_type *, const char *);

// Set object link
_PROTOTYPE _EXPORT int32_t cantor_set_olink(object_type *, const void *);

// Is pointer live?
_PROTOTYPE _EXPORT void *cantor_pointer_live(const void *, const _BOOLEAN);

// Print nodes in link
_PROTOTYPE _EXPORT void cantor_print_link_info(FILE *, link_type *);

// Generate VHTML page describing link [root thread]
_PROTOTYPE _EXPORT int32_t cantor_vhtml_link_info(const char *, const char *, link_type *);

// Generate VHTML (and browse it) [root thread]
_PROTOTYPE _EXPORT int32_t cantor_vhtml_node_browse(const _BOOLEAN, const char *, const node_type *);

// Generate VHTML (and browse it)
_PROTOTYPE _EXPORT int32_t cantor_vhtml_nodelist_browse(const _BOOLEAN, const char *, nodelist_type *);

// Generate (rgen) rule file from link data
_PROTOTYPE _EXPORT int32_t cantor_rulefile_from_clustered_link(const char *,const uint32_t, link_type **);

// Intersection of (local process heap) link
_PROTOTYPE _EXPORT int32_t cantor_link_intersection(const char *, link_type *, link_type *, link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Intersection of (persistent heap) link
_PROTOTYPE _EXPORT int32_t cantor_phlink_intersection(const uint32_t, const char *, link_type *, link_type *, link_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Union of (local process heap) linkways
_PROTOTYPE _EXPORT int32_t cantor_link_union(const char *, link_type *, link_type *, link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Union of (persistent heap) linkways
_PROTOTYPE _EXPORT int32_t cantor_phlink_union(const uint32_t, const char *, link_type *, link_type *, link_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Difference of (local process heap) linkways
_PROTOTYPE _EXPORT int32_t cantor_link_difference(const char *, link_type *, link_type *, link_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Difference of (persistent heap) linkways
_PROTOTYPE _EXPORT int32_t cantor_phlink_difference(const uint32_t, const char *, link_type *, link_type *, link_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Object  intersection of (local process heap) nodes
_PROTOTYPE _EXPORT int32_t cantor_object_intersection(const char *, node_type *, node_type *, node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Object  intersection of (persistent heap) nodes
_PROTOTYPE _EXPORT  int32_t cantor_phobject_intersection(const uint32_t, const char *, node_type *, node_type *, node_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Object union of (local process heap) nodes
_PROTOTYPE _EXPORT int32_t cantor_object_union(const char *, node_type *, node_type *, node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Object union of (persistent heap) nodes
_PROTOTYPE _EXPORT int32_t cantor_phobject_union(const uint32_t, const char *, node_type *, node_type *, node_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Object difference of (local process heap) nodes */
_PROTOTYPE _EXPORT int32_t cantor_object_difference(const char *, node_type *, node_type *, node_type *);


#ifdef PERSISTENT_HEAP_SUPPORT
// Object difference of (persistent heap) nodes
_PROTOTYPE _EXPORT int32_t cantor_phobject_difference(const uint32_t, const char *, node_type *, node_type *, node_type *);
#endif /* PERSISTENT_HEAP_SUPPORT */

#endif /* CANTOR_HDR */
