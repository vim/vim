#include "vim.h"

#if defined(FEAT_JOB_CHANNEL)

/*
 * Filter mask and verbosity — written by trace_apply_opt().
 */
static uint32_t trace_filter_mask = 0;
static trace_verbosity_T trace_verbosity = TRACE_VERBOSITY_NORMAL;

/*
 * Ring storage
 */
static trace_entry_T *trace_ring = NULL;
static size_t trace_ring_size = 512;
static size_t trace_ring_head = 0;
static size_t trace_ring_count = 0;
static uint64_t trace_seq = 1;

/*
 * Guard against re-entrant tracing during dump.
 */
static int trace_dumping = FALSE;

/*
 * Capture-time depth tracking for indentation.
 * MAPPING increments pending_depth for the next TYPEBUF;
 * injected TYPEBUF inherits current depth; user-typed TYPEBUF resets.
 */
static int trace_capture_depth = 0;
static int trace_capture_pending_depth = 0;

/*
 * Forward declarations
 */
static trace_entry_T *trace_ring_alloc(void);
static int trace_is_cmdline_printable(trace_entry_T *entry);

/*
 * Mapping from format-string kind name to trace_event_kind_T.
 * Used by trace_ingest() to parse "TRACE|KIND|...".
 */
    static int
trace_kind_from_name(char_u *name)
{
    if (name == NULL) return -1;
    if (STRCMP(name, "INPUT") == 0) return TRACE_INPUT;
    if (STRCMP(name, "COMMAND") == 0) return TRACE_COMMAND;
    if (STRCMP(name, "EX") == 0) return TRACE_EX;
    if (STRCMP(name, "CMD") == 0) return TRACE_CMD;
    if (STRCMP(name, "MAPPING") == 0) return TRACE_MAPPING;
    if (STRCMP(name, "TYPEBUF") == 0) return TRACE_TYPEBUF;
    if (STRCMP(name, "AUTOCMD") == 0) return TRACE_AUTOCMD;
    return -1;
}

/*
 * Convert keycode to readable name.
 */
    static char_u *
trace_key_name(int c)
{
    char_u buf[NUMBUFLEN];

    if (c == ' ')
	return vim_strsave((char_u *)"<Space>");

    if (IS_SPECIAL(c) || c < ' ' || c == DEL)
	return vim_strsave(get_special_key_name(c, 0));

    vim_snprintf((char *)buf, NUMBUFLEN, "%c", c);
    return vim_strsave(buf);
}

/*
 * Format a TRACE_INPUT event into a string for ch_log() ingestion.
 * Returns allocated string; caller must vim_free().
 */
    char_u *
trace_format_input(int c)
{
    return trace_key_name(c);
}

/*
 * Format a TRACE_COMMAND event into a string for ch_log() ingestion.
 * Returns allocated string; caller must vim_free().
 */
    char_u *
trace_format_command(cmdarg_T *ca)
{
    garray_T    msg_ga;

    if (ca == NULL)
	return vim_strsave((char_u *)"");

    ga_init2(&msg_ga, sizeof(char), 16);

    if (ca->oap != NULL
	    && ca->oap->regname != NUL
	    && ca->oap->regname != '"'
	    && ca->cmdchar != '"')
    {
	char reg = (char)ca->oap->regname;

	ga_concat(&msg_ga, (char_u *)"\"");
	if (reg == ' ' || reg < ' ' || reg == DEL)
	{
	    char_u *kn = trace_key_name((int)(unsigned char)reg);
	    if (kn != NULL)
	    {
		ga_concat(&msg_ga, kn);
		vim_free(kn);
	    }
	}
	else
	    ga_append(&msg_ga, (int)(unsigned char)reg);
    }

    {
	char_u *kn = trace_key_name((int)(unsigned char)ca->cmdchar);
	if (kn != NULL)
	{
	    ga_concat(&msg_ga, kn);
	    vim_free(kn);
	}
    }

    ga_append(&msg_ga, NUL);
    return (char_u *)msg_ga.ga_data;
}

/*
 * Format a TRACE_EX event into a string for ch_log() ingestion.
 * Returns allocated string; caller must vim_free().
 */
    char_u *
trace_format_ex(exarg_T *ea)
{
    char	    body[512];

    if (ea == NULL || ea->cmd == NULL || *ea->cmd == NUL)
	return vim_strsave((char_u *)"");

    if (ea->arg != NULL && *ea->arg != NUL && ea->arg > ea->cmd)
    {
	size_t cmdlen = (size_t)(ea->arg - ea->cmd);
	if (cmdlen > 0)
	    --cmdlen;
	vim_snprintf(body, sizeof(body), "%.*s %s",
		     (int)cmdlen, (char *)ea->cmd,
		     (char *)ea->arg);
    }
    else
    {
	vim_snprintf(body, sizeof(body), "%s", (char *)ea->cmd);
    }

    return vim_strsave((char_u *)body);
}

/*
 * Format a TRACE_MAPPING event into a string for ch_log() ingestion.
 * Returns allocated string; caller must vim_free().
 */
    char_u *
trace_format_mapping(mapblock_T *mp)
{
    char_u	   *lhs;
    char_u	   *rhs;
    char	    body[512];

    if (mp == NULL)
	return vim_strsave((char_u *)"");

    lhs = str2special_save(mp->m_keys, TRUE, FALSE);
    rhs = str2special_save(mp->m_str, TRUE, FALSE);

    if (lhs == NULL)
    {
	vim_free(rhs);
	return vim_strsave((char_u *)"");
    }

    if (rhs != NULL)
	vim_snprintf(body, sizeof(body), "%s -> %s",
		     (char *)lhs, (char *)rhs);
    else
	vim_snprintf(body, sizeof(body), "%s", (char *)lhs);

    vim_free(lhs);
    vim_free(rhs);

    return vim_strsave((char_u *)body);
}

/*
 * Format a TRACE_TYPEBUF event into a string for ch_log() ingestion.
 * Returns allocated string; caller must vim_free().
 */
    char_u *
trace_format_typebuf(char_u *buf, size_t buflen)
{
    char_u   *escaped;
    char	    body[512];

    if (buf == NULL || buflen == 0)
	return vim_strsave((char_u *)"");

    escaped = str2special_save(buf, TRUE, TRUE);
    if (escaped == NULL)
	return vim_strsave((char_u *)"");

    vim_snprintf(body, sizeof(body), "text=%s", (char *)escaped);
    vim_free(escaped);

    return vim_strsave((char_u *)body);
}


/*
 * Map event kind to filter bit.
 */
    static uint32_t
trace_kind_filter(trace_event_kind_T kind)
{
    switch (kind)
    {
	case TRACE_INPUT:
	case TRACE_TYPEBUF:
	    return TRACE_FILTER_INPUT;
	case TRACE_COMMAND: return TRACE_FILTER_COMMAND;
	case TRACE_CMD:
	case TRACE_EX:      return TRACE_FILTER_EX;
	case TRACE_MAPPING: return TRACE_FILTER_MAPPING;
	case TRACE_AUTOCMD: return TRACE_FILTER_AUTOCMD;
	default:            return 0;
    }
}

    int
trace_is_enabled(trace_event_kind_T kind)
{
    return (trace_filter_mask & trace_kind_filter(kind)) != 0;
}

    int
trace_verbose(trace_verbosity_T level)
{
    return trace_verbosity >= level;
}

    int
trace_is_active(void)
{
    return trace_filter_mask != 0;
}

/*
 * Parse the 'chtraceopt' option value and apply filter mask,
 * verbosity, and ring size.
 */
    void
trace_apply_opt(char_u *value)
{
    char_u		*copy;
    char_u		*tok;
    char_u		*p;
    uint32_t		mask = 0;
    trace_verbosity_T	verbosity = TRACE_VERBOSITY_NORMAL;
    size_t		ringsize = 512;
    int			has_ringsize = FALSE;

    static struct {
	char	*name;
	trace_verbosity_T value;
    } verbosity_table[] = {
	{"minimal", TRACE_VERBOSITY_MINIMAL},
	{"normal", TRACE_VERBOSITY_NORMAL},
	{"verbose", TRACE_VERBOSITY_VERBOSE},
	{"debug", TRACE_VERBOSITY_DEBUG},
	{NULL, 0}
    };

    static struct {
	char	*name;
	uint32_t bit;
    } filter_table[] = {
	{"input",   TRACE_FILTER_INPUT},
	{"command", TRACE_FILTER_COMMAND},
	{"operator", TRACE_FILTER_OPERATOR},
	{"ex",	    TRACE_FILTER_EX},
	{"mapping", TRACE_FILTER_MAPPING},
	{"autocmd", TRACE_FILTER_AUTOCMD},
	{"script",  TRACE_FILTER_SCRIPT},
	{"mode",    TRACE_FILTER_MODE},
	{NULL, 0}
    };

    if (value == NULL || *value == NUL)
    {
	trace_filter_mask = 0;
	trace_verbosity = TRACE_VERBOSITY_NORMAL;
	return;
    }

    copy = vim_strsave(value);
    if (copy == NULL)
	return;

    p = copy;

    while ((tok = (char_u *)vim_strchr(p, ',')) != NULL || *p != NUL)
    {
	if (tok != NULL)
	    *tok = NUL;

	if (STRNCMP(p, "verbosity:", 10) == 0)
	{
	    char_u *val = p + 10;

	    for (int i = 0; verbosity_table[i].name != NULL; ++i)
	    {
		if (STRCMP(val, (char_u *)verbosity_table[i].name) == 0)
		{
		    verbosity = verbosity_table[i].value;
		    break;
		}
	    }
	}
	else if (STRNCMP(p, "ringsize:", 9) == 0)
	{
	    long n = atol((char *)(p + 9));

	    if (n < 16)
		n = 16;
	    ringsize = (size_t)n;
	    has_ringsize = TRUE;
	}
	else
	{
	    for (int i = 0; filter_table[i].name != NULL; ++i)
	    {
		if (STRCMP(p, (char_u *)filter_table[i].name) == 0)
		{
		    mask |= filter_table[i].bit;
		    break;
		}
	    }
	}

	if (tok == NULL)
	    break;

	p = tok + 1;
    }

    vim_free(copy);

    trace_filter_mask = mask;
    trace_verbosity = verbosity;
    if (has_ringsize)
	trace_resize_ring(ringsize);
}

    static const char *
trace_kind_name(trace_event_kind_T kind)
{
    switch (kind)
    {
	case TRACE_INPUT:
	case TRACE_TYPEBUF:   return "input";
	case TRACE_COMMAND:   return "command";
	case TRACE_CMD:
	case TRACE_EX:        return "ex";
	case TRACE_MAPPING:   return "mapping";
	case TRACE_AUTOCMD:   return "autocmd";
	case TRACE_MODE:      return "mode";
	default:              return "unknown";
    }
}

    static const char *
trace_mode_name(char_u *mode)
{
    if (mode == NULL || *mode == NUL)
	return "UNKNOWN";

    switch (*mode)
    {
	case 'n':     return "NORMAL";
	case 'i':     return "INSERT";
	case 'R':     return "REPLACE";
	case 'v':     return "VISUAL";
	case 'V':     return "V-LINE";
	case Ctrl_V:  return "V-BLOCK";
	case 'c':     return "CMDLINE";
	case 'r':     return "HITRETURN";
	case 's':     return "SELECT";
	case 't':     return "TERMINAL";
	default:      break;
    }

    if (STRNCMP((char *)mode, "ic", 2) == 0) return "INSERT-COMPLETE";
    if (STRNCMP((char *)mode, "ix", 2) == 0) return "INSERT-COMPLETION";
    if (STRNCMP((char *)mode, "Rv", 2) == 0) return "VREPLACE";
    if (STRNCMP((char *)mode, "cv", 2) == 0) return "EX";
    if (STRNCMP((char *)mode, "ce", 2) == 0) return "NORMAL-EX";
    if (STRNCMP((char *)mode, "rm", 2) == 0) return "MORE";

    return (char *)mode;
}

/*
 * Return a canonical mode group for collapsing.
 */
    static const char *
trace_mode_group(char_u *mode)
{
    if (mode == NULL || *mode == NUL)
	return "";

    if (*mode == 'n') return "n";
    if (*mode == 'i') return "i";
    if (*mode == 'c') return "c";
    if (*mode == 'v' || *mode == 'V' || *mode == Ctrl_V)
	return (const char *)mode;
    if (*mode == 'R') return "R";
    if (*mode == 's') return "s";
    if (*mode == 't') return "t";

    return (const char *)mode;
}


/*
 * Initialise the trace ring buffer.
 */
    void
trace_init(void)
{
    if (trace_filter_mask == 0)
	return;
    if (trace_ring == NULL)
    {
	trace_ring = ALLOC_CLEAR_MULT(trace_entry_T, trace_ring_size);
	if (trace_ring == NULL)
	    trace_ring_size = 0;
    }
}

/*
 * Get entry by reverse index.
 * idx=0 => newest, idx=1 => previous, etc.
 */
    trace_entry_T *
trace_get_recent(size_t idx)
{
    size_t pos;

    if (idx >= trace_ring_count)
	return NULL;

    if (trace_ring == NULL)
	return NULL;

    pos = (trace_ring_head + trace_ring_size - 1 - idx) % trace_ring_size;

    if (!trace_ring[pos].in_use)
	return NULL;

    return &trace_ring[pos];
}

/*
 * Check whether a trace entry is a single printable char typed in
 * command-line mode (used for aggregation decisions).
 */
    static int
trace_is_cmdline_printable(trace_entry_T *entry)
{
    if (entry == NULL)
	return FALSE;

    if (entry->kind != TRACE_INPUT && entry->kind != TRACE_TYPEBUF)
	return FALSE;

    if (*entry->mode != 'c')
	return FALSE;

    if (entry->message == NULL || *entry->message == NUL)
	return FALSE;

    if (vim_strchr(entry->message, '<') != NULL
	    || vim_strchr(entry->message, '>') != NULL)
	return FALSE;

    for (char_u *p = entry->message; *p != NUL; p += mb_ptr2len(p))
    {
	if (mb_ptr2len(p) != 1 || !vim_isprintc(*p) || *p == ' ')
	    return FALSE;
    }
    return TRUE;
}

/*
 * Allocate the next ring slot and return it (cleared for reuse).
 * Returns NULL when tracing is suppressed (rendering, shutdown).
 */
    static trace_entry_T *
trace_ring_alloc(void)
{
    trace_entry_T *entry;

    if (trace_dumping)
	return NULL;

    if (trace_ring == NULL)
	trace_init();

    if (trace_ring == NULL)
	return NULL;

    entry = &trace_ring[trace_ring_head];

    vim_free(entry->message);
    vim_free(entry->payload);
    entry->message = NULL;
    entry->payload = NULL;

    entry->seq = trace_seq++;
    entry->repeat_count = 1;
    entry->in_use = TRUE;

    trace_ring_head = (trace_ring_head + 1) % trace_ring_size;
    if (trace_ring_count < trace_ring_size)
	++trace_ring_count;

    return entry;
}


/*
 * Parse an ingested trace event from ch_log().
 * Format: "TRACE|KIND|PAYLOAD|MESSAGE|INJECTED"
 *
 * Derived internally (not from format string):
 *   mode     = get_mode()
 *   depth    = state machine
 *   injected = for INPUT, overridden by !KeyTyped
 *   seq      = trace_seq++
 */
    void
trace_ingest(char_u *buf)
{
    char_u	*kind_start;
    char_u	*payload_start;
    char_u	*msg_start;
    char_u	*injected_start;
    char_u	*p;
    int		kind;
    char_u	mode[MODE_MAX_LENGTH];
    char_u	*payload = NULL;
    char_u	*message = NULL;
    int		injected = 0;
    trace_entry_T *entry;

    if (trace_filter_mask == 0)
	return;

    // Parse: "TRACE|KIND|PAYLOAD|MESSAGE|INJECTED"
    // buf starts with "TRACE|" (guaranteed by caller)
    kind_start = buf + 6;  // skip "TRACE|"

    // Find KIND end
    p = vim_strchr(kind_start, '|');
    if (p == NULL)
	return;
    *p = NUL;
    kind = trace_kind_from_name(kind_start);
    if (kind < 0)
    {
	*p = '|';
	return;
    }
    *p = '|';
    kind_start = p + 1;

    // Skip over PAYLOAD
    payload_start = kind_start;
    p = vim_strchr(payload_start, '|');
    if (p == NULL)
	return;
    *p = NUL;
    payload = vim_strsave(payload_start);
    *p = '|';
    msg_start = p + 1;

    // Skip over MESSAGE
    p = vim_strchr(msg_start, '|');
    if (p != NULL)
    {
	*p = NUL;
	message = vim_strsave(msg_start);
	*p = '|';
	injected_start = p + 1;
	// Parse INJECTED
	if (*injected_start >= '0' && *injected_start <= '1')
	    injected = *injected_start - '0';
    }
    else
    {
	message = vim_strsave(msg_start);
    }

    if (message == NULL)
    {
	vim_free(payload);
	return;
    }

    // Check filter
    if (!trace_is_enabled((trace_event_kind_T)kind))
    {
	vim_free(payload);
	vim_free(message);
	return;
    }

    trace_init();
    if (trace_ring == NULL)
    {
	vim_free(payload);
	vim_free(message);
	return;
    }

    get_mode(mode);

    // Depth state machine (same logic as old trace_process_channel)
    switch ((trace_event_kind_T)kind)
    {
	case TRACE_INPUT:
	    // User-typed input resets depth
	    if (KeyTyped)
		trace_capture_depth = 0;
	    injected = !KeyTyped;
	    break;

	case TRACE_TYPEBUF:
	    if (trace_capture_pending_depth > 0)
	    {
		trace_capture_depth = trace_capture_pending_depth;
		trace_capture_pending_depth = 0;
	    }
	    else if (!injected)
	    {
		trace_capture_depth = 0;
	    }
	    break;

	case TRACE_MAPPING:
	    trace_capture_pending_depth =
		    (trace_capture_depth > 0
			    ? trace_capture_depth : 0) + 1;
	    break;

	default:
	    break;
    }

    // Skip normal-mode input at non-debug verbosity
    if (kind == TRACE_INPUT
	    && STRCMP((char *)mode, "n") == 0
	    && trace_verbosity < TRACE_VERBOSITY_DEBUG)
    {
	vim_free(payload);
	vim_free(message);
	return;
    }

    // Cmdline-mode aggregation: if this is a single printable char
    // and the last entry is also a cmdline printable, append.
    {
	int do_aggregate = FALSE;

	if (kind == TRACE_INPUT
		&& *mode == 'c'
		&& message != NULL
		&& vim_isprintc(*message)
		&& *message != ' '
		&& mb_charlen(message) == 1
		&& vim_strchr(message, '<') == NULL
		&& vim_strchr(message, '>') == NULL
		&& trace_ring_count > 0)
	{
	    trace_entry_T *prev = trace_get_recent(0);
	    if (prev != NULL && prev->in_use
		    && trace_is_cmdline_printable(prev))
	    {
		size_t old_len = STRLEN(prev->message);
		size_t msg_len = STRLEN(message);
		char_u *new_msg = alloc(old_len + msg_len + 1);
		char_u *new_payload = alloc(old_len + msg_len + 1);

		if (new_msg != NULL && new_payload != NULL)
		{
		    mch_memmove(new_msg, prev->message, old_len);
		    mch_memmove(new_msg + old_len, message,
				msg_len + 1);
		    mch_memmove(new_payload, prev->payload, old_len);
		    mch_memmove(new_payload + old_len, message,
				msg_len + 1);

		    vim_free(prev->message);
		    vim_free(prev->payload);
		    prev->message = new_msg;
		    prev->payload = new_payload;
		    do_aggregate = TRUE;
		}
		else
		{
		    vim_free(new_msg);
		    vim_free(new_payload);
		}
	    }
	}

	if (do_aggregate)
	{
	    vim_free(payload);
	    vim_free(message);
	    return;
	}
    }

    // Normal path: allocate ring entry and populate
    entry = trace_ring_alloc();
    if (entry == NULL)
    {
	vim_free(payload);
	vim_free(message);
	return;
    }

    entry->kind = (trace_event_kind_T)kind;
    entry->injected = injected;
    vim_strncpy(entry->mode, mode, MODE_MAX_LENGTH - 1);
    entry->message = message;  // ownership transferred
    entry->payload = payload;  // ownership transferred
    entry->depth = trace_capture_depth;
}

/*
 * Clear entire trace history.
 * Keeps the ring buffer allocated so new events can be recorded immediately.
 */
    void
trace_clear_all(void)
{
    size_t i;

    if (trace_ring == NULL)
	return;

    for (i = 0; i < trace_ring_size; ++i)
    {
	vim_free(trace_ring[i].message);
	vim_free(trace_ring[i].payload);
	trace_ring[i].message = NULL;
	trace_ring[i].payload = NULL;
	trace_ring[i].in_use = FALSE;
    }

    trace_ring_head = 0;
    trace_ring_count = 0;
    trace_seq = 1;
    trace_capture_depth = 0;
    trace_capture_pending_depth = 0;
}

/*
 * Resize the ring buffer, preserving the newest entries.
 */
    int
trace_resize_ring(size_t new_size)
{
    trace_entry_T	*new_ring;
    size_t		copy_count;
    size_t		i;

    if (new_size == 0)
	return FAIL;

    new_ring = ALLOC_CLEAR_MULT(trace_entry_T, new_size);
    if (new_ring == NULL)
	return FAIL;

    copy_count = trace_ring_count < new_size
		   ? trace_ring_count : new_size;

    for (i = 0; i < copy_count; ++i)
    {
	trace_entry_T *src = trace_get_recent(copy_count - i - 1);
	if (src != NULL && src->in_use)
	{
	    new_ring[i] = *src;
	    src->message = NULL;
	    src->payload = NULL;
	    src->in_use = FALSE;
	}
    }

    if (trace_ring != NULL)
    {
	for (i = 0; i < trace_ring_size; ++i)
	{
	    vim_free(trace_ring[i].message);
	    vim_free(trace_ring[i].payload);
	}

	vim_free(trace_ring);
    }

    trace_ring = new_ring;
    trace_ring_size = new_size;
    trace_ring_head = copy_count % new_size;
    trace_ring_count = copy_count;

    return OK;
}


/*
 * ------------------------------------------------------------------
 *  Rendering helpers
 * ------------------------------------------------------------------
 */

/*
 * A single printable char typed in Insert mode.
 */
    static int
trace_is_insert_input(trace_entry_T *entry)
{
    if (entry == NULL)
	return FALSE;

    if (entry->kind != TRACE_INPUT && entry->kind != TRACE_TYPEBUF)
	return FALSE;

    if (*entry->mode != 'i')
	return FALSE;

    if (entry->message == NULL || *entry->message == NUL)
	return FALSE;

    if (vim_isprintc(*entry->message) == 0)
	return FALSE;

    if (vim_strchr(entry->message, '<') != NULL
	    || vim_strchr(entry->message, '>') != NULL)
	return FALSE;

    return mb_charlen(entry->message) == 1;
}

/*
 * Flush aggregated insert text to the output buffer.
 */
    static void
trace_flush_insert(garray_T *gap, garray_T *insert_text,
		   int *collecting, trace_entry_T *source)
{
    char buf[256];

    if (!*collecting || source == NULL)
	return;

    if (insert_text->ga_len == 0)
    {
	ga_clear(insert_text);
	ga_init2(insert_text, sizeof(char), 64);
	*collecting = FALSE;
	return;
    }

    vim_snprintf(buf, sizeof(buf), "#%llu [insert] ",
		 (unsigned long long)source->seq);
    ga_concat(gap, (char_u *)buf);

    ga_append(insert_text, NUL);
    insert_text->ga_len--;

    ga_append(gap, '"');
    ga_concat(gap, insert_text->ga_data);
    ga_append(gap, '"');

    if (*source->mode != NUL)
    {
	ga_concat(gap, (char_u *)"  [");
	ga_concat(gap, (char_u *)trace_mode_name(source->mode));
	ga_append(gap, ']');
    }

    ga_append(gap, '\n');

    ga_clear(insert_text);
    ga_init2(insert_text, sizeof(char), 64);
    *collecting = FALSE;
}

/*
 * Two events are equivalent when an INPUT is followed by a COMMAND
 * with the same payload (e.g. key "y" -> command "y").
 */
    static int
trace_events_equivalent(trace_entry_T *a, trace_entry_T *b)
{
    if (a == NULL || b == NULL)
	return FALSE;

    if (!((a->kind == TRACE_INPUT && b->kind == TRACE_COMMAND)
	    || (a->kind == TRACE_COMMAND && b->kind == TRACE_INPUT)))
	return FALSE;

    if (a->message == NULL || b->message == NULL)
	return FALSE;

    return STRCMP(a->message, b->message) == 0;
}

    static int
trace_same_command(trace_entry_T *a, trace_entry_T *b)
{
    if (a == NULL || b == NULL)
	return FALSE;

    if (a->kind != TRACE_COMMAND || b->kind != TRACE_COMMAND)
	return FALSE;

    if (strcmp(trace_mode_group(a->mode),
	       trace_mode_group(b->mode)) != 0)
	return FALSE;

    if (a->message == NULL || b->message == NULL)
	return FALSE;

    if (*a->message == NUL || *b->message == NUL)
	return FALSE;

    if (a->message[0] == '<' || b->message[0] == '<')
	return FALSE;

    return STRCMP(a->message, b->message) == 0;
}

    static int
trace_same_input(trace_entry_T *a, trace_entry_T *b)
{
    if (a == NULL || b == NULL)
	return FALSE;

    if (a->kind != TRACE_INPUT || b->kind != TRACE_INPUT)
	return FALSE;

    if (strcmp(trace_mode_group(a->mode),
	       trace_mode_group(b->mode)) != 0)
	return FALSE;

    if (a->message == NULL || b->message == NULL)
	return FALSE;

    return STRCMP(a->message, b->message) == 0;
}

    static int
trace_events_collapsible(trace_entry_T *a, trace_entry_T *b)
{
    switch (a->kind)
    {
	case TRACE_COMMAND: return trace_same_command(a, b);
	case TRACE_INPUT:   return trace_same_input(a, b);
	default:            return FALSE;
    }
}

/*
 * Hide an INPUT event when the immediately following visible event
 * is the equivalent COMMAND (avoid "[input] y\n[command] y").
 */
    static int
trace_event_hidden(trace_entry_T *ev, trace_entry_T *next)
{
    if (trace_verbosity >= TRACE_VERBOSITY_DEBUG)
	return FALSE;

    if (next == NULL)
	return FALSE;

    if ((ev->kind == TRACE_INPUT || ev->kind == TRACE_TYPEBUF)
	    && next->kind == TRACE_COMMAND
	    && trace_events_equivalent(ev, next))
	return TRUE;

    return FALSE;
}

/*
 * Walk backwards (newest->oldest) from *idx and return the next
 * visible (non-hidden) event.  *idx is updated to point past it.
 */
    static trace_entry_T *
trace_next_visible(size_t *idx, size_t start)
{
    size_t		j;
    trace_entry_T	*ev;
    trace_entry_T	*next;

    for (j = *idx; j > start; --j)
    {
	ev = trace_get_recent(j - 1);
	if (ev == NULL)
	    continue;

	next = (j > start + 1) ? trace_get_recent(j - 2) : NULL;

	if (!trace_event_hidden(ev, next))
	{
	    *idx = j - 1;
	    return ev;
	}
    }

    return NULL;
}


/*
 * Render a single event (or collapsed range) into the gap.
 */
    static void
trace_render_event(garray_T *gap, trace_entry_T *entry,
		   uint64_t start_seq, uint64_t end_seq)
{
    char buf[256];

    if (entry == NULL)
	return;

    if (start_seq == end_seq)
	vim_snprintf(buf, sizeof(buf), "#%llu ",
		     (unsigned long long)start_seq);
    else
	vim_snprintf(buf, sizeof(buf), "#%llu-#%llu ",
		     (unsigned long long)start_seq,
		     (unsigned long long)end_seq);

    ga_concat(gap, (char_u *)buf);

    ga_append(gap, '[');
    ga_concat(gap, (char_u *)trace_kind_name(entry->kind));
    ga_concat(gap, (char_u *)"] ");

    if (entry->message != NULL)
    {
	size_t msg_len = STRLEN(entry->message);
	size_t max_len = 120;

	if (msg_len > max_len)
	{
	    ga_concat_len(gap, entry->message, max_len);
	    ga_concat(gap, (char_u *)"...");
	}
	else
	    ga_concat(gap, entry->message);
    }

    if (entry->repeat_count > 1)
    {
	vim_snprintf(buf, sizeof(buf), " x%d", entry->repeat_count);
	ga_concat(gap, (char_u *)buf);
    }

    if (*entry->mode != NUL)
    {
	ga_concat(gap, (char_u *)"  [");
	ga_concat(gap, (char_u *)trace_mode_name(entry->mode));
	ga_append(gap, ']');
    }

    ga_append(gap, '\n');
}


/*
 * Dump a range of events (0-based indices), oldest first.
 * Handles hidden-event suppression, collapsing, insert-text aggregation.
 * Renders into a garray_T and returns the content as an allocated string
 * (caller must vim_free).  Returns NULL if nothing to dump.
 */
    static char_u *
trace_dump_to_string(size_t start, size_t end)
{
    size_t		cursor;
    garray_T		ga;
    garray_T		insert_text;
    int			collecting_insert = FALSE;
    int			collecting_insert_depth = 0;
    trace_entry_T	*insert_source = NULL;
    trace_event_kind_T	last_kind = TRACE_COMMAND;
    char_u		last_mode[MODE_MAX_LENGTH];

    last_mode[0] = NUL;

    if (trace_ring == NULL || trace_ring_count == 0)
	return NULL;

    if (start >= trace_ring_count)
	return NULL;

    if (end >= trace_ring_count)
	end = trace_ring_count - 1;

    if (start > end)
	return NULL;

    ga_init2(&ga, sizeof(char), 1024);
    ga_init2(&insert_text, sizeof(char), 64);

    cursor = end + 1;

    while (cursor > start)
    {
	size_t		current_idx;
	trace_entry_T	*ev;
	size_t		next_cursor;
	int		collapsed_repeat = 1;
	uint64_t	render_start_seq;
	uint64_t	render_end_seq;
	trace_entry_T	*next_visible;

	current_idx = cursor;
	ev = trace_next_visible(&current_idx, start);
	if (ev == NULL)
	    break;

	next_cursor = current_idx;

	next_visible = NULL;
	if (current_idx > start)
	{
	    size_t lookahead = current_idx;
	    next_visible = trace_next_visible(&lookahead, start);
	}

	if (trace_event_hidden(ev, next_visible))
	{
	    cursor = current_idx;
	    continue;
	}

	render_start_seq = ev->seq;
	render_end_seq = ev->seq;

	{
	    size_t collapse_cursor = current_idx;

	    while (collapse_cursor > start)
	    {
		size_t		la_idx = collapse_cursor;
		trace_entry_T	*candidate;

		candidate = trace_next_visible(&la_idx, start);
		if (candidate == NULL || candidate == ev)
		    break;

		if (!trace_events_collapsible(ev, candidate))
		    break;

		++collapsed_repeat;
		render_end_seq = candidate->seq;
		collapse_cursor = la_idx;
		next_cursor = collapse_cursor;
	    }
	}

	if (collapsed_repeat > 1)
	    ev->repeat_count = collapsed_repeat;

	if (trace_is_insert_input(ev))
	{
	    if (!collecting_insert)
	    {
		insert_source = ev;
		collecting_insert_depth = ev->depth;
	    }

	    if (ev->message != NULL && *ev->message != NUL)
	    {
		int j;
		for (j = 0; j < ev->repeat_count; ++j)
		    ga_concat(&insert_text, ev->message);

		collecting_insert = TRUE;
		ev->repeat_count = 1;
		cursor = next_cursor;
		continue;
	    }
	}

	if (collecting_insert)
	{
	    for (int i = 0; i < collecting_insert_depth; ++i)
		ga_concat(&ga, (char_u *)"  ");
	}

	trace_flush_insert(&ga, &insert_text,
			   &collecting_insert, insert_source);

	if (last_mode[0] != NUL)
	{
	    int mode_diff = STRCMP(last_mode, ev->mode) != 0;
	    int phase_start = (ev->kind == TRACE_COMMAND
			       || ev->kind == TRACE_EX
			       || ev->kind == TRACE_MAPPING);

	    if (mode_diff || (phase_start && last_kind != ev->kind))
		ga_append(&ga, '\n');
	}

	last_kind = ev->kind;
	vim_strncpy(last_mode, ev->mode, MODE_MAX_LENGTH - 1);

	for (int i = 0; i < ev->depth; ++i)
	    ga_concat(&ga, (char_u *)"  ");

	trace_render_event(&ga, ev,
			   render_start_seq, render_end_seq);

	ev->repeat_count = 1;
	cursor = next_cursor;
    }

    if (collecting_insert)
    {
	for (int i = 0; i < collecting_insert_depth; ++i)
	    ga_concat(&ga, (char_u *)"  ");
    }

    trace_flush_insert(&ga, &insert_text,
		       &collecting_insert, insert_source);

    ga_clear(&insert_text);

    if (ga.ga_len > 0)
    {
	ga_append(&ga, NUL);
	return (char_u *)ga.ga_data;  // ownership transferred to caller
    }

    ga_clear(&ga);
    return NULL;
}

/*
 * Dump a range of events to the message area.
 */
    void
trace_dump_range(size_t start, size_t end)
{
    char_u *result;

    if (trace_dumping)
	return;

    trace_dumping = TRUE;
    result = trace_dump_to_string(start, end);
    trace_dumping = FALSE;

    if (result != NULL)
    {
	msg_puts((char *)result);
	vim_free(result);
    }
}


/*
 * "ch_traceget({count})" — return the N most recent rendered trace
 * events as a list of strings.
 */
    void
f_ch_traceget(typval_T *argvars, typval_T *rettv)
{
    long    count = 20;
    char_u *result;

    if (rettv_list_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
	count = tv_get_number(&argvars[0]);

    if (count <= 0)
	count = 20;

    if (count > (long)trace_ring_size)
	count = (long)trace_ring_size;

    trace_dumping = TRUE;
    result = trace_dump_to_string(0, (size_t)(count - 1));
    trace_dumping = FALSE;

    if (result != NULL)
    {
	// Split the result by newlines into list items
	char_u *p = result;
	char_u *start = result;

	while (*p != NUL)
	{
	    if (*p == '\n')
	    {
		*p = NUL;
		list_append_string(rettv->vval.v_list,
				   start, -1);
		*p = '\n';
		p++;
		start = p;
	    }
	    else
	    {
		p++;
	    }
	}
	// Add remaining text (without trailing newline)
	if (*start != NUL)
	    list_append_string(rettv->vval.v_list,
			       start, -1);

	vim_free(result);
    }
}

/*
 * "ch_traceclear()" — clear the trace ring buffer.
 */
    void
f_ch_traceclear(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    trace_clear_all();
}

#else
/*
 * Stubs: tracing is only available with FEAT_JOB_CHANNEL.
 */
    void
trace_ingest(char_u *buf UNUSED)
{
}

    int
trace_is_enabled(trace_event_kind_T kind UNUSED)
{
    return FALSE;
}

    int
trace_verbose(trace_verbosity_T level UNUSED)
{
    return FALSE;
}

    int
trace_is_active(void)
{
    return FALSE;
}

    void
trace_apply_opt(char_u *value UNUSED)
{
}

    char_u *
trace_format_input(int c UNUSED)
{
    return vim_strsave((char_u *)"");
}

    char_u *
trace_format_command(cmdarg_T *ca UNUSED)
{
    return vim_strsave((char_u *)"");
}

    char_u *
trace_format_ex(exarg_T *ea UNUSED)
{
    return vim_strsave((char_u *)"");
}

    char_u *
trace_format_mapping(mapblock_T *mp UNUSED)
{
    return vim_strsave((char_u *)"");
}

    char_u *
trace_format_typebuf(char_u *buf UNUSED, size_t buflen UNUSED)
{
    return vim_strsave((char_u *)"");
}

    void
trace_init(void)
{
}

    void
trace_clear_all(void)
{
}

    int
trace_resize_ring(size_t new_size UNUSED)
{
    return FAIL;
}

    trace_entry_T *
trace_get_recent(size_t idx UNUSED)
{
    return NULL;
}

    void
trace_dump_range(size_t start UNUSED, size_t end UNUSED)
{
}

    void
f_ch_traceget(typval_T *argvars UNUSED, typval_T *rettv)
{
    rettv_list_alloc(rettv);
}

    void
f_ch_traceclear(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
}
#endif
