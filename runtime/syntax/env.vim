" Vim syntax file
" Language:    env
" Maintainer:  DuckAfire <duckafire@gmail.com>
" Last Change: 2026 Jan 26
" Version:      0
" Changelog:
" 0. Create syntax file.

if !exists("main_syntax")
	if exists("b:current_syntax")
		finish
	endif

	let main_syntax = "env"
endif

sy match   envField   nextgroup=envValue         /^\h\(\w\|\.\)*/
sy region  envValue   matchgroup=Operator        start=/=/ end=/$/
sy match   envComment contains=envTodo,envTitles /^#.*$/
sy keyword envTodo    contained                  CAUTION NOTE TODO WARN WARNING
sy match   envTitles  contained                  /#\s*\zs[A-Z0-9][A-Z0-9 ]*:/

hi def link envField   Identifier
hi def link envValue   String
hi def link envComment Comment
hi def link envTodo    Todo
hi def link envTitles  PreProc

let b:current_syntax = "env"

