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

#include <tree_sitter/api.h>

typedef struct
{
    int			tt_refcount;
    TSTree	    	*tt_tree;
} vts_tree_T;

typedef struct
{
    int			tn_refcount;
    vts_tree_T	    	*tn_tree;	// Parent TSTree object, we hold our own
					// reference.
    TSNode	    	*tn_node;
} vts_node_T;

// Represents a TSParser object. Each object may have child parsers, essentially
// creating a tree of parsers. This allows us to handle injected languages
// inside a language.
typedef struct
{
    TSParser	    	*tp_parser;
    vts_tree_T	    	*tp_tree;	// Associated TSTree object
    garray_T	    	tp_children;
} vts_parser_T;

#endif // FEAT_TREESITTER
