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

// This is not exposed in vimscript. It is held internally in a hash table to be
// used as needed.
typedef struct
{
    HANDLE		dll_handle;
    const TSLanguage	*tl_lang;
    char_u		name[1]; // Actually longer
} vts_language_T;

#define VTS_LANG_OFF (offsetof(vts_language_T, name))
#define HI2LANG(hi) ((vts_language_T *)(hi)->hi_key - VTS_LANG_OFF)

// Table of loaded TSLanguage objects. Each key the language name.
static hashtab_T languages;

// Singly linked list of parsers that are currently parsing, but are deferred to
// a later time. Each parser is completed before going onto the next one.
static vts_parser_T *pending_parser;

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
vts_load_language(char_u *name, char_u *path, char_u *symbol_name)
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
	vts_language_T	*obj =
	    alloc(VTS_LANG_OFF + STRLEN(name) + 1);

	if (obj == NULL)
	    return;

	STRCPY(obj->name, name);
	obj->tl_lang = lang_obj;
	obj->dll_handle = h;

	hash_add_item(&languages, hi, obj->name, hash);
    }
    else
    {
	// Replace assigned TSLanguage object with new one. This does not affect
	// any objects that were created using this language, everything is
	// refcounted.
	vts_language_T	*lang = HI2LANG(hi);

	ts_language_delete(lang->tl_lang);
	lang->tl_lang = lang_obj;
    }
}

/*
 * Allocate a new object representing a TSParser. Returns NULL on failure.
 */
    vts_parser_T *
vts_parser_new(char_u *language)
{
    vts_parser_T    *new;
    hashitem_T	    *hi;
    vts_language_T  *lang;
    TSParser	    *parser;

    // Lookup language object and set parser to it.
    hi = hash_find(&languages, language);

    if (HASHITEM_EMPTY(hi))
    {
	semsg(_(e_treesitter_lang_not_loaded), language);
	return NULL;
    }
    else
	lang = HI2LANG(hi);

    parser = ts_parser_new();

    if (parser == NULL)
	// No documentation on how the function handles errors, but still check
	// for them.
	return NULL;

    new = ALLOC_ONE(vts_parser_T);

    if (new == NULL)
    {
	ts_parser_delete(parser);
	return NULL;
    }

    ts_parser_set_language(parser, lang->tl_lang);
    ga_init2(&new->tp_children, sizeof(vts_parser_T), 2);
    new->tp_parser = parser;
    new->tp_tree = NULL;
    new->tp_next = NULL;

    return new;
}

    void
vts_parser_free(vts_parser_T *self)
{
    ts_parser_delete(self->tp_parser);

    for (int i = 0; i < self->tp_children.ga_len; i++)
    {
	vts_parser_T *child = ((vts_parser_T **)self->tp_children.ga_data)[i];

	vts_parser_free(child);
    }
    ga_clear(&self->tp_children);

    vim_free(self);
}

/*
 * Used to check the current elapsed time while parsing.
 */
    static bool 
vts_parser_parse_progress_callback(TSParseState *state)
{
#ifdef ELAPSED_FUNC
    vts_parser_T *parser = state->payload;

    return ELAPSED_FUNC(parser->tp_elapsed) >= parser->tp_timeout;
#else
    return false;
#endif
}

/*
 * Start the parser using the given TSInput parameters, which must be set before
 * calling this. It will do an initial parse before deferring, see below.
 *
 * "timeout" is how long until a single parse operation should
 * timeout. After that, it will defer another parse operation later and return
 * back into the event loop. "timeout" must be greater than 0 and is in
 * milliseconds.
 *
 * "input" will be copied, so no need to worry about it going out of scope.
 *
 * Returns OK on success, and FAIL if deferred.
 */
    int
vts_parser_start_parse(vts_parser_T *self, TSInput *input, long timeout)
{
    TSParseOptions opts;
    TSTree *tree;

    opts.payload = self;
    opts.progress_callback = vts_parser_parse_progress_callback;

#ifdef ELAPSED_FUNC
    ELAPSED_INIT(self->tp_elapsed);
#endif
    ts_parser_reset(self->tp_parser);
    self->tp_input = *input;

    tree = ts_parser_parse_with_options(
	    self->tp_parser, self->tp_tree->tt_tree, self->tp_input, opts
	    );

    if (tree == NULL)
    {
	// Append to tail of list
	vts_parser_T *parser = pending_parser;

	while (true)
	{
	    if (parser == NULL)
		pending_parser = self;
	    else if (parser->tp_next != NULL)
	    {
		parser = parser->tp_next;
		break;
	    }
	    else
		parser->tp_next = self;
	    break;
	}
	self->tp_parsing = true;

	return FAIL;
    }
    self->tp_parsing = false;

    // TODO: set tree obj
    self->tp_tree = NULL;
    return OK;
}

/*
 * Start a parse operation on the current parser for a while. Returns elapsed
 * time in milliseconds it took. "timeout" is in milliseconds.
 */
    long
vts_do_pending_parsing(long timeout)
{
#ifdef ELAPSED_FUNC
    elapsed_T start;
#endif

    if (pending_parser == NULL)
	return 0;

#ifdef ELAPSED_FUNC
    ELAPSED_INIT(start);
#endif

    pending_parser->tp_timeout = timeout;
    if (vts_parser_start_parse(
		pending_parser, &pending_parser->tp_input, timeout) == OK)
    {
	// Done procesing parser, go onto next one.
	pending_parser = pending_parser->tp_next;
    }
#ifdef ELAPSED_FUNC
    return ELAPSED_FUNC(start);
#else
    return timeout;
#endif
}

#endif // FEAT_TREESITTER
