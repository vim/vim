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

#define VTS_LANG_OFF offsetof(TSVimLanguage, name)
#define HI2LANG(hi) ((TSVimLanguage *)((hi)->hi_key - VTS_LANG_OFF))

#define OP2TSPARSER(o) (*OP2DATA(o, TSParser *))
#define OP2TSTREE(o) (*OP2DATA(o, TSTree *))
#define OP2TSNODE(o) (*OP2DATA(o, TSNode))
#define OP2TSNODETREE(o) (*OP2DATAOFF(o, opaque_T *, sizeof(TSNode)))
#define OP2TSQUERY(o) (*OP2DATA(o, TSQuery *))
#define OP2TSQUERYCURSOR(o) (*OP2DATA(o, TSQueryCursor *))
#define OP2TSQUERYMATCH(o) (*OP2DATA(o, TSQueryMatch))

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T    languages;

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

/*
 * Handler function for ts_load_lang()
 */
    void
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

    static void
tsquery_free_func(opaque_T *op)
{
    ts_query_delete(OP2TSQUERY(op));
}

    static bool
tsnode_equal_func(opaque_T *a, opaque_T *b)
{
    return ts_node_eq(OP2TSNODE(a), OP2TSNODE(b));
}

/*
 * Allocate a new TSParser object. Returns NULL on failure.
 */
    opaque_T *
tsparser_new(void)
{
    TSParser *parser = ts_parser_new();
    opaque_T *op;

    // Documentation says nothing about it returning NULL but just check to be
    // sure.
    if (parser == NULL)
	return NULL;

    op = opaque_new(TSPARSER, true, &parser, sizeof(TSParser *));

    if (op == NULL)
    {
	ts_parser_delete(parser);
	return NULL;
    }

    op->op_free_func = tsparser_free_func;
    op->op_equal_func= opaque_equal_ptr;
    op->op_refcount++;

    return op;
}

/*
 * Set the given parser to "language".
 */
    void
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
} ParserProgressContext;

    static bool 
parser_progress_callback(TSParseState *state)
{
    ParserProgressContext *ctx = state->payload;

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
    ParserProgressContext progress_ctx;
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
    memset(opts, 0, sizeof(opts));
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
    opaque_T *
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

    op = opaque_new(TSTREE, true, &res, sizeof(TSTree *));
    if (op == NULL)
    {
	ts_tree_delete(res);
	return NULL;
    }

    op->op_free_func = tstree_free_func;
    op->op_equal_func = opaque_equal_ptr;
    op->op_refcount++;

    return op;
}

/*
 * Edit the specified tree using the given information describing the edit.
 */
    void
tstree_edit(
	opaque_T *tree,
	uint32_t start_byte,
	uint32_t old_end_byte,
	uint32_t new_end_byte,
	uint32_t start_point[2],
	uint32_t old_end_point[2],
	uint32_t new_end_point[2]
	)
{
    TSInputEdit edit;

    edit.start_byte = start_byte;
    edit.old_end_byte = old_end_byte;
    edit.new_end_byte = new_end_byte;

    edit.start_point.row = start_point[0];
    edit.start_point.column = start_point[1];

    edit.old_end_point.row = old_end_point[0];
    edit.old_end_point.column = old_end_point[1];

    edit.new_end_point.row = new_end_point[0];
    edit.new_end_point.column = new_end_point[1];

    ts_tree_edit(OP2TSTREE(tree), &edit);
}

/*
 * Return the root node of the tree. Returns NULL on failure.
 */
    opaque_T *
tstree_root_node(opaque_T *tree)
{
    // We must make the node have its own reference to the tree, so that the
    // tree doesn't get gc'd and the node becomes invalid (undefined). We do
    // this by adding the pointer to the opaque_T of the TSTree at the end of
    // the TSNode struct in our opaque_T.
    TSNode node = ts_tree_root_node(OP2TSTREE(tree));
    opaque_T *op = opaque_new(TSNODE, true, NULL,
					    sizeof(TSNode) + sizeof(opaque_T *));

    if (op == NULL)
	return NULL;

    memcpy(op->op_data, &node, sizeof(TSNode));
    memcpy(op->op_data + sizeof(TSNode), &tree, sizeof(opaque_T *));
    tree->op_refcount++;

    op->op_free_func = tsnode_free_func;
    op->op_equal_func = tsnode_equal_func;
    op->op_refcount++;

    return op;
}

typedef enum
{
    TSNODE_PROP_END_BYTE = 0,
    TSNODE_PROP_END_POINT,
    TSNODE_PROP_EXTRA,
    TSNODE_PROP_MISSING,
    TSNODE_PROP_NAMED,
    TSNODE_PROP_START_BYTE,
    TSNODE_PROP_START_POINT,
    TSNODE_PROP_STRING,
    TSNODE_PROP_SYMBOL,
    TSNODE_PROP_TREE,
    TSNODE_PROP_TYPE,
    TSNODE_PROP_NODE = 99
} TSVimNodeProperty;

/*
 * Return a typval_T value of the specified property for the node.
 */
    typval_T
tsnode_property(opaque_T *node_obj, char_u *property)
{
    static keyvalue_T properties[] = {
	KEYVALUE_ENTRY(TSNODE_PROP_END_BYTE, "end_byte"),
	KEYVALUE_ENTRY(TSNODE_PROP_END_POINT, "end_point"),
	KEYVALUE_ENTRY(TSNODE_PROP_EXTRA, "extra"),
	KEYVALUE_ENTRY(TSNODE_PROP_MISSING, "missing"),
	KEYVALUE_ENTRY(TSNODE_PROP_NAMED, "named"),
	KEYVALUE_ENTRY(TSNODE_PROP_START_BYTE, "start_byte"),
	KEYVALUE_ENTRY(TSNODE_PROP_START_POINT, "start_point"),
	KEYVALUE_ENTRY(TSNODE_PROP_STRING, "string"),
	KEYVALUE_ENTRY(TSNODE_PROP_SYMBOL, "symbol"),
	KEYVALUE_ENTRY(TSNODE_PROP_TREE, "tree"),
	KEYVALUE_ENTRY(TSNODE_PROP_TYPE, "type"),
    };
    TSNode node = OP2TSNODE(node_obj);
    keyvalue_T target;
    keyvalue_T *entry;
    typval_T ret = {0};

    target.key = 0;
    target.value.string = property;
    target.value.length = 0;	// not used, see cmp_keyvalue_value()

    entry = (keyvalue_T *)bsearch(&target, &properties, ARRAY_LENGTH(properties),
				  sizeof(properties[0]), cmp_keyvalue_value);

    if (entry == NULL)
    {
	semsg(_(e_tsnode_property_str_no_exist), property);
	ret.v_type = VAR_UNKNOWN;
    }
    else
	switch (entry->key)
	{
	    case TSNODE_PROP_END_BYTE:
		ret.v_type = VAR_NUMBER;
		ret.vval.v_number = ts_node_end_byte(node);
		break;
	    case TSNODE_PROP_END_POINT:
		ret.v_type = VAR_TUPLE;
		ret.vval.v_tuple = tspoint_to_tuple(ts_node_end_point(node));
		break;
	    case TSNODE_PROP_EXTRA:
		ret.v_type = VAR_BOOL;
		ret.vval.v_number = ts_node_is_extra(node);
		break;
	    case TSNODE_PROP_MISSING:
		ret.v_type = VAR_BOOL;
		ret.vval.v_number = ts_node_is_missing(node);
		break;
	    case TSNODE_PROP_NAMED:
		ret.v_type = VAR_BOOL;
		ret.vval.v_number = ts_node_is_named(node);
		break;
	    case TSNODE_PROP_START_BYTE:
		ret.v_type = VAR_NUMBER;
		ret.vval.v_number = ts_node_start_byte(node);
		break;
	    case TSNODE_PROP_START_POINT:
		ret.v_type = VAR_TUPLE;
		ret.vval.v_tuple = tspoint_to_tuple(ts_node_start_point(node));
		break;
	    case TSNODE_PROP_STRING:
		ret.v_type = VAR_STRING;
		ret.vval.v_string = (char_u *)ts_node_string(node);
		break;
	    case TSNODE_PROP_SYMBOL:
		ret.v_type = VAR_NUMBER;
		ret.vval.v_number = ts_node_symbol(node);
		break;
	    case TSNODE_PROP_TREE:;
		opaque_T *tree_obj = OP2TSNODETREE(node_obj);

		tree_obj->op_refcount++;
		ret.v_type = VAR_OPAQUE;
		ret.vval.v_opaque = tree_obj;
		break;
	    case TSNODE_PROP_TYPE:
		ret.v_type = VAR_STRING;
		ret.vval.v_string = vim_strsave((char_u *)ts_node_type(node));
		break;
	    default:
		// Shouldn't happen
		semsg(_(e_tsnode_property_str_no_exist), property);
		ret.v_type = VAR_UNKNOWN;
		break;
	}

    return ret;
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
    opaque_T *
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

    op = opaque_new(TSQUERY, true, &query, sizeof(TSQuery *));

    if (op == NULL)
    {
	ts_query_delete(query);
	return NULL;
    }

    op->op_free_func = tsquery_free_func;
    op->op_equal_func = opaque_equal_ptr;
    op->op_refcount++;

    return op;
}

#endif // FEAT_TREESITTER
