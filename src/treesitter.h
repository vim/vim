/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * treesitter.h: Treesitter related definitions
 */

#ifdef FEAT_TREESITTER

#include "vim.h"
#include <tree_sitter/api.h>

typedef struct vts_tree_S vts_tree_T;
typedef struct vts_node_S vts_node_T;
typedef struct vts_parser_S vts_parser_T;

struct vts_tree_S
{
    int			tt_refcount;
    TSTree	    	*tt_tree;
}_tree_T;

struct vts_node_S
{
    int			tn_refcount;
    vts_tree_T	    	*tn_tree;	// Parent TSTree object, we hold our own
					// reference.
    TSNode	    	*tn_node;
};

// Represents a TSParser object. Each object may have child parsers, essentially
// creating a tree of parsers. This allows us to handle injected languages
// inside a language.
struct vts_parser_S
{
    TSParser	    	*tp_parser;
    vts_tree_T	    	*tp_tree;	// Associated TSTree object
    TSInput		tp_input;
    bool		tp_parsing;	// true if currently parsing

#ifdef ELAPSED_FUNC
    elapsed_T		tp_elapsed;	// Used check if a parse operation
					// should be cancelled.
#endif
    long		tp_timeout;

    garray_T	    	tp_children;
    
    // Used for async parsing
    vts_parser_T	*tp_next;
};

#endif // FEAT_TREESITTER
