" Vim syntax file
" Language:	caddy configure file
" Maintainer:	Vladimir Levin <opaozhub@gmail.com>
" Last Change:	2025 Jan 26

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn keyword	caddyTodo	contained TODO FIXME XXX

syn match	caddyComment	"^#.*" contains=caddyTodo,@Spell
syn match	caddyComment	"\s#.*"ms=s+1 contains=caddyTodo,@Spell

syn region	caddyString	start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline
syn region	caddyQuotedString	start=+`+ skip=+\\\\\|\\`+ end=+`+ oneline

syn region caddyPlaceholder start=+{+ end=+}+ oneline

" These regular expressions where taken from `syntax/nginx.vim`
syn match caddyInteger '\W\zs\(\d[0-9.]*\|[0-9.]*\d\)\w\?\ze\W'
syn match caddyIPaddr '\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{6}\(\x\{1,4}:\x\{1,4}\|\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[::\(\(\x\{1,4}:\)\{,6}\x\{1,4}\|\(\x\{1,4}:\)\{,5}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{1}:\(\(\x\{1,4}:\)\{,5}\x\{1,4}\|\(\x\{1,4}:\)\{,4}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{2}:\(\(\x\{1,4}:\)\{,4}\x\{1,4}\|\(\x\{1,4}:\)\{,3}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{3}:\(\(\x\{1,4}:\)\{,3}\x\{1,4}\|\(\x\{1,4}:\)\{,2}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{4}:\(\(\x\{1,4}:\)\{,2}\x\{1,4}\|\(\x\{1,4}:\)\{,1}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{5}:\(\(\x\{1,4}:\)\{,1}\x\{1,4}\|\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyIPaddr '\[\(\x\{1,4}:\)\{6}:\x\{1,4}\]'
" Highlight wildcard listening signs also as IPaddr
syn match ngxIPaddr '\s\zs\[::]'
syn match ngxIPaddr '\s\zs\*'


syn match caddyDirective "\v^\s*(\w\S*)" contains=caddyDirectiveKeyword
syn match caddyDirectiveKeyword /\<\w\S*\>/ contained

" Define the default highlighting.
" Only used when an item doesn't have highlighting yet
hi def link caddyComment	Comment
hi def link caddyTodo	Todo
hi def link caddyString	String
hi def link caddyQuotedString	String
hi def link caddyPlaceholder Special
hi def link caddyIPaddr Delimiter
hi def link caddyInteger Number
hi def link caddyDirectiveKeyword     Identifier

let b:current_syntax = "caddyfile"

