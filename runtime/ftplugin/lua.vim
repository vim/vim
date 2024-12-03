" Vim filetype plugin file.
" Language:		Lua
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Previous Maintainer:	Max Ischenko <mfi@ukr.net>
" Contributor:		Dorai Sitaram <ds26@gte.com>
"			C.D. MacEachern <craig.daniel.maceachern@gmail.com>
"			Tyler Miller <tmillr@proton.me>
" Last Change:		2024 Jan 14

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

setlocal comments=:---,:--
setlocal commentstring=--\ %s
setlocal formatoptions-=t formatoptions+=croql

let &l:define = '\<function\|\<local\%(\s\+function\)\='

" TODO: handle init.lua
setlocal includeexpr=tr(v:fname,'.','/')
setlocal suffixesadd=.lua

let b:undo_ftplugin = "setlocal cms< com< def< fo< inex< sua<"

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

" fold Lua functions
silent! function LuaFold(lnum)
  lua foldlevel_luacode()
endfunction

if has("folding") && get(g:, "lua_folding", 0)
  setlocal foldexpr=LuaFold(v:lnum)
  setlocal foldmethod=expr
  let b:undo_ftplugin ..= "|setl foldexpr< foldmethod<"
endif

let &cpo = s:cpo_save
unlet s:cpo_save

" The rest of the file needs to be :sourced only once per session.
" let b:foldlist = []
" let b:lasttick = -1

if exists('s:loaded_lua') || &cp | finish | endif
let s:loaded_lua = 1

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

function! FoldLevelLua(lnum) abort
  " if b:lasttick == b:changedtick
  "   return len(b:foldlist)
  " endif
  " let b:lasttick = b:changedtick

  let b:foldlist = []
  let buf = getline(1, a:lnum)
  for line in buf
    for t in s:patterns
      let tagopen = '\v^\s*'..t[0]..'\s*$'
      let tagclose = '\v^\s*'..t[1]..'\s*$'
      if line =~# tagopen
        call add(b:foldlist, t)
        break
      elseif line =~# tagclose
        if len(b:foldlist) > 0 && line =~# b:foldlist[-1][1]
          call remove(b:foldlist, -1)
        else
          let b:foldlist = []
        endif
        break
      endif
    endfor
  endfor

  return len(b:foldlist)
endfunction
