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
#define TS_VIM_BUFSIZE 4096

// This is not exposed in vimscript. It is held internally in a hash table to be
// used as needed.
typedef struct
{
    HANDLE		tl_dll;
    const TSLanguage	*tl_lang;
    char_u		tl_name[1]; // Actually longer
} ts_vim_language_T;

#define VTS_LANG_OFF offsetof(ts_vim_language_T, tl_name)
#define HI2LANG(hi) ((ts_vim_language_T *)((hi)->hi_key - VTS_LANG_OFF))

// Represents a TSParser object. Each buffer has its own TSParser.
struct ts_vim_parser_S
{
    TSParser	    	*tp_parser;
    garray_T		tp_trees; // Array of TSTree objects
    
    // Next parser that should be processed
    ts_vim_parser_T	*tp_next;
};

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T languages;

    int
init_treesitter(void)
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
ts_vim_load_language(char_u *name, char_u *path, char_u *symbol_name)
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
	ts_vim_language_T	*obj =
	    alloc(VTS_LANG_OFF + STRLEN(name) + 1);

	if (obj == NULL)
	    return;

	STRCPY(obj->tl_name, name);
	obj->tl_lang = lang_obj;
	obj->tl_dll = h;

	hash_add_item(&languages, hi, obj->tl_name, hash);
    }
    else
    {
	// Replace assigned TSLanguage object with new one. This does not affect
	// any objects that were created using this language, everything is
	// refcounted.
	ts_vim_language_T	*lang = HI2LANG(hi);

	ts_language_delete(lang->tl_lang);
	lang->tl_lang = lang_obj;
	close_dll(lang->tl_dll);
	lang->tl_dll = h;
    }
}

/*
 * Allocate new parser object, returns NULL on failure.
 */
    static ts_vim_parser_T *
ts_vim_parser_new(void)
{
    ts_vim_parser_T *vparser = ALLOC_CLEAR_ONE(ts_vim_parser_T);

    if (vparser == NULL)
	return NULL;

    vparser->tp_parser = ts_parser_new();

    // Docs don't say anything about this function returning NULL but check
    // it to be safe.
    if (vparser->tp_parser == NULL)
    {
	vim_free(vparser);
	return NULL;
    }
    
    ga_init2(&vparser->tp_trees, sizeof(TSTree *), 2);

    return vparser;
}

/*
 * Free resources allocated for parser object.
 */
    void
ts_vim_parser_free(ts_vim_parser_T *vparser)
{
    TSTree **trees = vparser->tp_trees.ga_data;

    // Free TSTree objects
    for (int i = 0; i < vparser->tp_trees.ga_len; i++)
	ts_tree_delete(trees[i]);

    ga_clear(&vparser->tp_trees);
    ts_parser_delete(vparser->tp_parser);

    vim_free(vparser);
}

#ifdef ELAPSED_FUNC
    static bool
parse_progress_callback(TSParseState *state)
{
    return ELAPSED_FUNC(*((elapsed_T *)state->payload)) >= 1000;
}
#endif

/*
 * Copied from Neovim with slight modifications.
 */
    static const char *
parse_read_buf_callback(
	void *payload,
	uint32_t byte_index,
	TSPoint position,
	uint32_t *bytes_read)
{
    buf_T	    *b = payload;
    static char	    buf[TS_VIM_BUFSIZE];
    char	    *mbuf, *end;
    linenr_T	    lnum;
    char	    *line;
    size_t	    len, tocopy;

    if ((linenr_T)position.row >= b->b_ml.ml_line_count) {
	*bytes_read = 0;
	return "";
    }
    lnum = (linenr_T)position.row + 1;
    line = (char *)ml_get_buf(b, lnum, FALSE);
    len = (size_t)ml_get_buf_len(b, lnum);

    if (position.column > len) {
	*bytes_read = 0;
	return "";
    }

    tocopy = MIN(len - position.column, TS_VIM_BUFSIZE);
    memcpy(buf, line + position.column, tocopy);

    // Translate embedded \n to NUL
    mbuf = buf;
    end = buf + len;
    while ((mbuf = memchr(mbuf, '\n', (size_t)(end - mbuf)))) {
	*mbuf++ = NUL;
    }

    *bytes_read = (uint32_t)tocopy;
    if (tocopy < TS_VIM_BUFSIZE) {
	// Now add the final \n, if it is meant to be present for this buffer.
	// If it didn't fit, input_cb will be called again on the same line with
	// advanced column.
	if (lnum != b->b_ml.ml_line_count || (!b->b_p_bin && b->b_p_fixeol)
		|| (lnum != b->b_no_eol_lnum && b->b_p_eol)) {
	    buf[tocopy] = '\n';
	    (*bytes_read)++;
	}
  }
  return buf;
}

// TODO: free parser when buf is freed

/*
 * Start parsing the given buffer asynchronously. We simulate async behaviour by
 * setting a timeout for each parse operation. If the timeout is reached, then
 * the parse operation is deferred later.
 */
    void
ts_vim_parse_buf(buf_T *buf)
{
    static char_u	*current_lang; // Current language the parser is set to.
    static TSTree	*last_tree;

    TSTree		*tree;
    TSInput		input;
    TSParseOptions	opts;
    ts_vim_parser_T	*vparser;
    ts_vim_language_T   *vlang;
#ifdef ELAPSED_FUNC
    elapsed_T		start; // Used to cancel parse if it is taking too long
#endif
    char_u		*lang = current_lang == NULL
				? buf->b_p_tslg : current_lang;
    hashitem_T		*hi = hash_find(&languages, lang);

    if (HASHITEM_EMPTY(hi))
    {
	semsg(_(e_treesitter_lang_not_loaded), lang);
	return;
    }
    vlang = HI2LANG(hi);

    // Create parser for buffer if we didn't already
    if (buf->b_ts_parser == NULL
	    && (buf->b_ts_parser = ts_vim_parser_new()) == NULL)
	return;

    vparser = buf->b_ts_parser;

    // Try growing tree array first
    if (ga_grow(&vparser->tp_trees, 1) == FAIL)
	return;
    
#ifdef ELAPSED_FUNC
    opts.payload = &start;
    opts.progress_callback = parse_progress_callback;
#else
    // Make it synchronous
    memset(opts, 0, sizeof(opts));
#endif

    input.decode = NULL;
    input.encoding = TSInputEncodingUTF8;
    input.payload = buf;
    input.read = parse_read_buf_callback;

    ts_parser_set_language(vparser->tp_parser, vlang->tl_lang);
    tree = ts_parser_parse_with_options(vparser->tp_parser, last_tree, input, opts);

    if (tree == NULL)
	return;

    // Finished parsing, add result tree to array
    ((TSTree **)vparser->tp_trees.ga_data)[vparser->tp_trees.ga_len++] = tree;
}

#endif // FEAT_TREESITTER
