/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Lua interface by Luis Carvalho
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "vim.h"

/* Only do the following when the feature is enabled.  Needed for "make
 * depend". */
#if defined(FEAT_LUA) || defined(PROTO)

#define LUAVIM_CHUNKNAME "vim chunk"
#define LUAVIM_NAME "vim"

typedef buf_T *luaV_Buffer;
typedef win_T *luaV_Window;
typedef void (*msgfunc_T)(char_u *);

static const char LUAVIM_BUFFER[] = "buffer";
static const char LUAVIM_WINDOW[] = "window";
static const char LUAVIM_FREE[] = "luaV_free";

#define luaV_getfield(L, s) \
    lua_pushlightuserdata((L), (void *)(s)); \
    lua_rawget((L), LUA_REGISTRYINDEX)
#define luaV_checksandbox(L) \
    if (sandbox) luaL_error((L), "not allowed in sandbox")
#define luaV_msg(L) luaV_msgfunc((L), (msgfunc_T) msg)
#define luaV_emsg(L) luaV_msgfunc((L), (msgfunc_T) emsg)


#ifdef DYNAMIC_LUA

#ifndef WIN3264
# include <dlfcn.h>
# define HANDLE void*
# define load_dll(n) dlopen((n), RTLD_LAZY|RTLD_GLOBAL)
# define symbol_from_dll dlsym
# define close_dll dlclose
#else
# define load_dll LoadLibrary
# define symbol_from_dll GetProcAddress
# define close_dll FreeLibrary
#endif

/* lauxlib */
#define luaL_register dll_luaL_register
#define luaL_typerror dll_luaL_typerror
#define luaL_checklstring dll_luaL_checklstring
#define luaL_checkinteger dll_luaL_checkinteger
#define luaL_optinteger dll_luaL_optinteger
#define luaL_checktype dll_luaL_checktype
#define luaL_error dll_luaL_error
#define luaL_loadfile dll_luaL_loadfile
#define luaL_loadbuffer dll_luaL_loadbuffer
#define luaL_newstate dll_luaL_newstate
#define luaL_buffinit dll_luaL_buffinit
#define luaL_prepbuffer dll_luaL_prepbuffer
#define luaL_addlstring dll_luaL_addlstring
#define luaL_pushresult dll_luaL_pushresult
/* lua */
#define lua_close dll_lua_close
#define lua_gettop dll_lua_gettop
#define lua_settop dll_lua_settop
#define lua_pushvalue dll_lua_pushvalue
#define lua_replace dll_lua_replace
#define lua_isnumber dll_lua_isnumber
#define lua_isstring dll_lua_isstring
#define lua_type dll_lua_type
#define lua_rawequal dll_lua_rawequal
#define lua_tonumber dll_lua_tonumber
#define lua_tointeger dll_lua_tointeger
#define lua_toboolean dll_lua_toboolean
#define lua_tolstring dll_lua_tolstring
#define lua_touserdata dll_lua_touserdata
#define lua_pushnil dll_lua_pushnil
#define lua_pushnumber dll_lua_pushnumber
#define lua_pushinteger dll_lua_pushinteger
#define lua_pushlstring dll_lua_pushlstring
#define lua_pushstring dll_lua_pushstring
#define lua_pushfstring dll_lua_pushfstring
#define lua_pushcclosure dll_lua_pushcclosure
#define lua_pushboolean dll_lua_pushboolean
#define lua_pushlightuserdata dll_lua_pushlightuserdata
#define lua_getfield dll_lua_getfield
#define lua_rawget dll_lua_rawget
#define lua_createtable dll_lua_createtable
#define lua_newuserdata dll_lua_newuserdata
#define lua_getmetatable dll_lua_getmetatable
#define lua_setfield dll_lua_setfield
#define lua_rawset dll_lua_rawset
#define lua_rawseti dll_lua_rawseti
#define lua_setmetatable dll_lua_setmetatable
#define lua_call dll_lua_call
#define lua_pcall dll_lua_pcall
/* libs */
#define luaopen_base dll_luaopen_base
#define luaopen_table dll_luaopen_table
#define luaopen_string dll_luaopen_string
#define luaopen_math dll_luaopen_math
#define luaopen_io dll_luaopen_io
#define luaopen_os dll_luaopen_os
#define luaopen_package dll_luaopen_package
#define luaopen_debug dll_luaopen_debug
#define luaL_openlibs dll_luaL_openlibs

/* lauxlib */
void (*dll_luaL_register) (lua_State *L, const char *libname, const luaL_Reg *l);
int (*dll_luaL_typerror) (lua_State *L, int narg, const char *tname);
const char *(*dll_luaL_checklstring) (lua_State *L, int numArg, size_t *l);
lua_Integer (*dll_luaL_checkinteger) (lua_State *L, int numArg);
lua_Integer (*dll_luaL_optinteger) (lua_State *L, int nArg, lua_Integer def);
void (*dll_luaL_checktype) (lua_State *L, int narg, int t);
int (*dll_luaL_error) (lua_State *L, const char *fmt, ...);
int (*dll_luaL_loadfile) (lua_State *L, const char *filename);
int (*dll_luaL_loadbuffer) (lua_State *L, const char *buff, size_t sz, const char *name);
lua_State *(*dll_luaL_newstate) (void);
void (*dll_luaL_buffinit) (lua_State *L, luaL_Buffer *B);
char *(*dll_luaL_prepbuffer) (luaL_Buffer *B);
void (*dll_luaL_addlstring) (luaL_Buffer *B, const char *s, size_t l);
void (*dll_luaL_pushresult) (luaL_Buffer *B);
/* lua */
void       (*dll_lua_close) (lua_State *L);
int (*dll_lua_gettop) (lua_State *L);
void (*dll_lua_settop) (lua_State *L, int idx);
void (*dll_lua_pushvalue) (lua_State *L, int idx);
void (*dll_lua_replace) (lua_State *L, int idx);
int (*dll_lua_isnumber) (lua_State *L, int idx);
int (*dll_lua_isstring) (lua_State *L, int idx);
int (*dll_lua_type) (lua_State *L, int idx);
int (*dll_lua_rawequal) (lua_State *L, int idx1, int idx2);
lua_Number (*dll_lua_tonumber) (lua_State *L, int idx);
lua_Integer (*dll_lua_tointeger) (lua_State *L, int idx);
int (*dll_lua_toboolean) (lua_State *L, int idx);
const char *(*dll_lua_tolstring) (lua_State *L, int idx, size_t *len);
void *(*dll_lua_touserdata) (lua_State *L, int idx);
void (*dll_lua_pushnil) (lua_State *L);
void (*dll_lua_pushnumber) (lua_State *L, lua_Number n);
void (*dll_lua_pushinteger) (lua_State *L, lua_Integer n);
void (*dll_lua_pushlstring) (lua_State *L, const char *s, size_t l);
void (*dll_lua_pushstring) (lua_State *L, const char *s);
const char *(*dll_lua_pushfstring) (lua_State *L, const char *fmt, ...);
void (*dll_lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
void (*dll_lua_pushboolean) (lua_State *L, int b);
void (*dll_lua_pushlightuserdata) (lua_State *L, void *p);
void (*dll_lua_getfield) (lua_State *L, int idx, const char *k);
void (*dll_lua_rawget) (lua_State *L, int idx);
void (*dll_lua_createtable) (lua_State *L, int narr, int nrec);
void *(*dll_lua_newuserdata) (lua_State *L, size_t sz);
int (*dll_lua_getmetatable) (lua_State *L, int objindex);
void (*dll_lua_setfield) (lua_State *L, int idx, const char *k);
void (*dll_lua_rawset) (lua_State *L, int idx);
void (*dll_lua_rawseti) (lua_State *L, int idx, int n);
int (*dll_lua_setmetatable) (lua_State *L, int objindex);
void (*dll_lua_call) (lua_State *L, int nargs, int nresults);
int (*dll_lua_pcall) (lua_State *L, int nargs, int nresults, int errfunc);
/* libs */
int (*dll_luaopen_base) (lua_State *L);
int (*dll_luaopen_table) (lua_State *L);
int (*dll_luaopen_string) (lua_State *L);
int (*dll_luaopen_math) (lua_State *L);
int (*dll_luaopen_io) (lua_State *L);
int (*dll_luaopen_os) (lua_State *L);
int (*dll_luaopen_package) (lua_State *L);
int (*dll_luaopen_debug) (lua_State *L);
void (*dll_luaL_openlibs) (lua_State *L);

typedef void **luaV_function;
typedef struct {
    const char *name;
    luaV_function func;
} luaV_Reg;

static const luaV_Reg luaV_dll[] = {
    /* lauxlib */
    {"luaL_register", (luaV_function) &dll_luaL_register},
    {"luaL_typerror", (luaV_function) &dll_luaL_typerror},
    {"luaL_checklstring", (luaV_function) &dll_luaL_checklstring},
    {"luaL_checkinteger", (luaV_function) &dll_luaL_checkinteger},
    {"luaL_optinteger", (luaV_function) &dll_luaL_optinteger},
    {"luaL_checktype", (luaV_function) &dll_luaL_checktype},
    {"luaL_error", (luaV_function) &dll_luaL_error},
    {"luaL_loadfile", (luaV_function) &dll_luaL_loadfile},
    {"luaL_loadbuffer", (luaV_function) &dll_luaL_loadbuffer},
    {"luaL_newstate", (luaV_function) &dll_luaL_newstate},
    {"luaL_buffinit", (luaV_function) &dll_luaL_buffinit},
    {"luaL_prepbuffer", (luaV_function) &dll_luaL_prepbuffer},
    {"luaL_addlstring", (luaV_function) &dll_luaL_addlstring},
    {"luaL_pushresult", (luaV_function) &dll_luaL_pushresult},
    /* lua */
    {"lua_close", (luaV_function) &dll_lua_close},
    {"lua_gettop", (luaV_function) &dll_lua_gettop},
    {"lua_settop", (luaV_function) &dll_lua_settop},
    {"lua_pushvalue", (luaV_function) &dll_lua_pushvalue},
    {"lua_replace", (luaV_function) &dll_lua_replace},
    {"lua_isnumber", (luaV_function) &dll_lua_isnumber},
    {"lua_isstring", (luaV_function) &dll_lua_isstring},
    {"lua_type", (luaV_function) &dll_lua_type},
    {"lua_rawequal", (luaV_function) &dll_lua_rawequal},
    {"lua_tonumber", (luaV_function) &dll_lua_tonumber},
    {"lua_tointeger", (luaV_function) &dll_lua_tointeger},
    {"lua_toboolean", (luaV_function) &dll_lua_toboolean},
    {"lua_tolstring", (luaV_function) &dll_lua_tolstring},
    {"lua_touserdata", (luaV_function) &dll_lua_touserdata},
    {"lua_pushnil", (luaV_function) &dll_lua_pushnil},
    {"lua_pushnumber", (luaV_function) &dll_lua_pushnumber},
    {"lua_pushinteger", (luaV_function) &dll_lua_pushinteger},
    {"lua_pushlstring", (luaV_function) &dll_lua_pushlstring},
    {"lua_pushstring", (luaV_function) &dll_lua_pushstring},
    {"lua_pushfstring", (luaV_function) &dll_lua_pushfstring},
    {"lua_pushcclosure", (luaV_function) &dll_lua_pushcclosure},
    {"lua_pushboolean", (luaV_function) &dll_lua_pushboolean},
    {"lua_pushlightuserdata", (luaV_function) &dll_lua_pushlightuserdata},
    {"lua_getfield", (luaV_function) &dll_lua_getfield},
    {"lua_rawget", (luaV_function) &dll_lua_rawget},
    {"lua_createtable", (luaV_function) &dll_lua_createtable},
    {"lua_newuserdata", (luaV_function) &dll_lua_newuserdata},
    {"lua_getmetatable", (luaV_function) &dll_lua_getmetatable},
    {"lua_setfield", (luaV_function) &dll_lua_setfield},
    {"lua_rawset", (luaV_function) &dll_lua_rawset},
    {"lua_rawseti", (luaV_function) &dll_lua_rawseti},
    {"lua_setmetatable", (luaV_function) &dll_lua_setmetatable},
    {"lua_call", (luaV_function) &dll_lua_call},
    {"lua_pcall", (luaV_function) &dll_lua_pcall},
    /* libs */
    {"luaopen_base", (luaV_function) &dll_luaopen_base},
    {"luaopen_table", (luaV_function) &dll_luaopen_table},
    {"luaopen_string", (luaV_function) &dll_luaopen_string},
    {"luaopen_math", (luaV_function) &dll_luaopen_math},
    {"luaopen_io", (luaV_function) &dll_luaopen_io},
    {"luaopen_os", (luaV_function) &dll_luaopen_os},
    {"luaopen_package", (luaV_function) &dll_luaopen_package},
    {"luaopen_debug", (luaV_function) &dll_luaopen_debug},
    {"luaL_openlibs", (luaV_function) &dll_luaL_openlibs},
    {NULL, NULL}
};

static HANDLE hinstLua = NULL;

    static void
end_dynamic_lua(void)
{
    if (hinstLua)
    {
	close_dll(hinstLua);
	hinstLua = 0;
    }
}

    static int
lua_link_init(char *libname, int verbose)
{
    const luaV_Reg *reg;
    if (hinstLua) return OK;
    hinstLua = load_dll(libname);
    if (!hinstLua)
    {
	if (verbose)
	    EMSG2(_(e_loadlib), libname);
	return FAIL;
    }
    for (reg = luaV_dll; reg->func; reg++)
    {
	if ((*reg->func = symbol_from_dll(hinstLua, reg->name)) == NULL)
	{
	    close_dll(hinstLua);
	    hinstLua = 0;
	    if (verbose)
		EMSG2(_(e_loadfunc), reg->name);
	    return FAIL;
	}
    }
    return OK;
}

    int
lua_enabled(int verbose)
{
    return lua_link_init(DYNAMIC_LUA_DLL, verbose) == OK;
}

#endif /* DYNAMIC_LUA */


/* =======   Internal   ======= */

    static void
luaV_newmetatable(lua_State *L, const char *tname)
{
    lua_newtable(L);
    lua_pushlightuserdata(L, (void *) tname);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

    static void *
luaV_toudata(lua_State *L, int ud, const char *tname)
{
    void *p = lua_touserdata(L, ud);

    if (p != NULL) /* value is userdata? */
    {
	if (lua_getmetatable(L, ud)) /* does it have a metatable? */
	{
	    luaV_getfield(L, tname); /* get metatable */
	    if (lua_rawequal(L, -1, -2)) /* MTs match? */
	    {
		lua_pop(L, 2); /* MTs */
		return p;
	    }
	}
    }
    return NULL;
}

    static void *
luaV_checkudata(lua_State *L, int ud, const char *tname)
{
    void *p = luaV_toudata(L, ud, tname);
    if (p == NULL) luaL_typerror(L, ud, tname);
    return p;
}

    static void
luaV_pushtypval(lua_State *L, typval_T *tv)
{
    if (tv == NULL) luaL_error(L, "null type");
    switch (tv->v_type)
    {
	case VAR_STRING:
	    lua_pushstring(L, (char *) tv->vval.v_string);
	    break;
	case VAR_NUMBER:
	    lua_pushinteger(L, (int) tv->vval.v_number);
	    break;
#ifdef FEAT_FLOAT
	case VAR_FLOAT:
	    lua_pushnumber(L, (lua_Number) tv->vval.v_float);
	    break;
#endif
	case VAR_LIST: {
	    list_T *l = tv->vval.v_list;

	    if (l != NULL)
	    {
		/* check cache */
		lua_pushlightuserdata(L, (void *) l);
		lua_rawget(L, LUA_ENVIRONINDEX);
		if (lua_isnil(L, -1)) /* not interned? */
		{
		    listitem_T *li;
		    int n = 0;
		    lua_pop(L, 1); /* nil */
		    lua_newtable(L);
		    lua_pushlightuserdata(L, (void *) l);
		    lua_pushvalue(L, -2);
		    lua_rawset(L, LUA_ENVIRONINDEX);
		    for (li = l->lv_first; li != NULL; li = li->li_next)
		    {
			luaV_pushtypval(L, &li->li_tv);
			lua_rawseti(L, -2, ++n);
		    }
		}
	    }
	    else lua_pushnil(L);
	    break;
		       }
	case VAR_DICT: {
	    dict_T *d = tv->vval.v_dict;

	    if (d != NULL)
	    {
		/* check cache */
		lua_pushlightuserdata(L, (void *) d);
		lua_rawget(L, LUA_ENVIRONINDEX);
		if (lua_isnil(L, -1)) /* not interned? */
		{
		    hashtab_T *ht = &d->dv_hashtab;
		    hashitem_T *hi;
		    int n = ht->ht_used; /* remaining items */
		    lua_pop(L, 1); /* nil */
		    lua_newtable(L);
		    lua_pushlightuserdata(L, (void *) d);
		    lua_pushvalue(L, -2);
		    lua_rawset(L, LUA_ENVIRONINDEX);
		    for (hi = ht->ht_array; n > 0; hi++)
		    {
			if (!HASHITEM_EMPTY(hi))
			{
			    dictitem_T *di = dict_lookup(hi);
			    luaV_pushtypval(L, &di->di_tv);
			    lua_setfield(L, -2, (char *) hi->hi_key);
			    n--;
			}
		    }
		}
	    }
	    else lua_pushnil(L);
	    break;
	}
	default:
	    luaL_error(L, "invalid type");
    }
}

/* similar to luaL_addlstring, but replaces \0 with \n if toline and
 * \n with \0 otherwise */
    static void
luaV_addlstring(luaL_Buffer *b, const char *s, size_t l, int toline)
{
    while (l--)
    {
	if (*s == '\0' && toline)
	    luaL_addchar(b, '\n');
	else if (*s == '\n' && !toline)
	    luaL_addchar(b, '\0');
	else
	    luaL_addchar(b, *s);
	s++;
    }
}

    static void
luaV_pushline(lua_State *L, buf_T *buf, linenr_T n)
{
    const char *s = (const char *) ml_get_buf(buf, n, FALSE);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaV_addlstring(&b, s, strlen(s), 0);
    luaL_pushresult(&b);
}

    static char_u *
luaV_toline(lua_State *L, int pos)
{
    size_t l;
    const char *s = lua_tolstring(L, pos, &l);

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaV_addlstring(&b, s, l, 1);
    luaL_pushresult(&b);
    return (char_u *) lua_tostring(L, -1);
}

/* pops a string s from the top of the stack and calls mf(t) for pieces t of
 * s separated by newlines */
    static void
luaV_msgfunc(lua_State *L, msgfunc_T mf)
{
    luaL_Buffer b;
    size_t l;
    const char *p, *s = lua_tolstring(L, -1, &l);
    luaL_buffinit(L, &b);
    luaV_addlstring(&b, s, l, 0);
    luaL_pushresult(&b);
    /* break string */
    p = s = lua_tolstring(L, -1, &l);
    while (l--)
    {
	if (*p++ == '\0') /* break? */
	{
	    mf((char_u *) s);
	    s = p;
	}
    }
    mf((char_u *) s);
    lua_pop(L, 2); /* original and modified strings */
}


/* =======   Buffer type   ======= */

    static luaV_Buffer *
luaV_newbuffer(lua_State *L, buf_T *buf)
{
    luaV_Buffer *b = (luaV_Buffer *) lua_newuserdata(L, sizeof(luaV_Buffer));
    *b = buf;
    lua_pushlightuserdata(L, (void *) buf);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_ENVIRONINDEX); /* env[buf] = udata */
    /* to avoid GC, store as key in env */
    lua_pushvalue(L, -1);
    lua_pushboolean(L, 1);
    lua_rawset(L, LUA_ENVIRONINDEX); /* env[udata] = true */
    /* set metatable */
    luaV_getfield(L, LUAVIM_BUFFER);
    lua_setmetatable(L, -2);
    return b;
}

    static luaV_Buffer *
luaV_pushbuffer (lua_State *L, buf_T *buf)
{
    luaV_Buffer *b = NULL;
    if (buf == NULL)
	lua_pushnil(L);
    else {
	lua_pushlightuserdata(L, (void *) buf);
	lua_rawget(L, LUA_ENVIRONINDEX);
	if (lua_isnil(L, -1)) /* not interned? */
	{
	    lua_pop(L, 1);
	    b = luaV_newbuffer(L, buf);
	}
	else
	    b = (luaV_Buffer *) lua_touserdata(L, -1);
    }
    return b;
}

/* Buffer metamethods */

    static int
luaV_buffer_tostring(lua_State *L)
{
    lua_pushfstring(L, "%s: %p", LUAVIM_BUFFER, lua_touserdata(L, 1));
    return 1;
}

    static int
luaV_buffer_len(lua_State *L)
{
    luaV_Buffer *b = lua_touserdata(L, 1);
    lua_pushinteger(L, (*b)->b_ml.ml_line_count);
    return 1;
}

    static int
luaV_buffer_call(lua_State *L)
{
    luaV_Buffer *b = (luaV_Buffer *) lua_touserdata(L, 1);
    lua_settop(L, 1);
    set_curbuf(*b, DOBUF_SPLIT);
    return 1;
}

    static int
luaV_buffer_index(lua_State *L)
{
    luaV_Buffer *b = (luaV_Buffer *) lua_touserdata(L, 1);
    linenr_T n = (linenr_T) lua_tointeger(L, 2);
    if (n > 0 && n <= (*b)->b_ml.ml_line_count)
	luaV_pushline(L, *b, n);
    else if (lua_isstring(L, 2))
    {
	const char *s = lua_tostring(L, 2);
	if (strncmp(s, "name", 4) == 0)
	    lua_pushstring(L, (char *) (*b)->b_sfname);
	else if (strncmp(s, "fname", 5) == 0)
	    lua_pushstring(L, (char *) (*b)->b_ffname);
	else if (strncmp(s, "number", 6) == 0)
	    lua_pushinteger(L, (*b)->b_fnum);
	/* methods */
	else if (strncmp(s,   "insert", 6) == 0
		|| strncmp(s, "next", 4) == 0
		|| strncmp(s, "previous", 8) == 0
		|| strncmp(s, "isvalid", 7) == 0)
	{
	    lua_getmetatable(L, 1);
	    lua_getfield(L, -1, s);
	}
	else
	    lua_pushnil(L);
    }
    else
	lua_pushnil(L);
    return 1;
}

    static int
luaV_buffer_newindex(lua_State *L)
{
    luaV_Buffer *b = (luaV_Buffer *) lua_touserdata(L, 1);
    linenr_T n = (linenr_T) luaL_checkinteger(L, 2);
#ifdef HAVE_SANDBOX
    luaV_checksandbox(L);
#endif
    if (n < 1 || n > (*b)->b_ml.ml_line_count)
	luaL_error(L, "invalid line number");
    if (lua_isnil(L, 3)) /* delete line */
    {
	buf_T *buf = curbuf;
	curbuf = *b;
	if (u_savedel(n, 1L) == FAIL)
	{
	    curbuf = buf;
	    luaL_error(L, "cannot save undo information");
	}
	else if (ml_delete(n, FALSE) == FAIL)
	{
	    curbuf = buf;
	    luaL_error(L, "cannot delete line");
	}
	else {
	    deleted_lines_mark(n, 1L);
	    if (*b == curwin->w_buffer) /* fix cursor in current window? */
	    {
		if (curwin->w_cursor.lnum >= n)
		{
		    if (curwin->w_cursor.lnum > n)
		    {
			curwin->w_cursor.lnum -= 1;
			check_cursor_col();
		    }
		    else check_cursor();
		    changed_cline_bef_curs();
		}
		invalidate_botline();
	    }
	}
	curbuf = buf;
    }
    else if (lua_isstring(L, 3)) /* update line */
    {
	buf_T *buf = curbuf;
	curbuf = *b;
	if (u_savesub(n) == FAIL)
	{
	    curbuf = buf;
	    luaL_error(L, "cannot save undo information");
	}
	else if (ml_replace(n, luaV_toline(L, 3), TRUE) == FAIL)
	{
	    curbuf = buf;
	    luaL_error(L, "cannot replace line");
	}
	else changed_bytes(n, 0);
	curbuf = buf;
	if (*b == curwin->w_buffer)
	    check_cursor_col();
    }
    else
	luaL_error(L, "wrong argument to change line");
    return 0;
}

    static int
luaV_buffer_insert(lua_State *L)
{
    luaV_Buffer *b = luaV_checkudata(L, 1, LUAVIM_BUFFER);
    linenr_T last = (*b)->b_ml.ml_line_count;
    linenr_T n = (linenr_T) luaL_optinteger(L, 3, last);
    buf_T *buf;
    luaL_checktype(L, 2, LUA_TSTRING);
#ifdef HAVE_SANDBOX
    luaV_checksandbox(L);
#endif
    /* fix insertion line */
    if (n < 0) n = 0;
    if (n > last) n = last;
    /* insert */
    buf = curbuf;
    curbuf = *b;
    if (u_save(n, n + 1) == FAIL)
    {
	curbuf = buf;
	luaL_error(L, "cannot save undo information");
    }
    else if (ml_append(n, luaV_toline(L, 2), 0, FALSE) == FAIL)
    {
	curbuf = buf;
	luaL_error(L, "cannot insert line");
    }
    else
	appended_lines_mark(n, 1L);
    curbuf = buf;
    update_screen(VALID);
    return 0;
}

    static int
luaV_buffer_next(lua_State *L)
{
    luaV_Buffer *b = luaV_checkudata(L, 1, LUAVIM_BUFFER);
    luaV_pushbuffer(L, (*b)->b_next);
    return 1;
}

    static int
luaV_buffer_previous(lua_State *L)
{
    luaV_Buffer *b = luaV_checkudata(L, 1, LUAVIM_BUFFER);
    luaV_pushbuffer(L, (*b)->b_prev);
    return 1;
}

    static int
luaV_buffer_isvalid(lua_State *L)
{
    luaV_Buffer *b = luaV_checkudata(L, 1, LUAVIM_BUFFER);
    lua_pushlightuserdata(L, (void *) (*b));
    lua_rawget(L, LUA_ENVIRONINDEX);
    lua_pushboolean(L, !lua_isnil(L, -1));
    return 1;
}

static const luaL_Reg luaV_Buffer_mt[] = {
    {"__tostring", luaV_buffer_tostring},
    {"__len", luaV_buffer_len},
    {"__call", luaV_buffer_call},
    {"__index", luaV_buffer_index},
    {"__newindex", luaV_buffer_newindex},
    {"insert", luaV_buffer_insert},
    {"next", luaV_buffer_next},
    {"previous", luaV_buffer_previous},
    {"isvalid", luaV_buffer_isvalid},
    {NULL, NULL}
};


/* =======   Window type   ======= */

    static luaV_Window *
luaV_newwindow(lua_State *L, win_T *win)
{
    luaV_Window *w = (luaV_Window *) lua_newuserdata(L, sizeof(luaV_Window));
    *w = win;
    lua_pushlightuserdata(L, (void *) win);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_ENVIRONINDEX); /* env[win] = udata */
    /* to avoid GC, store as key in env */
    lua_pushvalue(L, -1);
    lua_pushboolean(L, 1);
    lua_rawset(L, LUA_ENVIRONINDEX); /* env[udata] = true */
    /* set metatable */
    luaV_getfield(L, LUAVIM_WINDOW);
    lua_setmetatable(L, -2);
    return w;
}

    static luaV_Window *
luaV_pushwindow(lua_State *L, win_T *win)
{
    luaV_Window *w = NULL;
    if (win == NULL)
	lua_pushnil(L);
    else {
	lua_pushlightuserdata(L, (void *) win);
	lua_rawget(L, LUA_ENVIRONINDEX);
	if (lua_isnil(L, -1)) /* not interned? */
	{
	    lua_pop(L, 1);
	    w = luaV_newwindow(L, win);
	}
	else w = (luaV_Window *) lua_touserdata(L, -1);
    }
    return w;
}

/* Window metamethods */

    static int
luaV_window_tostring(lua_State *L)
{
    lua_pushfstring(L, "%s: %p", LUAVIM_WINDOW, lua_touserdata(L, 1));
    return 1;
}

    static int
luaV_window_call(lua_State *L)
{
    luaV_Window *w = (luaV_Window *) lua_touserdata(L, 1);
    lua_settop(L, 1);
    win_goto(*w);
    return 1;
}

    static int
luaV_window_index(lua_State *L)
{
    luaV_Window *w = (luaV_Window *) lua_touserdata(L, 1);
    const char *s = luaL_checkstring(L, 2);
    if (strncmp(s, "buffer", 6) == 0)
	luaV_pushbuffer(L, (*w)->w_buffer);
    else if (strncmp(s, "line", 4) == 0)
	lua_pushinteger(L, (*w)->w_cursor.lnum);
    else if (strncmp(s, "col", 3) == 0)
	lua_pushinteger(L, (*w)->w_cursor.col + 1);
#ifdef FEAT_VERTSPLIT
    else if (strncmp(s, "width", 5) == 0)
	lua_pushinteger(L, W_WIDTH((*w)));
#endif
    else if (strncmp(s, "height", 6) == 0)
	lua_pushinteger(L, (*w)->w_height);
    /* methods */
    else if (strncmp(s,   "next", 4) == 0
	    || strncmp(s, "previous", 8) == 0
	    || strncmp(s, "isvalid", 7) == 0)
    {
	lua_getmetatable(L, 1);
	lua_getfield(L, -1, s);
    }
    else
	lua_pushnil(L);
    return 1;
}

    static int
luaV_window_newindex (lua_State *L)
{
    luaV_Window *w = (luaV_Window *) lua_touserdata(L, 1);
    const char *s = luaL_checkstring(L, 2);
    int v = luaL_checkinteger(L, 3);
    if (strncmp(s, "line", 4) == 0)
    {
#ifdef HAVE_SANDBOX
	luaV_checksandbox(L);
#endif
	if (v < 1 || v > (*w)->w_buffer->b_ml.ml_line_count)
	    luaL_error(L, "line out of range");
	(*w)->w_cursor.lnum = v;
	update_screen(VALID);
    }
    else if (strncmp(s, "col", 3) == 0)
    {
#ifdef HAVE_SANDBOX
	luaV_checksandbox(L);
#endif
	(*w)->w_cursor.col = v - 1;
	update_screen(VALID);
    }
#ifdef FEAT_VERTSPLIT
    else if (strncmp(s, "width", 5) == 0)
    {
	win_T *win = curwin;
#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	curwin = *w;
	win_setwidth(v);
	curwin = win;
    }
#endif
    else if (strncmp(s, "height", 6) == 0)
    {
	win_T *win = curwin;
#ifdef FEAT_GUI
	need_mouse_correct = TRUE;
#endif
	curwin = *w;
	win_setheight(v);
	curwin = win;
    }
    else
	luaL_error(L, "invalid window property: `%s'", s);
    return 0;
}

    static int
luaV_window_next(lua_State *L)
{
    luaV_Window *w = luaV_checkudata(L, 1, LUAVIM_WINDOW);
    luaV_pushwindow(L, (*w)->w_next);
    return 1;
}

    static int
luaV_window_previous(lua_State *L)
{
    luaV_Window *w = luaV_checkudata(L, 1, LUAVIM_WINDOW);
    luaV_pushwindow(L, (*w)->w_prev);
    return 1;
}

    static int
luaV_window_isvalid(lua_State *L)
{
    luaV_Window *w = luaV_checkudata(L, 1, LUAVIM_WINDOW);
    lua_pushlightuserdata(L, (void *) (*w));
    lua_rawget(L, LUA_ENVIRONINDEX);
    lua_pushboolean(L, !lua_isnil(L, -1));
    return 1;
}

static const luaL_Reg luaV_Window_mt[] = {
    {"__tostring", luaV_window_tostring},
    {"__call", luaV_window_call},
    {"__index", luaV_window_index},
    {"__newindex", luaV_window_newindex},
    {"next", luaV_window_next},
    {"previous", luaV_window_previous},
    {"isvalid", luaV_window_isvalid},
    {NULL, NULL}
};


/* =======   Vim module   ======= */

    static int
luaV_print(lua_State *L)
{
    int i, n = lua_gettop(L); /* nargs */
    const char *s;
    size_t l;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    lua_getglobal(L, "tostring");
    for (i = 1; i <= n; i++)
    {
	lua_pushvalue(L, -1); /* tostring */
	lua_pushvalue(L, i); /* arg */
	lua_call(L, 1, 1);
	s = lua_tolstring(L, -1, &l);
	if (s == NULL)
	    return luaL_error(L, "cannot convert to string");
	if (i > 1) luaL_addchar(&b, ' '); /* use space instead of tab */
	luaV_addlstring(&b, s, l, 0);
	lua_pop(L, 1);
    }
    luaL_pushresult(&b);
    luaV_msg(L);
    return 0;
}

    static int
luaV_command(lua_State *L)
{
    do_cmdline_cmd((char_u *) luaL_checkstring(L, 1));
    update_screen(VALID);
    return 0;
}

    static int
luaV_eval(lua_State *L)
{
    typval_T *tv = eval_expr((char_u *) luaL_checkstring(L, 1), NULL);
    if (tv == NULL) luaL_error(L, "invalid expression");
    luaV_pushtypval(L, tv);
    return 1;
}

    static int
luaV_beep(lua_State *L UNUSED)
{
    vim_beep();
    return 0;
}

    static int
luaV_line(lua_State *L)
{
    luaV_pushline(L, curbuf, curwin->w_cursor.lnum);
    return 1;
}

    static int
luaV_buffer(lua_State *L)
{
    buf_T *buf;
    if (lua_isstring(L, 1)) /* get by number or name? */
    {
	if (lua_isnumber(L, 1)) /* by number? */
	{
	    int n = lua_tointeger(L, 1);
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		if (buf->b_fnum == n) break;
	}
	else { /* by name */
	    size_t l;
	    const char *s = lua_tolstring(L, 1, &l);
	    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	    {
		if (buf->b_ffname == NULL || buf->b_sfname == NULL)
		{
		    if (l == 0) break;
		}
		else if (strncmp(s, (char *)buf->b_ffname, l) == 0
			|| strncmp(s, (char *)buf->b_sfname, l) == 0)
		    break;
	    }
	}
	if (buf == NULL) /* not found? */
	    lua_pushnil(L);
	else
	    luaV_pushbuffer(L, buf);
    }
    else {
	buf = (lua_toboolean(L, 1)) ? firstbuf : curbuf; /* first buffer? */
	luaV_pushbuffer(L, buf);
    }
    return 1;
}

    static int
luaV_window(lua_State *L)
{
    win_T *win;
    if (lua_isnumber(L, 1)) /* get by number? */
    {
	int n = lua_tointeger(L, 1);
	for (win = firstwin; win != NULL; win = win->w_next, n--)
	    if (n == 1) break;
	if (win == NULL) /* not found? */
	    lua_pushnil(L);
	else
	    luaV_pushwindow(L, win);
    }
    else {
	win = (lua_toboolean(L, 1)) ? firstwin : curwin; /* first window? */
	luaV_pushwindow(L, win);
    }
    return 1;
}

    static int
luaV_open(lua_State *L)
{
    luaV_Buffer *b;
    char_u *s = NULL;
#ifdef HAVE_SANDBOX
    luaV_checksandbox(L);
#endif
    if (lua_isstring(L, 1)) s = (char_u *) lua_tostring(L, 1);
    b = luaV_pushbuffer(L, buflist_new(s, NULL, 1L, BLN_LISTED));
    return 1;
}

    static int
luaV_isbuffer(lua_State *L)
{
    lua_pushboolean(L, luaV_toudata(L, 1, LUAVIM_BUFFER) != NULL);
    return 1;
}

    static int
luaV_iswindow(lua_State *L)
{
    lua_pushboolean(L, luaV_toudata(L, 1, LUAVIM_WINDOW) != NULL);
    return 1;
}

/* for freeing buffer and window objects; lightuserdata as arg */
    static int
luaV_free(lua_State *L)
{
    lua_pushvalue(L, 1); /* lightudata */
    lua_rawget(L, LUA_ENVIRONINDEX);
    if (!lua_isnil(L, -1))
    {
	lua_pushnil(L);
	lua_rawset(L, LUA_ENVIRONINDEX); /* env[udata] = nil */
	lua_pushnil(L);
	lua_rawset(L, LUA_ENVIRONINDEX); /* env[lightudata] = nil */
    }
    return 0;
}

static const luaL_Reg luaV_module[] = {
    {"command", luaV_command},
    {"eval", luaV_eval},
    {"beep", luaV_beep},
    {"line", luaV_line},
    {"buffer", luaV_buffer},
    {"window", luaV_window},
    {"open", luaV_open},
    {"isbuffer", luaV_isbuffer},
    {"iswindow", luaV_iswindow},
    {NULL, NULL}
};

    static int
luaopen_vim(lua_State *L)
{
    /* set environment */
    lua_newtable(L);
    lua_newtable(L);
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    lua_replace(L, LUA_ENVIRONINDEX);
    /* print */
    lua_pushcfunction(L, luaV_print);
    lua_setglobal(L, "print");
    /* free */
    lua_pushlightuserdata(L, (void *) LUAVIM_FREE);
    lua_pushcfunction(L, luaV_free);
    lua_rawset(L, LUA_REGISTRYINDEX);
    /* register */
    luaV_newmetatable(L, LUAVIM_BUFFER);
    luaL_register(L, NULL, luaV_Buffer_mt);
    luaV_newmetatable(L, LUAVIM_WINDOW);
    luaL_register(L, NULL, luaV_Window_mt);
    luaL_register(L, LUAVIM_NAME, luaV_module);
    return 0;
}

    static lua_State *
luaV_newstate(void)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L); /* core libs */
    lua_pushcfunction(L, luaopen_vim); /* vim */
    lua_call(L, 0, 0);
    return L;
}

    static void
luaV_setrange(lua_State *L, int line1, int line2)
{
    lua_getglobal(L, LUAVIM_NAME);
    lua_pushinteger(L, line1);
    lua_setfield(L, -2, "firstline");
    lua_pushinteger(L, line2);
    lua_setfield(L, -2, "lastline");
    lua_pop(L, 1); /* vim table */
}


/* =======   Interface   ======= */

static lua_State *L = NULL;

    static int
lua_is_open(void)
{
    return L != NULL;
}

    static int
lua_init(void)
{
    if (L == NULL)
    {
#ifdef DYNAMIC_LUA
	if (!lua_enabled(TRUE))
	{
	    EMSG(_("Lua library cannot be loaded."));
	    return FAIL;
	}
#endif
	L = luaV_newstate();
    }
    return OK;
}

    void
lua_end(void)
{
    if (L != NULL)
    {
	lua_close(L);
	L = NULL;
#ifdef DYNAMIC_LUA
	end_dynamic_lua();
#endif
    }
}

/* ex commands */
    void
ex_lua(exarg_T *eap)
{
    char *script;
    if (lua_init() == FAIL) return;
    script = (char *) script_get(eap, eap->arg);
    if (!eap->skip)
    {
	char *s = (script) ? script :  (char *) eap->arg;
	luaV_setrange(L, eap->line1, eap->line2);
	if (luaL_loadbuffer(L, s, strlen(s), LUAVIM_CHUNKNAME)
		|| lua_pcall(L, 0, 0, 0))
	    luaV_emsg(L);
    }
    if (script != NULL) vim_free(script);
}

    void
ex_luado(exarg_T *eap)
{
    linenr_T l;
    const char *s = (const char *) eap->arg;
    luaL_Buffer b;
    size_t len;
    if (lua_init() == FAIL) return;
    if (u_save(eap->line1 - 1, eap->line2 + 1) == FAIL)
    {
	EMSG(_("cannot save undo information"));
	return;
    }
    luaV_setrange(L, eap->line1, eap->line2);
    luaL_buffinit(L, &b);
    luaL_addlstring(&b, "return function(line) ", 22); /* header */
    luaL_addlstring(&b, s, strlen(s));
    luaL_addlstring(&b, " end", 4); /* footer */
    luaL_pushresult(&b);
    s = lua_tolstring(L, -1, &len);
    if (luaL_loadbuffer(L, s, len, LUAVIM_CHUNKNAME))
    {
	luaV_emsg(L);
	lua_pop(L, 1); /* function body */
	return;
    }
    lua_call(L, 0, 1);
    lua_replace(L, -2); /* function -> body */
    for (l = eap->line1; l <= eap->line2; l++)
    {
	lua_pushvalue(L, -1); /* function */
	luaV_pushline(L, curbuf, l); /* current line as arg */
	if (lua_pcall(L, 1, 1, 0))
	{
	    luaV_emsg(L);
	    break;
	}
	if (lua_isstring(L, -1)) /* update line? */
	{
#ifdef HAVE_SANDBOX
	    luaV_checksandbox(L);
#endif
	    ml_replace(l, luaV_toline(L, -1), TRUE);
	    changed_bytes(l, 0);
	    lua_pop(L, 1); /* result from luaV_toline */
	}
	lua_pop(L, 1); /* line */
    }
    lua_pop(L, 1); /* function */
    check_cursor();
    update_screen(NOT_VALID);
}

    void
ex_luafile(exarg_T *eap)
{
    if (lua_init() == FAIL)
	return;
    if (!eap->skip)
    {
	luaV_setrange(L, eap->line1, eap->line2);
	if (luaL_loadfile(L, (char *) eap->arg) || lua_pcall(L, 0, 0, 0))
	    luaV_emsg(L);
    }
}

/* buffer */
    void
lua_buffer_free(buf_T *buf)
{
    if (!lua_is_open()) return;
    luaV_getfield(L, LUAVIM_FREE);
    lua_pushlightuserdata(L, (void *) buf);
    lua_call(L, 1, 0);
}

/* window */
    void
lua_window_free(win_T *win)
{
    if (!lua_is_open()) return;
    luaV_getfield(L, LUAVIM_FREE);
    lua_pushlightuserdata(L, (void *) win);
    lua_call(L, 1, 0);
}

#endif
