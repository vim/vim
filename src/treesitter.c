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

    union
    {
	TSParser	    *parser;
	TSTree	    	    *tree;
	struct {
	    TSNode	    obj;
	    opaque_T	    *tree;  // Hold back reference to parent TSTree
	} node;

	TSQuery		    *query;
	struct
	{
	    TSQueryCursor   *obj;

	    // These may be NULL
	    opaque_T	    *node;  // Reference to node object from exec
	    opaque_T	    *query; // Reference to query object from exec
	} querycursor;

	struct
	{
	    TSQueryMatch    obj;
	    opaque_T	    *querycursor; // Refrence to querycursor object
	} querymatch;
	void		    *dummy; // Used to compare pointers
    } val;
} TSVimObject;

#define VTS_LANG_OFF offsetof(TSVimLanguage, name)
#define HI2LANG(hi) ((TSVimLanguage *)((hi)->hi_key - VTS_LANG_OFF))

#define NEWOBJ(type) (opaque_new(&type, sizeof(TSVimObject)))

#define OBJ2OP(o) ((opaque_T *)(o))
#define OP2OBJ(o) ((TSVimObject *)(o))
#define OBJVAL(o) (OP2OBJ(o)->val)
#define OP2PTR(o) (OP2OBJ(o)->val.dummy)

#define OP2TSPARSER(o) (OP2OBJ(o)->val.parser)
#define OP2TSTREE(o) (OP2OBJ(o)->val.tree)
#define OP2TSNODE(o) (OP2OBJ(o)->val.node.obj)
#define OP2TSNODETREE(o) (OP2OBJ(o)->val.node.tree)
#define OP2TSQUERY(o) (OP2OBJ(o)->val.query)
#define OP2TSQUERYCURSOR(o) (OP2OBJ(o)->val.querycursor.obj)
#define OP2TSQUERYCURSORNODE(o) (OP2OBJ(o)->val.querycursor.node)
#define OP2TSQUERYCURSORQUERY(o) (OP2OBJ(o)->val.querycursor.query)
#define OP2TSQUERYMATCH(o) (OP2OBJ(o)->val.querymatch.obj)
#define OP2TSQUERYMATCHQUERYCURSOR(o) (OP2OBJ(o)->val.querymatch.querycursor)

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T    languages;

static opaque_type_T tsparser_type;
static opaque_type_T tstree_type;
static opaque_type_T tsnode_type;
static opaque_type_T tsquery_type;
static opaque_type_T tsquerycursor_type;
static opaque_type_T tsquerymatch_type;

    int
tsvim_init(void)
{
    hash_init(&languages);

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

	ts_language_delete(lang->lang);
	lang->lang = lang_obj;
	close_dll(lang->dll);
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

    op = NEWOBJ(tsnode_type);

    if (op == NULL)
	return NULL;

    OBJVAL(op).node.obj = *node;
    OBJVAL(op).node.tree = tree;
    tree->op_refcount++;
    op->op_refcount++;

    return op;
}

/*
 * Create a tuple that represents a TSPoint. Returns NULL on failure.
 */
    static tuple_T *
tspoint_to_tuple(TSPoint point)
{
    typval_T	row, column;
    tuple_T	*t = tuple_alloc_with_items(2);

    if (t == NULL)
	return NULL;

    row.v_type = VAR_NUMBER;
    column.v_type = VAR_NUMBER;
    row.vval.v_number = point.row;
    column.vval.v_number = point.column;

    tuple_set_item(t, 0, &row);
    tuple_set_item(t, 1, &column);
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

    if (tuple->tv_items.ga_len != 2)
    {
	semsg(_(e_invalid_argument_str), "tuple not type <number, number>");
	return FAIL;
    }

    row = TUPLE_ITEM(tuple, 0);
    col = TUPLE_ITEM(tuple, 1);

    if (row->v_type != VAR_NUMBER || col->v_type != VAR_NUMBER)
    {
	emsg(_(e_invalid_argument_tuple_not_number_number));
	return FAIL;
    }

    point->row = row->vval.v_number;
    point->column = col->vval.v_number;
    return OK;
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
tsobject_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2PTR(a) == OP2PTR(b);
}

    static void
tsparser_free_func(opaque_T *op)
{
    ts_parser_delete(OP2TSPARSER(op));
}

    static void
tstree_free_func(opaque_T *op)
{
    ts_tree_delete(OP2TSTREE(op));
}

    static void
tsnode_free_func(opaque_T *op)
{
    opaque_T *tree_obj = OP2TSNODETREE(op);

    // Remove our reference to the TSTree
    opaque_unref(tree_obj);
}

    static bool
tsnode_equal_func(opaque_T *a, opaque_T *b)
{
    return ts_node_eq(OP2TSNODE(a), OP2TSNODE(b));
}

    static void
tsquery_free_func(opaque_T *op)
{
    ts_query_delete(OP2TSQUERY(op));
}

    static void
tsquerycursor_free_func(opaque_T *op)
{
    opaque_unref(OP2TSQUERYCURSORNODE(op));
    opaque_unref(OP2TSQUERYCURSORQUERY(op));
    ts_query_cursor_delete(OP2TSQUERYCURSOR(op));
}

    static void
tsquerymatch_free_func(opaque_T *op)
{
    opaque_unref(OP2TSQUERYMATCHQUERYCURSOR(op));
}

    static bool
tsquerymatch_equal_func(opaque_T *a, opaque_T *b)
{
    return OP2TSQUERYMATCH(a).id == OP2TSQUERYMATCH(b).id;
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

    TSNode node = OP2TSNODE(op);
    opaque_T *tree_obj = OP2TSNODETREE(op);

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
	    rettv->v_type = VAR_TUPLE;
	    rettv->vval.v_tuple = tspoint_to_tuple(ts_node_end_point(node));
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
	    rettv->v_type = VAR_TUPLE;
	    rettv->vval.v_tuple = tspoint_to_tuple(ts_node_start_point(node));
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

static type_T *number_number[] = {&t_number, &t_number};
static type_T t_tspoint = {
    VAR_TUPLE, 2, 0, TTFLAG_STATIC, &t_number, NULL, number_number, NULL
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
    {NPROP_END_BYTE,	    	    OPPROPNAME("end_byte"), &t_number},
    {NPROP_END_POINT,	    	    OPPROPNAME("end_point"), &t_tspoint},
    {NPROP_EXTRA,	    	    OPPROPNAME("extra"), &t_bool},
    {NPROP_HAS_CHANGES,	    	    OPPROPNAME("has_changes"), &t_bool},
    {NPROP_HAS_ERROR,	    	    OPPROPNAME("has_error"), &t_bool},
    {NPROP_MISSING,	    	    OPPROPNAME("missing"), &t_bool},
    {NPROP_NAMED_CHILD_COUNT,	    OPPROPNAME("named_child_count"), &t_number},
    {NPROP_NAMED,	    	    OPPROPNAME("named"), &t_bool},
    {NPROP_NEXT_NAMED_SIBLING,	    OPPROPNAME("next_named_sibling"), &t_tsnode},
    {NPROP_NEXT_SIBLING,	    OPPROPNAME("next_sibling"), &t_tsnode},
    {NPROP_PARENT,		    OPPROPNAME("parent"), &t_tsnode},
    {NPROP_PREV_NAMED_SIBLING,	    OPPROPNAME("prev_named_sibling"), &t_tsnode},
    {NPROP_PREV_SIBLING,	    OPPROPNAME("prev_sibling"), &t_tsnode},
    {NPROP_START_BYTE,		    OPPROPNAME("start_byte"), &t_number},
    {NPROP_START_POINT,     	    OPPROPNAME("start_point"), &t_tspoint},
    {NPROP_STRING,	    	    OPPROPNAME("string"), &t_string},
    {NPROP_SYMBOL,	    	    OPPROPNAME("symbol"), &t_number},
    {NPROP_TREE,	    	    OPPROPNAME("tree"), &t_tstree},
    {NPROP_TYPE,	    	    OPPROPNAME("type"), &t_string},
};

static opaque_type_T tsparser_type = {
    (char_u *)"TSParser", 0, NULL, tsparser_free_func, tsobject_equal_func, NULL, NULL
};

static opaque_type_T tstree_type = {
    (char_u *)"TSTree", 0, NULL, tstree_free_func, tsobject_equal_func, NULL, NULL
};

static opaque_type_T tsnode_type = {
    (char_u *)"TSNode", ARRAY_LENGTH(tsnode_properties), tsnode_properties,
    tsnode_free_func, tsnode_equal_func, NULL, tsnode_property_func
};

static opaque_type_T tsquery_type = {
    (char_u *)"TSQuery", 0, NULL, tsquery_free_func, tsobject_equal_func, NULL, NULL
};

static opaque_type_T tsquerycursor_type = {
    (char_u *)"TSQueryCursor", 0, NULL, tsquerycursor_free_func,
    tsobject_equal_func, NULL, NULL
};

static opaque_type_T tsquerymatch_type = {
    (char_u *)"TSQueryMatch", 0, NULL, tsquerymatch_free_func,
    tsquerymatch_equal_func, NULL, NULL
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

    op = NEWOBJ(tsparser_type);

    if (op == NULL)
    {
	ts_parser_delete(parser);
	return NULL;
    }
    OBJVAL(op).parser = parser;
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
	ts_parser_set_language(OP2TSPARSER(parser), lang->lang);
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

    input.decode = NULL;
    input.encoding = TSInputEncodingUTF8;
    input.payload = userdata;
    input.read = read;

#ifdef ELAPSED_FUNC
    ELAPSED_INIT(progress_ctx.start);
    progress_ctx.timeout = timeout;
    opts.payload = &progress_ctx;
    opts.progress_callback = parser_progress_callback;
#else
    memset(&opts, 0, sizeof(opts));
#endif

    res = ts_parser_parse_with_options(parser, last_tree, input, opts);

    return res;
}

    static const char *
parse_buf_read_callback(
	void *payload,
	uint32_t byte_index,
	TSPoint position,
	uint32_t *bytes_read)
{
    buf_T        *bp = payload;
    static char buf[TSVIM_BUFSIZE];

    // Finish if we are past the last line
    if (position.row >= bp->b_ml.ml_line_count)
    {
	*bytes_read = 0;
	return NULL;
    }

    char *line = (char *)ml_get_buf(bp, position.row + 1, FALSE);
    colnr_T cols = ml_get_buf_len(bp, position.row + 1);
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
 * completed within the timeout, else NULL.
 */
    static opaque_T *
tsparser_parse_buf(
	opaque_T *parser,
	opaque_T *last_tree,
	buf_T *buf,
	long timeout)
{
    TSTree *ltree = last_tree == NULL ? NULL : OP2TSTREE(last_tree);
    TSTree *res;
    opaque_T *op;

    // Check if parser is set to a language
    if (ts_parser_language(OP2TSPARSER(parser)) == NULL)
    {
	emsg(_(e_tsparser_not_set_to_language));
	return NULL;
    }

    res = tsvim_parser_parse(OP2TSPARSER(parser), ltree,
	    parse_buf_read_callback, buf, timeout);

    if (res == NULL)
	return NULL;

    op = NEWOBJ(tstree_type);
    if (op == NULL)
    {
	ts_tree_delete(res);
	return NULL;
    }
    OBJVAL(op).tree = res;
    op->op_refcount++;

    return op;
}

/*
 * Edit the specified tree using the given information describing the edit.
 */
    static void
tstree_edit(
	opaque_T *tree,
	uint32_t start_byte,
	uint32_t old_end_byte,
	uint32_t new_end_byte,
	TSPoint start_point,
	TSPoint old_end_point,
	TSPoint new_end_point
	)
{
    TSInputEdit edit;

    edit.start_byte = start_byte;
    edit.old_end_byte = old_end_byte;
    edit.new_end_byte = new_end_byte;

    edit.start_point = start_point;
    edit.old_end_point = old_end_point;
    edit.new_end_point = new_end_point;

    ts_tree_edit(OP2TSTREE(tree), &edit);
}

/*
 * Return the root node of the tree. Returns NULL on failure.
 */
    static opaque_T *
tstree_root_node(opaque_T *tree)
{
    TSNode node = ts_tree_root_node(OP2TSTREE(tree));
    opaque_T *op = new_tsnode(&node, tree);

    return op;
}

/*
 * Return the child at the given index "idx" of the TSNode object. Returns NULL
 * on failure or if no child exists at "idx". If "named" is true, then skip over
 * anonymous nodes.
 */
    static opaque_T *
tsnode_child(opaque_T *node, int idx, bool named)
{
    TSNode new;

    if (named)
	new = ts_node_named_child(OP2TSNODE(node), idx);
    else
	new = ts_node_child(OP2TSNODE(node), idx);

    return new_tsnode(&new, OP2TSNODETREE(node));
}

/*
 * Get the smallest node within this node that spans the given range of (row,
 * column) positions. If "named" is true, then skip over anonymous nodes.
 * Returns NULL on failure or if no node exists.
 */
    static opaque_T *
tsnode_descendant_for_point_range(
	opaque_T *node,
	TSPoint start,
	TSPoint end,
	bool named)
{
    TSNode new;

    if (named)
	new = ts_node_descendant_for_point_range(OP2TSNODE(node), start, end);
    else
	new = ts_node_named_descendant_for_point_range(
						OP2TSNODE(node), start, end);

    return new_tsnode(&new, OP2TSNODETREE(node));
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
	line_length = end_str != NULL ? (int)(end_str - src_tmp) : STRLEN(src_tmp);
	line_end = line_start + line_length;

	if (line_end > offset)
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

    op = NEWOBJ(tsquery_type);
    if (op == NULL)
    {
	ts_query_delete(query);
	return NULL;
    }
    OBJVAL(op).query = query;
    op->op_refcount++;

    return op;
}

    void
f_ts_load(typval_T *argvars, typval_T *rettv)
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
f_tsparser_new(typval_T *argvars, typval_T *rettv)
{
    opaque_T *op = tsparser_new();

    if (op == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = op;
}

    void
f_tsparser_set_language(typval_T *argvars, typval_T *rettv)
{
    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsparser_type) == FAIL)
	return;

    tsparser_set_language(argvars[0].vval.v_opaque, argvars[1].vval.v_string);

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
	buf_T *buf = get_buf_arg(argvars + 1);
	opaque_T *res;

	if (buf == NULL)
	    return;

	res = tsparser_parse_buf(
		argvars[0].vval.v_opaque,
		argvars[3].v_type == VAR_UNKNOWN
		? NULL : argvars[3].vval.v_opaque,
		buf, argvars[2].vval.v_number);

	if (res == NULL)
	{
	    rettv->v_type = VAR_OPAQUE;
	    rettv->vval.v_opaque = NULL;
	    return;
	}

	rettv->v_type = VAR_OPAQUE;
	rettv->vval.v_opaque = res;
    }
}

    void
f_tstree_edit(typval_T *argvars, typval_T *rettv)
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

    tstree_edit(
	    argvars[0].vval.v_opaque,
	    argvars[1].vval.v_number,
	    argvars[2].vval.v_number,
	    argvars[3].vval.v_number,
	    start_point, old_end_point, new_end_point);
}

    void
f_tstree_root_node(typval_T *argvars, typval_T *rettv)
{
    opaque_T *res;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tstree_type) == FAIL)
	return;

    res = tstree_root_node(argvars[0].vval.v_opaque);

    if (res == NULL)
	return;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = res;
}

    void
f_tsnode_child(typval_T *argvars, typval_T *rettv)
{
    opaque_T *res;
    bool named = false;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1) == FAIL
	    || check_for_opt_bool_arg(argvars, 2) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsnode_type) == FAIL)
	return;

    if (argvars[3].v_type != VAR_UNKNOWN)
	named = argvars[3].vval.v_number ? true : false;

    res = tsnode_child(argvars[0].vval.v_opaque, argvars[1].vval.v_number, named);

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = res;
}

    void
f_tsnode_descendant_for_range(typval_T *argvars, typval_T *rettv)
{
    opaque_T	*res;
    TSPoint	start_point, end_point;
    bool	named = false;

    if (check_for_opaque_arg(argvars, 0) == FAIL
		|| check_for_tuple_arg(argvars, 1) == FAIL
		|| check_for_tuple_arg(argvars, 2) == FAIL
		|| check_for_opt_bool_arg(argvars, 3) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsnode_type) == FAIL)
	return;

    if (argvars[3].v_type != VAR_UNKNOWN)
	named = argvars[3].vval.v_number ? true : false;

    if (tuple_to_tspoint(argvars[1].vval.v_tuple, &start_point) == FAIL
	    || tuple_to_tspoint(argvars[2].vval.v_tuple, &end_point) == FAIL)
	return;

    res = tsnode_descendant_for_point_range(argvars[0].vval.v_opaque,
	    start_point, end_point, named);

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
f_tsquery_disable_capture(typval_T *argvars, typval_T *rettv)
{
    TSQuery *query;
    char_u *str;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_string_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquery_type) == FAIL)
	return;

    query = OP2TSQUERY(argvars[0].vval.v_opaque);
    str = argvars[1].vval.v_string;

    ts_query_disable_capture(query, (char *)str, STRLEN(str));
}

    void
f_tsquery_disable_pattern(typval_T *argvars, typval_T *rettv)
{
    TSQuery *query;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_number_arg(argvars, 1) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquery_type) == FAIL)
	return;

    query = OP2TSQUERY(argvars[0].vval.v_opaque);

    ts_query_disable_pattern(query, argvars[1].vval.v_number);
}

    void
f_tsquerycursor_new(typval_T *argvars, typval_T *rettv)
{
    TSQueryCursor   *cursor;
    opaque_T	    *op;
    bool	    have_range = false;
    bool 	    have_max_depth = false;
    bool 	    have_match_limit = false;
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
    op = NEWOBJ(tsquerycursor_type);

    if (op == NULL)
    {
	ts_query_cursor_delete(cursor);
	return;
    }
    OBJVAL(op).querycursor.obj = cursor;
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
f_tsquerycursor_exec(typval_T *argvars, typval_T *rettv)
{
    TSQueryCursor	    *cursor;
    TSQuery	    	    *query;
    TSNode	    	    node;
    opaque_T		    *old_query, *old_node;

    if (check_for_opaque_arg(argvars, 0) == FAIL
	    || check_for_opaque_arg(argvars, 1) == FAIL
	    || check_for_opaque_arg(argvars, 2) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL
	    || check_for_opaque_type_arg(argvars, 1, &tsquery_type) == FAIL
	    || check_for_opaque_type_arg(argvars, 2, &tsnode_type) == FAIL)
	return;

    cursor = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque);
    query = OP2TSQUERY(argvars[1].vval.v_opaque);
    node = OP2TSNODE(argvars[2].vval.v_opaque);

    // Add back references to the query and node object. Replace the current
    // ones (and unref them) if any.
    old_node = OP2TSQUERYCURSORNODE(argvars[2].vval.v_opaque);
    old_query = OP2TSQUERYCURSORQUERY(argvars[1].vval.v_opaque);

    if (old_node != NULL)
	opaque_unref(old_node);
    if (old_query != NULL)
	opaque_unref(old_query);

    // Ref node
    OBJVAL(argvars[0].vval.v_opaque).querycursor.node = argvars[2].vval.v_opaque;
    argvars[2].vval.v_opaque->op_refcount++;
    // Ref query
    OBJVAL(argvars[0].vval.v_opaque).querycursor.query = argvars[1].vval.v_opaque;
    argvars[1].vval.v_opaque->op_refcount++;

    ts_query_cursor_exec(cursor, query, node);
}

    void
f_tsquerycursor_next_match(typval_T *argvars, typval_T *rettv)
{
    TSQueryCursor   *cursor;
    opaque_T	    *op;
    TSQueryMatch    match;

    if (check_for_opaque_arg(argvars, 0) == FAIL)
	return;

    if (check_for_opaque_type_arg(argvars, 0, &tsquerycursor_type) == FAIL)
	return;

    cursor = OP2TSQUERYCURSOR(argvars[0].vval.v_opaque);

    if (!ts_query_cursor_next_match(cursor, &match))
    {
	rettv->v_type = VAR_OPAQUE;
	rettv->vval.v_opaque = NULL;
	return;
    }

    op = NEWOBJ(tsquerymatch_type);

    if (op == NULL)
	return;

    OBJVAL(op).querymatch.obj = match;
    OBJVAL(op).querymatch.querycursor = argvars[0].vval.v_opaque;
    op->op_refcount++;
    argvars[0].vval.v_opaque->op_refcount++;

    rettv->v_type = VAR_OPAQUE;
    rettv->vval.v_opaque = op;
}

#endif // FEAT_TREESITTER
