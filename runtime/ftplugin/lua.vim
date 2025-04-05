" Vim filetype plugin file.
" Language:		Lua
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Previous Maintainer:	Max Ischenko <mfi@ukr.net>
" Contributor:		Dorai Sitaram <ds26@gte.com>
"			C.D. MacEachern <craig.daniel.maceachern@gmail.com>
"			Tyler Miller <tmillr@proton.me>
"			Phạm Bình An <phambinhanctb2004@gmail.com>
"			@konfekt
" Last Change:		2025 Apr 04

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

" keep in sync with syntax/lua.vim
if !exists("lua_version")
  " Default is lua 5.3
  let lua_version = 5
  let lua_subversion = 3
elseif !exists("lua_subversion")
  " lua_version exists, but lua_subversion doesn't. In this case set it to 0
  let lua_subversion = 0
endif

let s:cpo_save = &cpo
set cpo&vim

setlocal comments=:---,:--
setlocal commentstring=--\ %s
setlocal formatoptions-=t formatoptions+=croql

let &l:define = '\<function\|\<local\%(\s\+function\)\='

let &l:include = '\<\%(\%(do\|load\)file\|require\)\s*('
setlocal includeexpr=s:LuaInclude(v:fname)
setlocal suffixesadd=.lua

let b:undo_ftplugin = "setl cms< com< def< fo< inc< inex< sua<"

if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_ignorecase = 0
  let b:match_words =
	\ '\<\%(do\|function\|if\)\>:' ..
	\ '\<\%(return\|else\|elseif\)\>:' ..
	\ '\<end\>,' ..
	\ '\<repeat\>:\<until\>,' ..
	\ '\%(--\)\=\[\(=*\)\[:]\1]'
  let b:undo_ftplugin ..= " | unlet! b:match_words b:match_ignorecase"
endif

if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
  let b:browsefilter = "Lua Source Files (*.lua)\t*.lua\n"
  if has("win32")
    let b:browsefilter ..= "All Files (*.*)\t*\n"
  else
    let b:browsefilter ..= "All Files (*)\t*\n"
  endif
  let b:undo_ftplugin ..= " | unlet! b:browsefilter"
endif

if has("folding") && get(g:, "lua_folding", 0)
  setlocal foldmethod=expr
  setlocal foldexpr=s:LuaFold()
  let b:lua_lasttick = -1
  let b:undo_ftplugin ..= " | setl foldexpr< foldmethod< | unlet! b:lua_lasttick b:lua_foldlists"
endif

" The rest of the file needs to be :sourced only once per Vim session
if exists("s:loaded_lua") || &cp
  let &cpo = s:cpo_save
  unlet s:cpo_save
  finish
endif
let s:loaded_lua = 1

function s:LuaInclude(fname) abort
  let lua_ver = str2float(printf("%d.%02d", g:lua_version, g:lua_subversion))
  let fname = tr(a:fname, '.', '/')
  let paths = lua_ver >= 5.03 ? [fname .. ".lua", fname .. "/init.lua"] : [fname .. ".lua"]
  for path in paths
    if filereadable(path)
      return path
    endif
  endfor
  return fname
endfunction

let s:patterns = [
      \ ['do', 'end'],
      \ ['if\s+.+\s+then', 'end'],
      \ ['repeat', 'until\s+.+'],
      \ ['for\s+.+\s+do', 'end'],
      \ ['while\s+.+\s+do', 'end'],
      \ ['function.+', 'end'],
      \ ['return\s+function.+', 'end'],
      \ ['local\s+function\s+.+', 'end'],
      \ ]

if !has('vim9script')
  function s:LuaFold() abort
    if b:lua_lasttick == b:changedtick
      return b:lua_foldlists[v:lnum - 1]
    endif
    let b:lua_lasttick = b:changedtick

    let b:lua_foldlists = []
    let foldlist = []
    let buf = getline(1, "$")
    for line in buf
      for t in s:patterns
      	let end = 0
      	let tagopen  = '\v^\s*' .. t[0] ..'\s*$'
      	let tagclose = '\v^\s*' .. t[1] ..'\s*$'
      	if line =~# tagopen
	  call add(foldlist, t)
	  break
      	elseif line =~# tagclose
	  if len(foldlist) > 0 && line =~# foldlist[-1][1]
	    call remove(foldlist, -1)
      	    let end = 1
	  else
	    let foldlist = []
	  endif
	  break
      	endif
      endfor
      call add(b:lua_foldlists, len(foldlist) + end)
    endfor

    return b:lua_foldlists[v:lnum - 1]
  endfunction

  let &cpo = s:cpo_save
  unlet s:cpo_save

  finish
else
  def s:LuaFold(): number
    if b:lua_lasttick == b:changedtick
      return b:lua_foldlists[v:lnum - 1]
    endif
    b:lua_lasttick = b:changedtick

    b:lua_foldlists = []
    var foldlist = []
    var buf = getline(1, "$")
    for line in buf
      var end = 0
      for t in patterns
      	var tagopen  = '\v^\s*' .. t[0] .. '\s*$'
      	var tagclose = '\v^\s*' .. t[1] .. '\s*$'
      	if line =~# tagopen
	  add(foldlist, t)
	  break
      	elseif line =~# tagclose
	  if len(foldlist) > 0 && line =~# foldlist[-1][1]
	    end = 1
	    remove(foldlist, -1)
	  else
	    foldlist = []
	  endif
	  break
      	endif
      endfor
      add(b:lua_foldlists, len(foldlist) + end)
    endfor

    return b:lua_foldlists[v:lnum - 1]
  enddef

  let &cpo = s:cpo_save
  unlet s:cpo_save
endif

" vim: nowrap sw=2 sts=2 ts=8 noet:
