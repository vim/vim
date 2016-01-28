/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * json.c: Encoding and decoding JSON.
 *
 * Follows this standard: https://tools.ietf.org/html/rfc7159.html
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)
static int json_encode_item(garray_T *gap, typval_T *val, int copyID);
static void json_decode_item(js_read_T *reader, typval_T *res);

/*
 * Encode "val" into a JSON format string.
 */
    char_u *
json_encode(typval_T *val)
{
    garray_T ga;

    /* Store bytes in the growarray. */
    ga_init2(&ga, 1, 4000);
    json_encode_item(&ga, val, get_copyID());
    return ga.ga_data;
}

    static void
write_string(garray_T *gap, char_u *str)
{
    char_u	*res = str;
    char_u	numbuf[NUMBUFLEN];

    if (res == NULL)
	ga_concat(gap, (char_u *)"null");
    else
    {
	ga_append(gap, '"');
	while (*res != NUL)
	{
	    int c = PTR2CHAR(res);

	    switch (c)
	    {
		case 0x08:
		    ga_append(gap, '\\'); ga_append(gap, 'b'); break;
		case 0x09:
		    ga_append(gap, '\\'); ga_append(gap, 't'); break;
		case 0x0a:
		    ga_append(gap, '\\'); ga_append(gap, 'n'); break;
		case 0x0c:
		    ga_append(gap, '\\'); ga_append(gap, 'f'); break;
		case 0x0d:
		    ga_append(gap, '\\'); ga_append(gap, 'r'); break;
		case 0x22: /* " */
		case 0x5c: /* \ */
		    ga_append(gap, '\\');
		    ga_append(gap, c);
		    break;
		default:
		    if (c >= 0x20)
		    {
			numbuf[mb_char2bytes(c, numbuf)] = NUL;
			ga_concat(gap, numbuf);
		    }
		    else
		    {
			vim_snprintf((char *)numbuf, NUMBUFLEN,
							 "\\u%04lx", (long)c);
			ga_concat(gap, numbuf);
		    }
	    }
	    mb_cptr_adv(res);
	}
	ga_append(gap, '"');
    }
}

/*
 * Encode "val" into "gap".
 * Return FAIL or OK.
 */
    static int
json_encode_item(garray_T *gap, typval_T *val, int copyID)
{
    char_u	numbuf[NUMBUFLEN];
    char_u	*res;
    list_T	*l;
    dict_T	*d;

    switch (val->v_type)
    {
	case VAR_SPECIAL:
	    switch (val->vval.v_number)
	    {
		case VVAL_FALSE: ga_concat(gap, (char_u *)"false"); break;
		case VVAL_TRUE: ga_concat(gap, (char_u *)"true"); break;
		case VVAL_NONE: break;
		case VVAL_NULL: ga_concat(gap, (char_u *)"null"); break;
	    }
	    break;

	case VAR_NUMBER:
	    vim_snprintf((char *)numbuf, NUMBUFLEN, "%ld",
						    (long)val->vval.v_number);
	    ga_concat(gap, numbuf);
	    break;

	case VAR_STRING:
	    res = val->vval.v_string;
	    write_string(gap, res);
	    break;

	case VAR_FUNC:
	    /* no JSON equivalent */
	    EMSG(_(e_invarg));
	    return FAIL;

	case VAR_LIST:
	    l = val->vval.v_list;
	    if (l == NULL)
		ga_concat(gap, (char_u *)"null");
	    else
	    {
		if (l->lv_copyID == copyID)
		    ga_concat(gap, (char_u *)"[]");
		else
		{
		    listitem_T	*li;

		    l->lv_copyID = copyID;
		    ga_append(gap, '[');
		    for (li = l->lv_first; li != NULL && !got_int; )
		    {
			if (json_encode_item(gap, &li->li_tv, copyID) == FAIL)
			    return FAIL;
			li = li->li_next;
			if (li != NULL)
			    ga_append(gap, ',');
		    }
		    ga_append(gap, ']');
		    l->lv_copyID = 0;
		}
	    }
	    break;

	case VAR_DICT:
	    d = val->vval.v_dict;
	    if (d == NULL)
		ga_concat(gap, (char_u *)"null");
	    else
	    {
		if (d->dv_copyID == copyID)
		    ga_concat(gap, (char_u *)"{}");
		else
		{
		    int		first = TRUE;
		    int		todo = (int)d->dv_hashtab.ht_used;
		    hashitem_T	*hi;

		    d->dv_copyID = copyID;
		    ga_append(gap, '{');

		    for (hi = d->dv_hashtab.ht_array; todo > 0 && !got_int;
									 ++hi)
			if (!HASHITEM_EMPTY(hi))
			{
			    --todo;
			    if (first)
				first = FALSE;
			    else
				ga_append(gap, ',');
			    write_string(gap, hi->hi_key);
			    ga_append(gap, ':');
			    if (json_encode_item(gap, &dict_lookup(hi)->di_tv,
							      copyID) == FAIL)
				return FAIL;
			}
		    ga_append(gap, '}');
		    d->dv_copyID = 0;
		}
	    }
	    break;

#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    vim_snprintf((char *)numbuf, NUMBUFLEN, "%g", val->vval.v_float);
	    ga_concat(gap, numbuf);
	    break;
#endif
	default: EMSG2(_(e_intern2), "json_encode_item()"); break;
		 return FAIL;
    }
    return OK;
}

/*
 * Skip white space in "reader".
 */
    static void
json_skip_white(js_read_T *reader)
{
    int c;

    while ((c = reader->js_buf[reader->js_used]) == ' '
					   || c == TAB || c == NL || c == CAR)
	++reader->js_used;
}

/*
 * Make sure there are at least enough characters buffered to read a number.
 */
    static void
json_fill_buffer(js_read_T *reader UNUSED)
{
    /* TODO */
}

    static void
json_decode_array(js_read_T *reader, typval_T *res)
{
    char_u	*p;
    typval_T	item;
    listitem_T	*li;

    if (rettv_list_alloc(res) == FAIL)
	goto failsilent;
    ++reader->js_used; /* consume the '[' */

    while (TRUE)
    {
	json_skip_white(reader);
	p = reader->js_buf + reader->js_used;
	if (*p == NUL)
	    goto fail;
	if (*p == ']')
	{
	    ++reader->js_used; /* consume the ']' */
	    return;
	}

	if (!reader->js_eof && (int)(reader->js_end - p) < NUMBUFLEN)
	    json_fill_buffer(reader);

	json_decode_item(reader, &item);
	li = listitem_alloc();
	if (li == NULL)
	    return;
	li->li_tv = item;
	list_append(res->vval.v_list, li);

	json_skip_white(reader);
	p = reader->js_buf + reader->js_used;
	if (*p == ',')
	    ++reader->js_used;
	else if (*p != ']')
	    goto fail;
    }
fail:
    EMSG(_(e_invarg));
failsilent:
    res->v_type = VAR_SPECIAL;
    res->vval.v_number = VVAL_NONE;
}

    static void
json_decode_object(js_read_T *reader, typval_T *res)
{
    char_u	*p;
    typval_T	tvkey;
    typval_T	item;
    dictitem_T	*di;
    char_u	buf[NUMBUFLEN];
    char_u	*key;

    if (rettv_dict_alloc(res) == FAIL)
	goto failsilent;
    ++reader->js_used; /* consume the '{' */

    while (TRUE)
    {
	json_skip_white(reader);
	p = reader->js_buf + reader->js_used;
	if (*p == NUL)
	    goto fail;
	if (*p == '}')
	{
	    ++reader->js_used; /* consume the '}' */
	    return;
	}

	if (!reader->js_eof && (int)(reader->js_end - p) < NUMBUFLEN)
	    json_fill_buffer(reader);
	json_decode_item(reader, &tvkey);
	key = get_tv_string_buf_chk(&tvkey, buf);
	if (key == NULL || *key == NUL)
	{
	    /* "key" is NULL when get_tv_string_buf_chk() gave an errmsg */
	    if (key != NULL)
		EMSG(_(e_emptykey));
	    clear_tv(&tvkey);
	    goto failsilent;
	}

	json_skip_white(reader);
	p = reader->js_buf + reader->js_used;
	if (*p != ':')
	{
	    clear_tv(&tvkey);
	    goto fail;
	}
	++reader->js_used;
	json_skip_white(reader);

	if (!reader->js_eof && (int)(reader->js_end - p) < NUMBUFLEN)
	    json_fill_buffer(reader);
	json_decode_item(reader, &item);

	di = dictitem_alloc(key);
	clear_tv(&tvkey);
	if (di == NULL)
	{
	    clear_tv(&item);
	    goto fail;
	}
	di->di_tv = item;
	if (dict_add(res->vval.v_dict, di) == FAIL)
	    dictitem_free(di);

	json_skip_white(reader);
	p = reader->js_buf + reader->js_used;
	if (*p == ',')
	    ++reader->js_used;
	else if (*p != '}')
	    goto fail;
    }
fail:
    EMSG(_(e_invarg));
failsilent:
    res->v_type = VAR_SPECIAL;
    res->vval.v_number = VVAL_NONE;
}

    static void
json_decode_string(js_read_T *reader, typval_T *res)
{
    garray_T    ga;
    int		len;
    char_u	*p = reader->js_buf + reader->js_used + 1;
    int		c;
    long	nr;
    char_u	buf[NUMBUFLEN];

    ga_init2(&ga, 1, 200);

    /* TODO: fill buffer when needed. */
    while (*p != NUL && *p != '"')
    {
	if (*p == '\\')
	{
	    c = -1;
	    switch (p[1])
	    {
		case 'b': c = BS; break;
		case 't': c = TAB; break;
		case 'n': c = NL; break;
		case 'f': c = FF; break;
		case 'r': c = CAR; break;
		case 'u':
		    vim_str2nr(p + 2, NULL, &len,
				     STR2NR_HEX + STR2NR_FORCE, &nr, NULL, 4);
		    p += len + 2;
#ifdef FEAT_MBYTE
		    buf[(*mb_char2bytes)((int)nr, buf)] = NUL;
		    ga_concat(&ga, buf);
#else
		    ga_append(&ga, nr);
#endif
		    break;
		default: c = p[1]; break;
	    }
	    if (c > 0)
	    {
		p += 2;
		ga_append(&ga, c);
	    }
	}
	else
	{
	    len = MB_PTR2LEN(p);
	    if (ga_grow(&ga, len) == OK)
	    {
		mch_memmove((char *)ga.ga_data + ga.ga_len, p, (size_t)len);
		ga.ga_len += len;
	    }
	    p += len;
	}
	if (!reader->js_eof && (int)(reader->js_end - p) < NUMBUFLEN)
	{
	    reader->js_used = (int)(p - reader->js_buf);
	    json_fill_buffer(reader);
	    p = reader->js_buf + reader->js_used;
	}
    }
    reader->js_used = (int)(p - reader->js_buf);
    if (*p == '"')
    {
	++reader->js_used;
	res->v_type = VAR_STRING;
	if (ga.ga_data == NULL)
	    res->vval.v_string = NULL;
	else
	    res->vval.v_string = vim_strsave(ga.ga_data);
    }
    else
    {
	EMSG(_(e_invarg));
	res->v_type = VAR_SPECIAL;
	res->vval.v_number = VVAL_NONE;
    }
    ga_clear(&ga);
}

/*
 * Decode one item and put it in "result".
 * Must already have skipped white space.
 */
    static void
json_decode_item(js_read_T *reader, typval_T *res)
{
    char_u	*p = reader->js_buf + reader->js_used;

    switch (*p)
    {
	case '[': /* array */
	    json_decode_array(reader, res);
	    return;

	case '{': /* object */
	    json_decode_object(reader, res);
	    return;

	case '"': /* string */
	    json_decode_string(reader, res);
	    return;

	case ',': /* comma: empty item */
	case NUL: /* empty */
	    res->v_type = VAR_SPECIAL;
	    res->vval.v_number = VVAL_NONE;
	    return;

	default:
	    if (VIM_ISDIGIT(*p) || *p == '-')
	    {
		int	len;
		char_u  *sp = p;
#ifdef FEAT_FLOAT
		if (*sp == '-')
		    ++sp;
		sp = skipdigits(sp);
		if (*sp == '.' || *sp == 'e' || *sp == 'E')
		{
		    res->v_type = VAR_FLOAT;
		    len = string2float(p, &res->vval.v_float);
		}
		else
#endif
		{
		    long nr;

		    res->v_type = VAR_NUMBER;
		    vim_str2nr(reader->js_buf + reader->js_used,
			    NULL, &len, 0, /* what */
			    &nr, NULL, 0);
		    res->vval.v_number = nr;
		}
		reader->js_used += len;
		return;
	    }
	    if (STRNICMP((char *)p, "false", 5) == 0)
	    {
		reader->js_used += 5;
		res->v_type = VAR_SPECIAL;
		res->vval.v_number = VVAL_FALSE;
		return;
	    }
	    if (STRNICMP((char *)p, "true", 4) == 0)
	    {
		reader->js_used += 4;
		res->v_type = VAR_SPECIAL;
		res->vval.v_number = VVAL_TRUE;
		return;
	    }
	    if (STRNICMP((char *)p, "null", 4) == 0)
	    {
		reader->js_used += 4;
		res->v_type = VAR_SPECIAL;
		res->vval.v_number = VVAL_NULL;
		return;
	    }
	    break;
    }

    EMSG(_(e_invarg));
    res->v_type = VAR_SPECIAL;
    res->vval.v_number = VVAL_NONE;
}

/*
 * Decode the JSON from "reader" and store the result in "res".
 */
    void
json_decode(js_read_T *reader, typval_T *res)
{
    json_skip_white(reader);
    json_decode_item(reader, res);
    json_skip_white(reader);
    if (reader->js_buf[reader->js_used] != NUL)
	EMSG(_(e_invarg));
}
#endif
