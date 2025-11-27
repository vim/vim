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

// This is not exposed in vimscript. It is held internally in a hash table to be
// used as needed.
typedef struct
{
    HANDLE		dll;
    const TSLanguage	*lang;
    char_u		name[1]; // Actually longer
} TSVimLanguage;

#define VTS_LANG_OFF offsetof(TSVimLanguage, name)
#define HI2LANG(hi) ((TSVimLanguage *)((hi)->hi_key - VTS_LANG_OFF))

typedef struct {
    TSTree *tree;
    garray_T children;
} TSVimTree;

// Holds the state for a single treesitter parser instance
struct TSVimState
{
    TSParser *parser;
    union
    {
	buf_T *buf;
    } source;

    TSVimTree *tree;

    const TSLanguage *cur_lang;
    TSVimState *next;

};

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T    languages;
static TSVimState   *pending_state;

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

    // Check if parser is set to a language
    if (ts_parser_language(parser) == NULL)
	return NULL;

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

    static TSVimState *
tsvim_state_create(void)
{
    TSVimState *state = ALLOC_ONE(TSVimState); 

    if (state == NULL)
	return NULL;

    state->parser = ts_parser_new();

    // Not sure if function can fail but still check for it
    if (state->parser == NULL)
    {
	vim_free(state);
	return NULL;
    }
    state->tree = NULL;
    state->cur_lang = NULL;
    state->next = NULL;

    return state;
}

    void
tsvim_state_free(TSVimState *state)
{
    TSVimState *s = pending_state;

    if (s == state)
	pending_state = state->next;
    else
	while (s != NULL)
	{
	    if (s->next == state)
		s->next = state->next;
	}

    ts_parser_delete(state->parser);
    if(state->tree != NULL)
	ts_tree_delete(state->tree);
    vim_free(state);
}

// Treesitter internally updates the "position" argument by counting newlines in
// the text we return.
    static const char *
parse_buf_read_callback(
	void *payload,
	uint32_t byte_index,
	TSPoint position,
	uint32_t *bytes_read)
{
    buf_T	*bp = payload;
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

    void
tsvim_parse_buf(buf_T *buf)
{
    hashitem_T	    *hi;
    TSVimLanguage   *lang;
    TSVimState	    *state;
    TSTree	    *tree;

    // Lookup language object and set parser to it.
    hi = hash_find(&languages, buf->b_p_tslg);

    if (HASHITEM_EMPTY(hi))
    {
	semsg(_(e_treesitter_lang_not_loaded), buf->b_p_tslg);
	return;
    }
    else
	lang = HI2LANG(hi);

    if (buf->b_tsstate == NULL)
    {
	buf->b_tsstate = tsvim_state_create();

	if (buf->b_tsstate == NULL)
	    return;
    }
    state = buf->b_tsstate;

    ts_parser_set_language(state->parser, lang->lang);

    // TODO: handle case when buffer is destroyed while parsing
    tree = tsvim_parser_parse(state->parser, state->tree,
	    parse_buf_read_callback, buf, 10);

    if (tree != NULL)
    {
	state->tree = tree;
	smsg("Done!");
	return;
    }
    state->cur_lang = lang->lang;
    // Defer next parse later
    if (pending_state == NULL)
	pending_state = state;
    else
    {
	TSVimState *s = pending_state;
	while (s != NULL)
	{
	    if (s->next == NULL)
		break;
	    else
		s = s->next;
	}
	s->next = state;
    }
    state->source.buf = buf;
}

    bool
tsvim_parse_pending(void)
{
    TSTree	    *tree;

    if (pending_state == NULL)
	return false;

    tree = tsvim_parser_parse(pending_state->parser, pending_state->tree,
	    parse_buf_read_callback, pending_state->source.buf, 3);

    if (tree != NULL)
    {
	pending_state->tree = tree;
	pending_state = pending_state->next;
	smsg("Done pending!");
	return false;
    }

    return true;
}

#endif // FEAT_TREESITTER
