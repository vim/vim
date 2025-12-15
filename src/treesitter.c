/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * treesitter.c: Treesitter integration logic
 */

#include "vim.h"

#ifdef FEAT_TREESITTER

#include <tree_sitter/api.h>

#ifdef MSWIN
# define load_dll vimLoadLib
# define symbol_from_dll GetProcAddress
# define close_dll FreeLibrary
# define load_dll_error GetWin32Error
#else
# include <dlfcn.h>
# define HANDLE void*
# define load_dll(n) dlopen((n), RTLD_LAZY)
# define symbol_from_dll dlsym
# define close_dll dlclose
# define load_dll_error dlerror
#endif

// Buffer size to use when parsing
#define TSVIM_BUFSIZE 256

// Represents a loaded TSLanguage object, that is identified with a name.
typedef struct
{
    HANDLE		dll;
    const TSLanguage	*lang;
    char_u		name[1]; // Actually longer
} TSVimLanguage;

typedef struct
{
    opaque_T	opaque;
    TSParser	*parser;
} TSVimParser;

typedef struct
{
    opaque_T	opaque;
    TSTree	*tree;
} TSVimTree;

typedef struct
{
    opaque_T	opaque;
    TSNode	node;
    opaque_T	*tree;	// Hold back reference to parent TSTree
} TSVimNode;

typedef struct
{
    opaque_T	opaque;
    TSQuery	*query;
} TSVimQuery;

typedef struct
{
    opaque_T	    opaque;
    TSQueryCursor   *querycursor;

    // These may be NULL
    opaque_T	    *node;  // Reference to node object from exec
    opaque_T	    *query; // Reference to query object from exec
} TSVimQueryCursor;

typedef struct
{
    opaque_T	    opaque;
    TSQueryMatch    querymatch; // Note that the captures array is owned by the
			   // querycursor, meaning it will be changed when the
			   // cursor goes onto the next match.
    opaque_T	    *querycursor;
} TSVimQueryMatch;

#define VTS_LANG_OFF offsetof(TSVimLanguage, name)
#define HI2LANG(hi) ((TSVimLanguage *)((hi)->hi_key - VTS_LANG_OFF))

#define OP2TSOBJ(t, s) ((TSVim##t *)s)
#define TSOBJ2OP(s)) ((opaque_T *)s)

#define OP2TSPARSER(s) OP2TSOBJ(Parser, s)
#define OP2TSTREE(s) OP2TSOBJ(Tree, s)
#define OP2TSNODE(s) OP2TSOBJ(Node, s)
#define OP2TSQUERY(s) OP2TSOBJ(Query, s)
#define OP2TSQUERYCURSOR(s) OP2TSOBJ(QueryCursor, s)
#define OP2TSQUERYMATCH(s) OP2TSOBJ(QueryMatch, s)

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T    languages;

static opaque_type_T tsparser_type;
static opaque_type_T tstree_type;
static opaque_type_T tsnode_type;
static opaque_type_T tsquery_type;
static opaque_type_T tsquerycursor_type;
static opaque_type_T tsquerymatch_type;

    static void *
ts_calloc(size_t n, size_t size)
{
    return alloc_clear(n * size);
}

    static void *
ts_realloc(void *ptr, size_t size)
{
    return vim_realloc(ptr, size);
}

    int
tsvim_init(void)
{
    hash_init(&languages);

    ts_set_allocator(alloc, ts_calloc, ts_realloc, vim_free);

    return OK;
}

/*
 * Return the language parser from the given shared object at "path", with the
 * symbol suffix as "symbol". Return NULL on failure.
 */
    static const TSLanguage *
load_language(char *path, char *symbol, HANDLE *dll)
{
    HANDLE	    h = load_dll(path);
    TSLanguage	    *lang;
    TSLanguage	    *(*parser)(void);
    char	    symbol_buf[255];

    if (h == NULL)
    {
	semsg(_(e_could_not_load_library_str_str), path, load_dll_error());
	return NULL;
    }

    vim_snprintf(symbol_buf, sizeof(symbol_buf), "tree_sitter_%s", symbol);

    if ((parser = symbol_from_dll(h, symbol_buf)) == NULL)
    {
	semsg(_(e_could_not_load_library_function_str), symbol_buf);
	close_dll(h);
	return NULL;
    }

    if ((lang = parser()) == NULL)
    {
	semsg(_(e_treesitter_get_lang_error), path);
	close_dll(h);
	return NULL;
    }

    *dll = h;

    return lang;
}

    static void
tsvim_load_language(char_u *name, char_u *path, char_u *symbol_name)
{
    hashitem_T		*hi;
    HANDLE		h;
    const TSLanguage	*lang_obj =
	load_language((char *)path, (char *)symbol_name, &h);

    if (lang_obj == NULL)
	return;

    hi = hash_find(&languages, name);

    if (HASHITEM_EMPTY(hi))
    {
	hash_T		hash = hash_hash(name);
	TSVimLanguage	*obj =
	    alloc(VTS_LANG_OFF + STRLEN(name) + 1);

	if (obj == NULL)
	    return;

	STRCPY(obj->name, name);
	obj->lang = lang_obj;
	obj->dll = h;

	hash_add_item(&languages, hi, obj->name, hash);
    }
    else
    {
	// Replace assigned TSLanguage object with new one.
	TSVimLanguage	*lang = HI2LANG(hi);

	lang->lang = lang_obj;
	lang->dll = h;
    }
}

/*
 * Create new opaque_T for node. Returns NULL on failure or if node is a null
 * node.
 */
    static opaque_T *
new_tsnode(TSNode *node, opaque_T *tree)
{
    // We must make the node have its own reference to the tree, so that the
    // tree doesn't get gc'd and the node becomes invalid (undefined). We do
    // this by adding the pointer to the opaque_T of the TSTree at the end of
    // the TSNode struct in our opaque_T.
    opaque_T *op;

    if (node->tree == NULL)
	// Node is NULL
	return NULL;

    op = opaque_new(&tsnode_type, sizeof(TSVimNode));

    if (op == NULL)
	return NULL;

    OP2TSNODE(op)->node = *node;
    OP2TSNODE(op)->tree = tree;
    tree->op_refcount++;
    op->op_refcount++;

    return op;
}

/*
 * Create a tuple that represents a TSPoint. Returns NULL on failure.
 */
    static tuple_T *
tspoint_to_tuple(TSPoint *point)
{
    tuple_T	*t = tuple_alloc_with_items(2);

    if (t == NULL)
	return NULL;

    tuple_set_number(t, 0, point->row + 1);
    tuple_set_number(t, 1, point->column + 1);
    t->tv_refcount++;

    return t;
}

/*
 * Convert a tuple into a TSPoint struct, Returns FAIL on failure and emits an
 * error.
 */
    static int
tuple_to_tspoint(tuple_T *tuple, TSPoint *point)
{
    typval_T *row;
    typval_T *col;

    if (tuple_len(tuple) != 2)
    {
	semsg(_(e_invalid_argument_str), "tuple not of type <number, number>");
	return FAIL;
    }

    row = TUPLE_ITEM(tuple, 0);
    col = TUPLE_ITEM(tuple, 1);

    if (row->v_type != VAR_NUMBER || col->v_type != VAR_NUMBER)
    {
	semsg(_(e_invalid_argument_str), "tuple not of type <number, number>");
	return FAIL;
    }

    point->row = row->vval.v_number - 1;
    point->column = col->vval.v_number - 1;
    return OK;
}

/*
 * Convert a tuple into a TSRange struct. Returns FAIL on failure, and emits an
 * error. The tuple should be in the format of:
 * ((start_point), start_byte, (end_point), end_byte)
 *
 */
    static int
tuple_to_tsrange(tuple_T *tuple, TSRange *range)
{
    typval_T	*starttv, *endtv, *start_bytetv, *end_bytetv;
    TSPoint	start, end;

    if (tuple_len(tuple) != 4)
    {
	semsg(_(e_invalid_argument_str),
		"tuple not type <tuple<number, number>, number, tuple<number, "
		"number>, number>");
	return FAIL;
    }

    starttv = TUPLE_ITEM(tuple, 0);
    start_bytetv = TUPLE_ITEM(tuple, 1);
    endtv = TUPLE_ITEM(tuple, 2);
    end_bytetv = TUPLE_ITEM(tuple, 3);

    if (starttv->v_type != VAR_TUPLE
	    || endtv->v_type != VAR_TUPLE
	    || start_bytetv->v_type != VAR_NUMBER
	    || end_bytetv->v_type != VAR_NUMBER)
    {
	semsg(_(e_invalid_argument_str),
		"tuple not type <tuple<number, number>, number, tuple<number, "
		"number>, number>");
	return FAIL;
    }

    if (tuple_to_tspoint(starttv->vval.v_tuple, &start) == FAIL
	    || tuple_to_tspoint(endtv->vval.v_tuple, &end) == FAIL)
	return FAIL;

    range->start_point = start;
    range->end_point = end;
    range->start_byte = start_bytetv->vval.v_number;
    range->end_byte = end_bytetv->vval.v_number;

    return OK;
}

/*
 * Create a tuple that represents a TSRange. Returns NULL on failure.
 */
    static tuple_T *
tsrange_to_tuple(TSRange *range)
{
    tuple_T	*t = tuple_alloc_with_items(4);
    tuple_T	*start, *end;

    if (t == NULL)
	return NULL;

    start = tspoint_to_tuple(&range->start_point);

    if (start == NULL)
    {
	tuple_unref(t);
	return NULL;
    }

    end = tspoint_to_tuple(&range->end_point);

    if (end == NULL)
    {
	tuple_unref(t);
	tuple_unref(start);
	return NULL;
    }

    tuple_set_tuple(t, 0, start);
    tuple_set_number(t, 1, range->start_byte);
    tuple_set_tuple(t, 2, end);
    tuple_set_number(t, 3, range->end_byte);
    t->tv_refcount++;

    return t;
}

    static TSVimLanguage *
get_language(char_u *language)
{
    hashitem_T *hi = hash_find(&languages, language);

    if (!HASHITEM_EMPTY(hi))
	return HI2LANG(hi);
    else
	semsg(_(e_treesitter_lang_not_loaded), language);
    return NULL;
}

    static bool
tsparser_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2TSPARSER(a)->parser == OP2TSPARSER(b)->parser;
}

    static void
tsparser_free_func(opaque_T *op)
{
    ts_parser_delete(OP2TSPARSER(op)->parser);
}

    static bool
tstree_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2TSTREE(a)->tree == OP2TSTREE(b)->tree;
}

    static void
tstree_free_func(opaque_T *op)
{
    ts_tree_delete(OP2TSTREE(op)->tree);
}

    static void
tsnode_free_func(opaque_T *op)
{
    // Remove our reference to the TSTree
    opaque_unref(OP2TSNODE(op)->tree);
}

    static bool
tsnode_equal_func(opaque_T *a, opaque_T *b)
{
    return ts_node_eq(OP2TSNODE(a)->node, OP2TSNODE(b)->node);
}

    static bool
tsquery_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2TSQUERY(a)->query == OP2TSQUERY(b)->query;
}

    static void
tsquery_free_func(opaque_T *op)
{
    ts_query_delete(OP2TSQUERY(op)->query);
}

    static bool
tsquerycursor_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2TSQUERYCURSOR(a)->querycursor == OP2TSQUERYCURSOR(b)->querycursor;
}

    static void
tsquerycursor_free_func(opaque_T *op)
{
    opaque_unref(OP2TSQUERYCURSOR(op)->node);
    opaque_unref(OP2TSQUERYCURSOR(op)->query);
    ts_query_cursor_delete(OP2TSQUERYCURSOR(op)->querycursor);
}

    static bool
tsquerymatch_equal_func(opaque_T *a, opaque_T *b)
{
    TSVimQueryMatch *a2 = OP2TSQUERYMATCH(a);
    TSVimQueryMatch *b2 = OP2TSQUERYMATCH(b);

    return tsquerycursor_equal_func(a2->querycursor, b2->querycursor)
	&& a2->querymatch.id == b2->querymatch.id;
}

    static void
tsquerymatch_free_func(opaque_T *op)
{
    opaque_unref(OP2TSQUERYMATCH(op)->querycursor);
}

typedef enum
{
    NPROP_CHILD_COUNT = 0,
    NPROP_END_BYTE,
    NPROP_END_POINT,
    NPROP_EXTRA,
    NPROP_HAS_CHANGES,
    NPROP_HAS_ERROR,
    NPROP_MISSING,
    NPROP_NAMED_CHILD_COUNT,
    NPROP_NAMED,
    NPROP_NEXT_NAMED_SIBLING,
    NPROP_NEXT_SIBLING,
    NPROP_PARENT,
    NPROP_PREV_NAMED_SIBLING,
    NPROP_PREV_SIBLING,
    NPROP_START_BYTE,
    NPROP_START_POINT,
    NPROP_STRING,
    NPROP_SYMBOL,
    NPROP_TREE,
    NPROP_TYPE,
} TSVimNodeProperty;

    static int
tsnode_property_func(opaque_T *op, opaque_property_T *prop, typval_T *rettv)
{
#define NEW_TSNODE(func) \
    do { \
	TSNode	n = func(node); \
	if (n.tree != NULL) \
	{ \
	    opaque_T *new = new_tsnode(&n, tree_obj); \
	    if (new == NULL) \
	    return FAIL; \
	    rettv->v_type = VAR_OPAQUE; \
	    rettv->vval.v_opaque = new; \
	} \
	else \
	{ \
	    rettv->v_type = VAR_OPAQUE; \
	    rettv->vval.v_opaque = NULL; \
	} \
    } while (false);

    TSNode node = OP2TSNODE(op)->node;
    opaque_T *tree_obj = OP2TSNODE(op)->tree;

    switch ((TSVimNodeProperty)prop->opp_idx)
    {
	case NPROP_CHILD_COUNT:		    // child_count
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = ts_node_child_count(node);
	    break;
	case NPROP_END_BYTE:		    // end_byte
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = ts_node_end_byte(node);
	    break;
	case NPROP_END_POINT:		    // end_point
	{
	    TSPoint p = ts_node_end_point(node);
	    rettv->v_type = VAR_TUPLE;
	    rettv->vval.v_tuple = tspoint_to_tuple(&p);
	}
	    break;
	case NPROP_EXTRA:		    // extra
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = ts_node_is_extra(node);
	    break;
	case NPROP_HAS_CHANGES:		    // has_changes
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = ts_node_has_changes(node);
	    break;
	case NPROP_HAS_ERROR:		    // has_error
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = ts_node_has_error(node);
	    break;
	case NPROP_MISSING:		    // missing
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = ts_node_is_missing(node);
	    break;
	case NPROP_NAMED_CHILD_COUNT:	    // named_child_count
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = ts_node_named_child_count(node);
	    break;
	case NPROP_NAMED:		    // named
	    rettv->v_type = VAR_BOOL;
	    rettv->vval.v_number = ts_node_is_named(node);
	    break;
	case NPROP_NEXT_NAMED_SIBLING:	    // next_named_sibling
	    NEW_TSNODE(ts_node_next_named_sibling);
	    break;
	case NPROP_NEXT_SIBLING:	    // next_sibling
	    NEW_TSNODE(ts_node_next_sibling);
	    break;
	case NPROP_PARENT:		    // parent
	    NEW_TSNODE(ts_node_parent);
	    break;
	case NPROP_PREV_NAMED_SIBLING:	    // prev_named_sibling
	    NEW_TSNODE(ts_node_prev_named_sibling);
	    break;
	case NPROP_PREV_SIBLING:	    // prev_sibling
	    NEW_TSNODE(ts_node_prev_sibling);
	    break;
	case NPROP_START_BYTE:		    // start_byte
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = ts_node_start_byte(node);
	    break;
	case NPROP_START_POINT:		    // start_point
	{
	    TSPoint p = ts_node_start_point(node);
	    rettv->v_type = VAR_TUPLE;
	    rettv->vval.v_tuple = tspoint_to_tuple(&p);
	}
	    break;
	case NPROP_STRING:		    // string
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = (char_u *)ts_node_string(node);
	    break;
	case NPROP_SYMBOL:		    // symbol
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = ts_node_symbol(node);
	    break;
	case NPROP_TREE:		    // tree
	{
	    tree_obj->op_refcount++;
	    rettv->v_type = VAR_OPAQUE;
	    rettv->vval.v_opaque = tree_obj;
	}
	    break;
	case NPROP_TYPE:		    // type
	    rettv->v_type = VAR_STRING;
	    rettv->vval.v_string = vim_strsave((char_u *)ts_node_type(node));
	    break;
    }
#undef NEW_TSNODE

    return OK;
}

typedef enum
{
    QMPROP_CAPTURES = 0,
    QMPROP_ID,
    QMPROP_PATTERN_INDEX,
} TSVimQueryCursorProperty;

    static int
tsquerymatch_property_func(opaque_T *op, opaque_property_T *prop, typval_T *rettv)
{
    TSQueryMatch *match = &OP2TSQUERYMATCH(op)->querymatch;
    opaque_T *tree = OP2TSNODE(OP2TSQUERYCURSOR(
		OP2TSQUERYMATCH(op)->querycursor)->node)->tree;

    switch ((TSVimQueryCursorProperty)prop->opp_idx)
    {
	case QMPROP_CAPTURES:		    // captures
	{
	    tuple_T	*ret;

	    ret = tuple_alloc_with_items(match->capture_count);
	    if (ret == NULL)
		return FAIL;

	    for (int i = 0; i < match->capture_count; i++)
	    {
		TSQueryCapture  capture = match->captures[i];
		tuple_T		*t = tuple_alloc_with_items(2);
		typval_T	node;

		if (t == NULL)
		{
		    tuple_unref(ret);
		    return FAIL;
		}

		node.v_type = VAR_OPAQUE;
		// If it fails then we just get a null opaque object
		node.vval.v_opaque = new_tsnode(&capture.node, tree);

		tuple_set_item(t, 0, &node);
		tuple_set_number(t, 1, capture.index);

		t->tv_refcount++;
		tuple_set_tuple(ret, i, t);
	    }
	    ret->tv_refcount++;
	    rettv->v_type = VAR_TUPLE;
	    rettv->vval.v_tuple = ret;
	}
	    break;
	case QMPROP_ID:			    // id
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = match->id;
	    break;
	case QMPROP_PATTERN_INDEX:	    // pattern_index
	    rettv->v_type = VAR_NUMBER;
	    rettv->vval.v_number = match->pattern_index;
	    break;
    }
    return OK;
}

typedef enum
{
    TRPROP_ROOT = 0,
} TSVimTreeProperty;

    static int
tstree_property_func(opaque_T *op, opaque_property_T *prop, typval_T *rettv)
{
    TSVimTree *tree = OP2TSTREE(op);

    switch ((TSVimTreeProperty)prop->opp_idx)
    {
	case TRPROP_ROOT:		    // root
	{
	    TSNode node = ts_tree_root_node(OP2TSTREE(tree)->tree);
	    opaque_T *res = new_tsnode(&node, op);

	    if (res == NULL)
		return FAIL;
	    rettv->v_type = VAR_OPAQUE;
	    rettv->vval.v_opaque = res;
	}
	    break;
    }
    return OK;
}

static type_T *number_number[] = {&t_number, &t_number};
static type_T *tsnode_number[] = {&t_tsnode, &t_number};

static type_T t_tspoint = {
    VAR_TUPLE, 2, 2, TTFLAG_STATIC, &t_number, NULL, number_number, NULL
};

static type_T t_tscapture = {
    VAR_TUPLE, 2, 2, TTFLAG_STATIC, &t_any, NULL, tsnode_number, NULL
};

static type_T t_tscapturetuple = {
    VAR_TUPLE, -1, 0,TTFLAG_STATIC, &t_tscapture, NULL, NULL, NULL
};

type_T t_tsparser = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tsparser_type
};

type_T t_tstree = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tstree_type
};

type_T t_tsnode = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tsnode_type
};

type_T t_tsquery = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tsquery_type
};

type_T t_tsquerycursor = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tsquerycursor_type
};

type_T t_tsquerymatch = {
    VAR_OPAQUE, 0, 0, TTFLAG_STATIC, NULL, NULL, NULL, &tsquerymatch_type
};

static opaque_property_T tsnode_properties[] = {
    {NPROP_CHILD_COUNT,		    OPPROPNAME("child_count"), &t_number},
    {NPROP_END_BYTE,		    OPPROPNAME("end_byte"), &t_number},
    {NPROP_END_POINT,		    OPPROPNAME("end_point"), &t_tspoint},
    {NPROP_EXTRA,		    OPPROPNAME("extra"), &t_bool},
    {NPROP_HAS_CHANGES,		    OPPROPNAME("has_changes"), &t_bool},
    {NPROP_HAS_ERROR,		    OPPROPNAME("has_error"), &t_bool},
    {NPROP_MISSING,		    OPPROPNAME("missing"), &t_bool},
    {NPROP_NAMED_CHILD_COUNT,	    OPPROPNAME("named_child_count"), &t_number},
    {NPROP_NAMED,		    OPPROPNAME("named"), &t_bool},
    {NPROP_NEXT_NAMED_SIBLING,	    OPPROPNAME("next_named_sibling"), &t_tsnode},
    {NPROP_NEXT_SIBLING,	    OPPROPNAME("next_sibling"), &t_tsnode},
    {NPROP_PARENT,		    OPPROPNAME("parent"), &t_tsnode},
    {NPROP_PREV_NAMED_SIBLING,	    OPPROPNAME("prev_named_sibling"), &t_tsnode},
    {NPROP_PREV_SIBLING,	    OPPROPNAME("prev_sibling"), &t_tsnode},
    {NPROP_START_BYTE,		    OPPROPNAME("start_byte"), &t_number},
    {NPROP_START_POINT,		    OPPROPNAME("start_point"), &t_tspoint},
    {NPROP_STRING,		    OPPROPNAME("string"), &t_string},
    {NPROP_SYMBOL,		    OPPROPNAME("symbol"), &t_number},
    {NPROP_TREE,		    OPPROPNAME("tree"), &t_tstree},
    {NPROP_TYPE,		    OPPROPNAME("type"), &t_string},
};

static opaque_property_T tsquerymatch_properties[] = {
    {QMPROP_CAPTURES,	    OPPROPNAME("captures"), &t_tscapturetuple},
    {QMPROP_ID,		    OPPROPNAME("id"), &t_number},
    {QMPROP_PATTERN_INDEX,    OPPROPNAME("pattern_index"), &t_number},
};

static opaque_property_T tstree_properties[] = {
    {TRPROP_ROOT,		    OPPROPNAME("root"), &t_tsnode},
};

static opaque_type_T tsparser_type = {
    (char_u *)"TSParser", 0, NULL, tsparser_free_func, tsparser_equal_func, NULL, NULL
};

static opaque_type_T tstree_type = {
    (char_u *)"TSTree", ARRAY_LENGTH(tstree_properties), tstree_properties,
    tstree_free_func, tstree_equal_func, NULL, tstree_property_func
};

static opaque_type_T tsnode_type = {
    (char_u *)"TSNode", ARRAY_LENGTH(tsnode_properties), tsnode_properties,
    tsnode_free_func, tsnode_equal_func, NULL, tsnode_property_func
};

static opaque_type_T tsquery_type = {
    (char_u *)"TSQuery", 0, NULL, tsquery_free_func, tsquery_equal_func, NULL, NULL
};

static opaque_type_T tsquerycursor_type = {
    (char_u *)"TSQueryCursor", ARRAY_LENGTH(tsquerymatch_properties),
    tsquerymatch_properties, tsquerycursor_free_func,
    tsquerycursor_equal_func, NULL, tsquerymatch_property_func
};

static opaque_type_T tsquerymatch_type = {
    (char_u *)"TSQueryMatch", ARRAY_LENGTH(tsquerymatch_properties),
    tsquerymatch_properties, tsquerymatch_free_func,
    tsquerymatch_equal_func, NULL, tsquerymatch_property_func
};


// Should be in sync with opaque_types_table[]
static opaque_type_T *opaque_types[] = {
    &tsnode_type,
    &tsparser_type,
    &tsquery_type,
    &tsquerycursor_type,
    &tsquerymatch_type,
    &tstree_type
};

// Should be sorted alphabetically
static keyvalue_T opaque_types_table[] = {
    KEYVALUE_ENTRY(0, "TSNode"),
    KEYVALUE_ENTRY(1, "TSParser"),
    KEYVALUE_ENTRY(2, "TSQuery"),
    KEYVALUE_ENTRY(3, "TSQueryCursor"),
    KEYVALUE_ENTRY(4, "TSQueryMatch"),
    KEYVALUE_ENTRY(5, "TSTree"),
};

/*
 * Lookup the given opaque type and return the opaque_type_T struct if it
 * corresponds to a treesitter type.
 */
    opaque_type_T *
tsvim_lookup_opaque_type(char_u *name, size_t namelen)
{
    keyvalue_T target;
    keyvalue_T *entry;

    target.key = 0;
    target.value.string = name;
    target.value.length = namelen;

    entry = (keyvalue_T *)bsearch(&target, &opaque_types_table,
	    ARRAY_LENGTH(opaque_types_table), sizeof(opaque_types_table[0]),
	    cmp_keyvalue_value_n);

    if (entry == NULL)
	return NULL;

    return opaque_types[entry->key];
}

/*
 * Allocate a new TSParser object. Returns NULL on failure.
 */
    static opaque_T *
tsparser_new(void)
{
    TSParser *parser = ts_parser_new();
    opaque_T *op;

    // Documentation says nothing about it returning NULL but just check to be
    // sure.
    if (parser == NULL)
	return NULL;

    op = opaque_new(&tsparser_type, sizeof(TSVimParser));

    if (op == NULL)
    {
	ts_parser_delete(parser);
	return NULL;
    }
    OP2TSPARSER(op)->parser = parser;
    op->op_refcount++;

    return op;
}

/*
 * Set the given parser to "language".
 */
    static void
tsparser_set_language(opaque_T *parser, char_u *language)
{
    TSVimLanguage *lang = get_language(language);

    if (lang != NULL)
	ts_parser_set_language(OP2TSPARSER(parser)->parser, lang->lang);
}

#ifdef ELAPSED_FUNC
typedef struct
{
    elapsed_T start;
    long timeout;
} ProgressContext;

    static bool
parser_progress_callback(TSParseState *state)
{
    ProgressContext *ctx = state->payload;

    return ELAPSED_FUNC(ctx->start) >= ctx->timeout;
}
#endif

    static uint32_t
parser_decode_callback(const uint8_t *string, uint32_t length, int32_t *code_point)
{
    char_u *str = (char_u *)string;

    if (length == 0)
    {
	*code_point = 0;
	return 0;
    }
    else if (has_mbyte)
    {
	uint32_t char_len = (uint32_t)(*mb_ptr2len_len)(str, length);

	if (char_len > length)
	    // In middle of mutlibyte character
	    return 0;

	*code_point = (int32_t)(*mb_ptr2char)(str);
	return char_len;
    }

    // Characters are just single bytes, just set it directly
    *code_point = *string;
    return 1;
}

    static TSTree *
tsvim_parser_parse(
	TSParser *parser,
	TSTree *last_tree,
	const char *(*read)(
	    void *payload,
	    uint32_t byte_index,
	    TSPoint position,
	    uint32_t *bytes_read),
	void *userdata,
	long timeout
	)
{

    TSInput	    input;
    TSTree	    *res;
    TSParseOptions  opts;
#ifdef ELAPSED_FUNC
    ProgressContext progress_ctx;
#else
    (void)timeout;
#endif

    if (enc_utf8)
	input.encoding = TSInputEncodingUTF8;
    else if (enc_unicode == 2)
    {
	int prop = enc_canon_props(p_enc);

	// Can either be UTF-16 or UCS-2, make sure it is UTF-16.
	if (prop & (ENC_ENDIAN_B | ENC_2WORD))
	    input.encoding = TSInputEncodingUTF16BE;
	else if (prop & (ENC_ENDIAN_L | ENC_2WORD))
	    input.encoding = TSInputEncodingUTF16LE;
	else
	    input.decode = parser_decode_callback;
    }
    else
	input.decode = parser_decode_callback;

    input.payload = userdata;
    input.read = read;

    if (timeout != -1)
    {
#ifdef ELAPSED_FUNC
	ELAPSED_INIT(progress_ctx.start);
	progress_ctx.timeout = timeout;
	opts.payload = &progress_ctx;
	opts.progress_callback = parser_progress_callback;
#else
	memset(&opts, 0, sizeof(opts));
#endif
	res = ts_parser_parse_with_options(parser, last_tree, input, opts);
    }
    else
	res = ts_parser_parse(parser, last_tree, input);

    return res;
}

    static const char *
parse_buf_read_callback(
	void *payload,
	uint32_t byte_index UNUSED,
	TSPoint position,
	uint32_t *bytes_read)
{
    buf_T        *bp = payload;
    static char buf[TSVIM_BUFSIZE];

    // Finish if we are past the last line
    if ((linenr_T)position.row >= bp->b_ml.ml_line_count)
    {
	*bytes_read = 0;
	return NULL;
    }

    char *line = (char *)ml_get_buf(bp, position.row + 1, FALSE);
    uint32_t cols = ml_get_buf_len(bp, position.row + 1);
    uint32_t to_copy;

    // Should only be true if the last call didn't add a newline
    if (position.column > cols)
    {
	*bytes_read = 0;
	return NULL;
    }

    // Subtract one from buffer size so we can include newline
    to_copy = MIN(cols - position.column, TSVIM_BUFSIZE - 1);
    memcpy(buf, line + position.column, to_copy);

    // If to_copy == cols, then the entire line fits in the buffer - 1, add a
    // newline. If to_copy == 0, then add a newline because we are at end of the
    // line.
    if (to_copy == cols || to_copy == 0)
	buf[to_copy++] = NL;

    *bytes_read = to_copy;

    return buf;
}

/*
 * Parse the given buffer using the parser and return the result TSTree if is
 * completed within the timeout, else NULL. If "timeout" is -1, then there will
 * be no timeout
 */
    static opaque_T *
tsparser_parse_buf(
	opaque_T *parser,
	opaque_T *last_tree,
	buf_T *buf,
	long timeout)
{
    TSTree	*ltree = last_tree == NULL ? NULL : OP2TSTREE(last_tree)->tree;
    TSTree	*res;
    opaque_T	*op;

    // Check if parser is set to a language
    if (ts_parser_language(OP2TSPARSER(parser)->parser) == NULL)
    {
	emsg(_(e_tsparser_not_set_to_language));
	return NULL;
    }

    res = tsvim_parser_parse(OP2TSPARSER(parser)->parser, ltree,
	    parse_buf_read_callback, buf, timeout);

    if (res == NULL)
	return NULL;

    op = opaque_new(&tstree_type, sizeof(TSVimTree));
    if (op == NULL)
    {
	ts_tree_delete(res);
	return NULL;
    }
    OP2TSTREE(op)->tree = res;
    op->op_refcount++;

    return op;
}

    static char_u *
query_error_to_string(TSQueryError error)
{
    // Copied from Neovim
    switch (error)
    {
	case TSQueryErrorSyntax:
	    return (char_u *)"Invalid syntax";
	case TSQueryErrorNodeType:
	    return (char_u *)"Invalid node type";
	case TSQueryErrorField:
	    return (char_u *)"Invalid field name";
	case TSQueryErrorCapture:
	    return (char_u *)"Invalid capture name";
	case TSQueryErrorStructure:
	    return (char_u *)"Impossible pattern";
	default:
	    return (char_u *)"Error";
    }
}

    static void
query_do_error_message(char_u *str, uint32_t offset, TSQueryError error)
{
    // Copied from Neovim (mostly)
    int	    line_start = 0;
    int	    row = 0;
    int	    column;
    char_u  *end_str;
    char_u  *msg;

    do {
	char_u *src_tmp = str + line_start;

	int line_length;
	int line_end;

	end_str = vim_strchr(src_tmp, '\n');
	line_length = end_str != NULL ?
	    (int)(end_str - src_tmp) : (int)STRLEN(src_tmp);
	line_end = line_start + line_length;

	if ((uint32_t)line_end > offset)
	    break;

	line_start = line_end + 1;
	row++;
    } while (end_str != NULL);

    column = offset - line_start;
    msg = query_error_to_string(error);
    semsg(_(e_tsquery_error), row, column, msg);
}

/*
 * Create a new TSQuery object using the given string. Returns NULL on failure.
 */
    static opaque_T *
tsquery_new(char_u *language, char_u *query_str)
{
    TSVimLanguage   *lang = get_language(language);
    TSQuery	    *query;
    uint32_t	    error_offset;
    TSQueryError    error_type;
    opaque_T	    *op;

    if (lang == NULL)
	return NULL;

    query = ts_query_new(lang->lang, (char *)query_str,
	    STRLEN(query_str), &error_offset, &error_type);

    if (query == NULL)
    {
	query_do_error_message(query_str, error_offset, error_type);
	return NULL;
    }

    op = opaque_new(&tsquery_type, sizeof(TSVimQuery));
    if (op == NULL)
    {
	ts_query_delete(query);
	return NULL;
    }
    OP2TSQUERY(op)->query = query;
    op->op_refcount++;

    return op;
}

    void
f_ts_load(typval_T *argvars, typval_T *rettv UNUSED)
{
    char_u *name;
    char_u *path;
    char_u *symbol = NULL;

    if (check_for_string_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL
	    || check_for_opt_dict_arg(argvars, 2) == FAIL)
	return;

    name = argvars[0].vval.v_string;
    path = argvars[1].vval.v_string;

    if (argvars[0].v_type != VAR_UNKNOWN && argvars[1].v_type != VAR_UNKNOWN
	    && argvars[1].v_type == VAR_DICT)
    {
	dict_T *d =  argvars[2].vval.v_dict;

	symbol = dict_get_string(d, "symbol", FALSE);
    }

    if (symbol == NULL)
	symbol = name;

    tsvim_load_language(name, path, symbol);
}

    void
f_tsparser_new(typval_T *argvars UNUSED, typval_T *rettv)
{
    opaque_T *op = tsparser_new();

    if (op == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = op;
}

    void
f_tsparser_set_language(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL)
	return;

    tsparser_set_language(argvars[0].vval.v_opaque, argvars[1].vval.v_string);

}

/*
 * Convert an array of TSRange structs to a list. Returns NULL on failure.
 */
    static list_T *
tsrange_array_to_tuple(const TSRange *arr, uint32_t n)
{
    list_T *ret = list_alloc_with_items(n);

    if (ret == NULL)
	return NULL;

    for (uint32_t i = 0; i < n; i++)
    {
	TSRange range = arr[i];
	tuple_T	*t = tsrange_to_tuple(&range);

	if (t == NULL)
	{
	    list_unref(ret);
	    return NULL;
	}

	list_append_tuple(ret, t);
    }

    ret->lv_refcount++;
    return ret;
}

    void
f_tsparser_parse_buf(typval_T *argvars, typval_T *rettv)
{
    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_buffer_arg(argvars, 1) == FAIL
	    || check_for_number_arg(argvars, 2) == FAIL
	    || check_for_opt_opaque_arg(argvars, 3) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL
	    || check_for_opt_opaque_type_arg(argvars, 3, &tstree_type) == FAIL)
	return;

    {
	buf_T	    *buf = get_buf_arg(argvars + 1);
	opaque_T    *res;
        tuple_T	    *t;
        opaque_T    *oldtree =
            argvars[3].v_type == VAR_UNKNOWN ? NULL : argvars[3].vval.v_opaque;

	if (buf == NULL)
	    return;

	res = tsparser_parse_buf(argvars[0].vval.v_opaque, oldtree,
		buf, argvars[2].vval.v_number);

	if (res != NULL)
	{
	    typval_T	tv;
	    list_T	*l;
	    TSRange	*ranges;
	    uint32_t    count;

	    t = tuple_alloc_with_items(2);

	    if (t == NULL)
	    {
		opaque_unref(res);
		return;
	    }

	    tv.v_type = VAR_OPAQUE;
	    tv.vval.v_opaque = res;
	    tuple_set_item(t, 0, &tv);

	    // Get changed ranges if finished, else included ranges of old tree.
	    if (oldtree != NULL)
                ranges = ts_tree_get_changed_ranges(
			OP2TSTREE(oldtree)->tree, OP2TSTREE(res)->tree, &count);
	    else
                ranges = ts_tree_included_ranges(OP2TSTREE(res)->tree, &count);

	    l = tsrange_array_to_tuple(ranges, count);

	    if (l == NULL)
	    {
		tuple_unref(t);
		return;
	    }

	    vim_free(ranges);
	    tuple_set_list(t, 1, l);

	    t->tv_refcount++;
	}
	else
	    t = NULL;

	rettv->v_type = VAR_TUPLE;
	rettv->vval.v_tuple = t;
    }
}

    void
f_tsparser_set_included_ranges(typval_T *argvars, typval_T *rettv)
{
    listitem_T	*li;
    TSRange	*ranges;
    long	len;
    uint32_t	i = 0;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_list_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL)
	return;

    // The list contains tuples in the format of:
    // ((start_point), start_byte, (end_point), end_byte)
    len = list_len(argvars[1].vval.v_list);

    if (len == 0)
    {
	semsg(_(e_invalid_argument_str), "empty list");
	return;
    }

    ranges = ALLOC_MULT(TSRange, len);

    if (ranges == NULL)
	return;

    FOR_ALL_LIST_ITEMS(argvars[1].vval.v_list, li)
    {
	if (li->li_tv.v_type != VAR_TUPLE)
	{
	    semsg(_(e_invalid_argument_str), "list of tuples required");
	    goto exit;
	}
	if (tuple_to_tsrange(li->li_tv.vval.v_tuple, ranges + i) == FAIL)
	    goto exit;
	i++;
    }

    rettv->vval.v_number = ts_parser_set_included_ranges(
	    OP2TSPARSER(argvars[0].vval.v_opaque)->parser, ranges, len);
    rettv->v_type = VAR_BOOL;
exit:
    vim_free(ranges);
}

    void
f_tsparser_included_ranges(typval_T *argvars, typval_T *rettv)
{
    list_T	    *ret;
    const TSRange   *ranges;
    uint32_t	    count;
    opaque_T	    *parser;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL)
	return;

    parser = argvars[0].vval.v_opaque;
    ranges = ts_parser_included_ranges(OP2TSPARSER(parser)->parser, &count);

    ret = tsrange_array_to_tuple(ranges, count);

    if (ret == NULL)
	return;

    rettv->v_type = VAR_LIST;
    rettv->vval.v_list = ret;
}

    void
f_tsparser_reset(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL)
	return;

    ts_parser_reset(OP2TSPARSER(argvars[0].vval.v_opaque)->parser);
}

    void
f_tstree_edit(typval_T *argvars, typval_T *rettv UNUSED)
{
    TSPoint start_point, old_end_point, new_end_point;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1) == FAIL
	    || check_for_number_arg(argvars, 2) == FAIL
	    || check_for_number_arg(argvars, 3) == FAIL
	    || check_for_tuple_arg(argvars, 4) == FAIL
	    || check_for_tuple_arg(argvars, 5) == FAIL
	    || check_for_tuple_arg(argvars, 6) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tstree_type) == FAIL)
	return;

    if (tuple_to_tspoint(argvars[4].vval.v_tuple, &start_point) == FAIL
	    || tuple_to_tspoint(argvars[5].vval.v_tuple, &old_end_point) == FAIL
	    || tuple_to_tspoint(argvars[6].vval.v_tuple, &new_end_point) == FAIL)
	return;

    TSInputEdit edit;

    edit.start_byte = argvars[1].vval.v_number;
    edit.old_end_byte = argvars[2].vval.v_number;
    edit.new_end_byte = argvars[3].vval.v_number;

    edit.start_point = start_point;
    edit.old_end_point = old_end_point;
    edit.new_end_point = new_end_point;

    ts_tree_edit(OP2TSTREE(argvars[0].vval.v_opaque)->tree, &edit);
}

    void
f_tsnode_child(typval_T *argvars, typval_T *rettv)
{
    opaque_T *res;
    bool named = false;
    TSNode new;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1) == FAIL
	    || check_for_opt_bool_arg(argvars, 2) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsnode_type) == FAIL)
	return;

    if (argvars[3].v_type != VAR_UNKNOWN)
	named = argvars[3].vval.v_number ? true : false;

    if (named)
	new = ts_node_named_child(
		OP2TSNODE(argvars[0].vval.v_opaque)->node,
		argvars[1].vval.v_number);
    else
	new = ts_node_child(
		OP2TSNODE(argvars[0].vval.v_opaque)->node,
		argvars[1].vval.v_number);

    res = new_tsnode(&new, OP2TSNODE(argvars[0].vval.v_opaque)->tree);

    if (res == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = res;
}

    void
f_tsnode_descendant_for_range(typval_T *argvars, typval_T *rettv)
{
    opaque_T	*res;
    TSPoint	start, end;
    bool	named = false;
    TSNode	new;

    if (check_for_opaque_arg(argvars, 0) == FAIL
		|| check_for_tuple_arg(argvars, 1) == FAIL
		|| check_for_tuple_arg(argvars, 2) == FAIL
		|| check_for_opt_bool_arg(argvars, 3) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsnode_type) == FAIL)
	return;

    if (argvars[3].v_type != VAR_UNKNOWN)
	named = argvars[3].vval.v_number ? true : false;

    if (tuple_to_tspoint(argvars[1].vval.v_tuple, &start) == FAIL
	    || tuple_to_tspoint(argvars[2].vval.v_tuple, &end) == FAIL)
	return;

    if (named)
	new = ts_node_descendant_for_point_range(
		OP2TSNODE(argvars[0].vval.v_opaque)->node, start, end);
    else
	new = ts_node_named_descendant_for_point_range(
		OP2TSNODE(argvars[0].vval.v_opaque)->node, start, end);

    res = new_tsnode(&new, OP2TSNODE(argvars[0].vval.v_opaque)->tree);

    if (res == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = res;
}

    void
f_tsquery_new(typval_T *argvars, typval_T *rettv)
{
    opaque_T *res;

    if (check_for_string_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL)
	return;

    res = tsquery_new(argvars[0].vval.v_string, argvars[1].vval.v_string);

    if (res == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = res;
}

    void
f_tsquery_disable_capture(typval_T *argvars, typval_T *rettv UNUSED)
{
    TSQuery *query;
    char_u *str;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquery_type) == FAIL)
	return;

    query = OP2TSQUERY(argvars[0].vval.v_opaque)->query;
    str = argvars[1].vval.v_string;

    ts_query_disable_capture(query, (char *)str, STRLEN(str));
}

    void
f_tsquery_disable_pattern(typval_T *argvars, typval_T *rettv UNUSED)
{
    TSQuery *query;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquery_type) == FAIL)
	return;

    query = OP2TSQUERY(argvars[0].vval.v_opaque)->query;

    ts_query_disable_pattern(query, argvars[1].vval.v_number);
}

/*
 * Get the predicates/directives and captures for the given query.
 */
    void
f_tsquery_inspect(typval_T *argvars, typval_T *rettv)
{
    TSQuery *query;
    dict_T  *dict;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquery_type) == FAIL)
	return;

    query = OP2TSQUERY(argvars[0].vval.v_opaque)->query;

   /*
    * The dictionary returned is in the following format:
    *
    * {
    *   captures = (
    *	   "capture", ...
    *   ),
    *   patterns = {
    *	  '1': [
    *	    [ <predicate/directive>, <args> ],
    *	    ...
    *     ]
    *   }
    * }
    */
    dict = dict_alloc();

    if (dict == NULL)
	return;

    // Get captures and save them in the dictionary
    {
	uint32_t    n = ts_query_capture_count(query);
	tuple_T	    *captures = tuple_alloc_with_items(n);

	if (captures == NULL)
	    goto fail;

	for (uint32_t i = 0; i < n; i++)
	{
	    uint32_t	length;
	    char_u	*name = vim_strsave((char_u *)
				ts_query_capture_name_for_id(query, i, &length));

	    if (name == NULL)
		goto captures_fail;
	    tuple_set_string(captures, i, name);
	}

	if (dict_add_tuple(dict, "captures", captures) == FAIL)
	{
captures_fail:
	    tuple_unref(captures);
	    goto fail;
	}
    }

    // Get predicates/directives and save them in the dictionary
    {
	uint32_t    n = ts_query_pattern_count(query);
	dict_T	    *patterns = dict_alloc();
	char	    buf[NUMBUFLEN + 1];

	if (patterns == NULL)
	    goto fail;

	for (uint32_t i = 0; i < n; i++)
	{
	    // Each pattern is a list that may contain multiple lists, each
	    // representing a predicate or directive.
	    uint32_t			len;
	    const TSQueryPredicateStep	*step;
	    list_T			*pattern;
	    list_T			*p = NULL;

	    step = ts_query_predicates_for_pattern(query, (uint32_t)i, &len);
	    if (len == 0)
		continue;

	    pattern = list_alloc();

	    if (pattern == NULL)
		goto patterns_fail;

	    // Add each step of the predicate/directive to the list. Each
	    // series of steps that represent a predicate/directive is
	    // terminated with TSQueryPredicateStepTypeDone.
	    for (uint32_t k = 0; k < len; k++)
	    {
		typval_T tv;

		if (p == NULL)
		{
		    p = list_alloc();

		    if (p == NULL)
		    {
			list_unref(pattern);
			goto patterns_fail;
		    }
		}

		switch (step[k].type)
		{
		    case TSQueryPredicateStepTypeString:
		    {
			uint32_t    slen; // Must be provided for value_for_id()
			const char  *str = ts_query_string_value_for_id(
					    query, step[k].value_id, &slen);

			tv.v_type = VAR_STRING;
			tv.vval.v_string = (char_u *)str;
		    }
			break;
		    case TSQueryPredicateStepTypeCapture:
			tv.v_type = VAR_NUMBER;
			tv.vval.v_number = step[k].value_id;
			break;
		    case TSQueryPredicateStepTypeDone:
		    {
			if (list_append_list(pattern, p) == FAIL)
			{
			    list_unref(p);
			    list_unref(pattern);
			    goto patterns_fail;
			}
			p = NULL;
		    }
			continue;
		}
		if (list_append_tv(p, &tv) == FAIL)
		{
		    list_unref(p);
		    list_unref(pattern);
		    goto patterns_fail;
		}
	    }
	    // Add pattern tuple to patterns tuple
	    sprintf(buf, "%d", i);
	    if (dict_add_list(patterns, buf, pattern) == FAIL)
	    {
		list_unref(pattern);
		goto patterns_fail;
	    }
	}

	if (dict_add_dict(dict, "patterns", patterns) == FAIL)
	{
patterns_fail:
	    dict_unref(patterns);
	    goto fail;
	}
    }


    dict->dv_refcount++;
    rettv->v_type = VAR_DICT;
    rettv->vval.v_dict = dict;
    return;
fail:
    dict_unref(dict);
}

    void
f_tsquerycursor_new(typval_T *argvars, typval_T *rettv)
{
    TSQueryCursor   *cursor;
    opaque_T	    *op;
    bool	    have_range = false;
    bool	    have_max_depth = false;
    bool	    have_match_limit = false;
    TSPoint	    startp, endp;
    uint32_t	    max_depth, match_limit;

    if (check_for_opt_dict_arg(argvars, 0) == FAIL)
	return;

    if (argvars[0].v_type == VAR_DICT)
    {
	dict_T *opts = argvars[0].vval.v_dict;
	typval_T rangetv, max_depthtv, match_limittv;

	// "range" type: tuple<tuple<number, number>, tuple<number, number>>
	if (dict_get_tv(opts, "range", &rangetv) == OK)
	{
	    typval_T *starttv, *endtv;
	    tuple_T *start, *end;

	    if (rangetv.v_type != VAR_TUPLE
		    || rangetv.vval.v_tuple->tv_items.ga_len != 2)
	    {
range_fail:
		semsg(_(e_invalid_argument_str), "range");
		clear_tv(&rangetv);
		return;
	    }

	    starttv = TUPLE_ITEM(rangetv.vval.v_tuple, 0);
	    endtv = TUPLE_ITEM(rangetv.vval.v_tuple, 1);

	    if (starttv->v_type != VAR_TUPLE || endtv->v_type != VAR_TUPLE)
		goto range_fail;

	    start = starttv->vval.v_tuple;
	    end = endtv->vval.v_tuple;

	    if (tuple_to_tspoint(start, &startp) == FAIL
		    || tuple_to_tspoint(end, &endp) == FAIL)
	    {
		clear_tv(&rangetv);
		return;
	    }

	    have_range = true;
	}

	if (dict_get_tv(opts, "max_start_depth", &max_depthtv) == OK)
	{
	    if (max_depthtv.v_type != VAR_NUMBER)
	    {
		semsg(_(e_invalid_argument_str), "max_start_depth");
		clear_tv(&rangetv);
		clear_tv(&max_depthtv);
		return;
	    }
	    max_depth = max_depthtv.vval.v_number;
	    have_max_depth = true;
	}

	if (dict_get_tv(opts, "match_limit", &match_limittv) == OK)
	{
	    if (match_limittv.v_type != VAR_NUMBER)
	    {
		semsg(_(e_invalid_argument_str), "match_limit");
		clear_tv(&rangetv);
		clear_tv(&max_depthtv);
		clear_tv(&match_limittv);
		return;
	    }
	    match_limit = match_limittv.vval.v_number;
	    have_match_limit = true;
	}
    }

    cursor = ts_query_cursor_new();

    if (cursor == NULL)
	return;

    // The querycursor struct is to hold the query and node object when we exec.
    op = opaque_new(&tsquerycursor_type, sizeof(TSVimQueryCursor));

    if (op == NULL)
    {
	ts_query_cursor_delete(cursor);
	return;
    }
    OP2TSQUERYCURSOR(op)->querycursor = cursor;
    op->op_refcount++;

    if (have_range)
	ts_query_cursor_set_point_range(cursor, startp, endp);
    if (have_max_depth)
	ts_query_cursor_set_max_start_depth(cursor, max_depth);
    if (have_match_limit)
	ts_query_cursor_set_match_limit(cursor, match_limit);

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = op;
}

    void
f_tsquerycursor_exec(typval_T *argvars, typval_T *rettv UNUSED)
{
    TSQueryCursor	    *cursor;
    TSQuery		    *query;
    TSNode		    node;
    opaque_T		    *old_query, *old_node;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_opaque_arg(argvars, 1) == FAIL
	    || check_for_opaque_arg(argvars, 2) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL
	    || check_for_opaque_type_arg(argvars, 1, &tsquery_type) == FAIL
	    || check_for_opaque_type_arg(argvars, 2, &tsnode_type) == FAIL)
	return;

    cursor = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->querycursor;
    query = OP2TSQUERY(argvars[1].vval.v_opaque)->query;
    node = OP2TSNODE(argvars[2].vval.v_opaque)->node;

    // Add back references to the query and node object. Replace the current
    // ones (and unref them) if any.
    old_node = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->node;
    old_query = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->query;

    opaque_unref(old_node);
    opaque_unref(old_query);

    // Ref node
    OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->node = argvars[2].vval.v_opaque;
    argvars[2].vval.v_opaque->op_refcount++;

    // Ref query
    OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->query = argvars[1].vval.v_opaque;
    argvars[1].vval.v_opaque->op_refcount++;

    ts_query_cursor_exec(cursor, query, node);
}

    void
f_tsquerycursor_next_match(typval_T *argvars, typval_T *rettv)
{
    TSVimQueryCursor	*obj;
    TSQueryCursor	*cursor;
    TSQueryMatch	match;
    opaque_T		*op;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL)
	return;

    obj = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque);
    cursor = obj->querycursor;

    if (ts_query_cursor_next_match(cursor, &match))
    {
	op = opaque_new(&tsquerymatch_type, sizeof(TSVimQueryMatch));

	if (op == NULL)
	    return;
	OP2TSQUERYMATCH(op)->querymatch = match;
	OP2TSQUERYMATCH(op)->querycursor = argvars[0].vval.v_opaque;
	op->op_refcount++;
	argvars[0].vval.v_opaque->op_refcount++;
    }
    else
	// No next match
	op = NULL;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = op;
}

    void
f_tsquerycursor_next_capture(typval_T *argvars, typval_T *rettv)
{
    TSVimQueryCursor	*obj;
    TSQueryCursor	*cursor;
    TSQueryMatch	match;
    uint32_t		idx;
    tuple_T		*t;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL)
	return;

    obj = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque);
    cursor = obj->querycursor;

    if (ts_query_cursor_next_capture(cursor, &match, &idx))
    {
	opaque_T *op;

	t = tuple_alloc_with_items(2);
	if (t == NULL)
	    return;

	op = opaque_new(&tsquerymatch_type, sizeof(TSVimQueryMatch));

	if (op == NULL)
	    return;
	OP2TSQUERYMATCH(op)->querymatch = match;
	OP2TSQUERYMATCH(op)->querycursor = argvars[0].vval.v_opaque;
	op->op_refcount++;
	argvars[0].vval.v_opaque->op_refcount++;

	tuple_set_opaque(t, 0, op);
	tuple_set_number(t, 1, idx);

	t->tv_refcount++;
    }
    else
	// No next match/capture
	t = NULL;

    rettv->v_type = VAR_TUPLE;
    rettv->vval.v_tuple = t;
}

    void
f_tsquerycursor_remove_match(typval_T *argvars, typval_T *rettv UNUSED)
{
    TSQueryCursor *cursor;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1))
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL)
	return;

    cursor = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque)->querycursor;
    ts_query_cursor_remove_match(cursor, argvars[1].vval.v_number);
}

#endif // FEAT_TREESITTER
