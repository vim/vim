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

typedef enum
{
    TSOBJECT_TYPE_PARSER,
    TSOBJECT_TYPE_TREE,
    TSOBJECT_TYPE_NODE,
    TSOBJECT_TYPE_QUERY,
    TSOBJECT_TYPE_QUERYCURSOR,
    TSOBJECT_TYPE_QUERYMATCH,
} tsobject_type_T;

struct tsobject_S
{
    int		    to_refcount;
    tsobject_type_T to_type;
    union
    {
	TSParser	*to_parser;
	TSTree	    	*to_tree;
	TSNode	    	to_node;
	TSQuery		*to_query;
	TSQueryCursor   *to_querycursor;
	TSQueryMatch	to_querymatch;
    };
};

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
 * Allocate new treesitter object. Returns NULL on failure.
 */
    static tsobject_T *
tsobject_new(tsobject_type_T type)
{
    tsobject_T *obj = ALLOC_ONE(tsobject_T);

    if (obj == NULL)
	return NULL;

    obj->to_refcount = 1;
    obj->to_type = type;
    
    return obj;
}

    tsobject_T *
tsobject_ref(tsobject_T *obj)
{
    obj->to_refcount++;
    return obj;
}

    void
tsobject_free(tsobject_T *obj)
{
    switch (obj->to_type)
    {
	case TSOBJECT_TYPE_PARSER:
	    if (obj->to_parser != NULL)
		ts_parser_delete(obj->to_parser);
	    break;
	case TSOBJECT_TYPE_TREE:
	    if (obj->to_tree != NULL)
		ts_tree_delete(obj->to_tree);
	    break;
	case TSOBJECT_TYPE_NODE:
	    break;
	case TSOBJECT_TYPE_QUERY:
	    if (obj->to_query != NULL)
		ts_query_delete(obj->to_query);
	    break;
	case TSOBJECT_TYPE_QUERYCURSOR:
	    if (obj->to_querycursor != NULL)
		ts_query_cursor_delete(obj->to_querycursor);
	    break;
	case TSOBJECT_TYPE_QUERYMATCH:
	    break;

    }
    vim_free(obj);
}

    void
tsobject_unref(tsobject_T *obj)
{
    if (obj != NULL && --obj->to_refcount <= 0)
	tsobject_free(obj);
}

    bool
tsobject_equal(tsobject_T *a, tsobject_T *b)
{
    if (a->to_type != b->to_type)
	return false;

    switch (a->to_type)
    {
	case TSOBJECT_TYPE_PARSER:
	    return a->to_parser == b->to_parser;
	case TSOBJECT_TYPE_TREE:
	    return a->to_tree == b->to_tree;
	case TSOBJECT_TYPE_NODE:
	    return ts_node_eq(a->to_node, b->to_node);
	case TSOBJECT_TYPE_QUERY:
	    return a->to_query == b->to_query;
	case TSOBJECT_TYPE_QUERYCURSOR:
	    return a->to_querycursor == b->to_querycursor;
	case TSOBJECT_TYPE_QUERYMATCH:
	    return false;
    }
    return false;
}

/*
 * Returns statically allocated string
 */
    char_u *
tsobject_get_name(tsobject_T *obj)
{
    switch (obj->to_type)
    {
	case TSOBJECT_TYPE_PARSER:
	    return (char_u *)"TSParser";
	case TSOBJECT_TYPE_TREE:
	    return (char_u *)"TSTree";
	case TSOBJECT_TYPE_NODE:
	    return (char_u *)"TSNode";
	case TSOBJECT_TYPE_QUERY:
	    return (char_u *)"TSQuery";
	case TSOBJECT_TYPE_QUERYCURSOR:
	    return (char_u *)"TSQueryCursor";
	case TSOBJECT_TYPE_QUERYMATCH:
	    return (char_u *)"TSQueryMatch";
    }
    return (char_u *)"";
}

    int
tsobject_get_refcount(tsobject_T *obj)
{
    return obj->to_refcount;
}

    bool
tsobject_is_parser(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_PARSER;
}

    bool
tsobject_is_tree(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_TREE;
}

    bool
tsobject_is_node(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_NODE;
}

    bool
tsobject_is_query(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_QUERY;
}

    bool
tsobject_is_querycursor(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_QUERYCURSOR;
}

    bool
tsobject_is_querymatch(tsobject_T *obj)
{
    return obj->to_type == TSOBJECT_TYPE_QUERYMATCH;
}

    int
check_tsobject_type_arg(
	typval_T *args,
	int idx,
	bool opt,
	bool (*func)(tsobject_T *obj))
{
    if (opt && args[idx].v_type == VAR_UNKNOWN)
	return OK;
    if (!func(args[idx].vval.v_tsobject))
    {
        semsg(_(e_tsobject_str_required_for_argument_nr),
		tsobject_get_name(args[idx].vval.v_tsobject), idx);
	return FAIL;
    }
    return OK;
}

/*
 * Create a tuple that represents a TSPoint. Returns NULL on failure.
 */
    static tuple_T *
tspoint_to_tuple(TSPoint point)
{
    typval_T	*row, *column;
    tuple_T	*t = tuple_alloc_with_items(2);

    if (t == NULL)
	return NULL;

    if ((row = alloc_tv()) == NULL)
    {
	tuple_free(t);
	return NULL;
    }
    if ((column = alloc_tv()) == NULL)
    {
	free_tv(row);
	tuple_free(t);
	return NULL;
    }

    tuple_set_item(t, 0, row);
    tuple_set_item(t, 1, column);
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

/*
 * Allocate a new TSParser object. Returns NULL on failure.
 */
    tsobject_T *
tsparser_new(void)
{
    tsobject_T *obj = tsobject_new(TSOBJECT_TYPE_PARSER);

    obj->to_parser = ts_parser_new();

    // Documentation says nothing about it returning NULL but just check to be
    // sure.
    if (obj->to_parser == NULL)
    {
	tsobject_free(obj);
	return NULL;
    }

    return obj;
}

/*
 * Set the given parser to "language".
 */
    void
tsparser_set_language(tsobject_T *parser, char_u *language)
{
    TSVimLanguage *lang = get_language(language);

    if (lang != NULL)
	ts_parser_set_language(parser->to_parser, lang->lang);
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
    tsobject_T *
tsparser_parse_buf(
	tsobject_T *parser,
	tsobject_T *last_tree,
	buf_T *buf,
	long timeout)
{
    TSTree *ltree = last_tree == NULL ? NULL : last_tree->to_tree;
    TSTree *res;
    tsobject_T *obj;

    // Check if parser is set to a language
    if (ts_parser_language(parser->to_parser) == NULL)
    {
	emsg(_(e_tsparser_not_set_to_language));
	return NULL;
    }

    res = tsvim_parser_parse(parser->to_parser, ltree,
	    parse_buf_read_callback, buf, timeout);

    if (res == NULL)
	return NULL;

    obj = tsobject_new(TSOBJECT_TYPE_TREE);
    if (obj == NULL)
    {
	ts_tree_delete(res);
	return NULL;
    }

    obj->to_tree = res;

    return obj;
}

/*
 * Edit the specified tree using the given information describing the edit.
 */
    void
tstree_edit(
	tsobject_T *tree,
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

    ts_tree_edit(tree->to_tree, &edit);
}

/*
 * Return the root node of the tree. Returns NULL on failure.
 */
    tsobject_T *
tstree_root_node(tsobject_T *tree)
{
    tsobject_T *obj = tsobject_new(TSOBJECT_TYPE_NODE);

    if (obj == NULL)
	return NULL;

    obj->to_node = ts_tree_root_node(tree->to_tree);
    return obj;
}

/*
 * Return a dictionary describing the node, returns NULL on failure. The
 * following entries that will be in the dictionary are:
 *
 * "kkkkkkkk
 */
    dict_T *
tsnode_info(tsobject_T *node_obj)
{
    TSNode node = node_obj->to_node;
    dict_T *dict = dict_alloc();

    if (dict == NULL)
	return NULL;

    else if (dict_add_string(dict, "id", (char_u *)node.id) == FAIL)
	goto fail;
    if (dict_add_string(dict, "type", (char_u *)ts_node_type(node)) == FAIL)
	goto fail;
    else if (dict_add_string(dict, "grammar_type",
		(char_u *)ts_node_grammar_type(node)) == FAIL)
	goto fail;
    else if (dict_add_number(dict, "start_byte", ts_node_start_byte(node)) == FAIL)
	goto fail;
    else if (dict_add_tuple(dict, "start",
		tspoint_to_tuple(ts_node_start_point(node))) == FAIL)
	goto fail;
    else if (dict_add_number(dict, "end_byte", ts_node_end_byte(node)) == FAIL)
	goto fail;
    else if (dict_add_tuple(dict, "end",
		tspoint_to_tuple(ts_node_end_point(node))) == FAIL)
	goto fail;

    dict->dv_refcount++;
    return dict;
fail:
    dict_unref(dict);
    return NULL;
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
    tsobject_T *
tsquery_new(char_u *language, char_u *query_str)
{
    TSVimLanguage   *lang = get_language(language);
    TSQuery	    *query;
    uint32_t	    error_offset;
    TSQueryError    error_type;
    tsobject_T	    *obj;

    if (lang == NULL)
	return NULL;

    query = ts_query_new(lang->lang, (char *)query_str,
	    STRLEN(query_str), &error_offset, &error_type);

    if (query == NULL)
    {
	query_do_error_message(query_str, error_offset, error_type);
	return NULL;
    }

    obj = tsobject_new(TSOBJECT_TYPE_QUERY);

    if (obj == NULL)
    {
	ts_query_delete(query);
	return NULL;
    }

    obj->to_query = query;

    return obj;
}

#endif // FEAT_TREESITTER
