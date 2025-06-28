" Vim syntax file
" Language:	Caddy configure file
" Maintainer:	Vladimir Levin <opaozhub@gmail.com>
" Last Change:	2025 Jan 26

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn keyword	caddyfileTodo	contained TODO FIXME XXX

syn match	caddyfileComment	"^#.*" contains=caddyfileTodo,@Spell
syn match	caddyfileComment	"\s#.*"ms=s+1 contains=caddyfileTodo,@Spell

syn region	caddyfileString	start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline
syn region	caddyfileQuotedString	start=+`+ skip=+\\\\\|\\`+ end=+`+ oneline

syn region caddyfilePlaceholder start=+{+ end=+}+ oneline

" These regular expressions where taken from `syntax/nginx.vim`
syn match caddyfileInteger '\W\zs\(\d[0-9.]*\|[0-9.]*\d\)\w\?\ze\W'
syn match caddyfileIPaddr '\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{6}\(\x\{1,4}:\x\{1,4}\|\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[::\(\(\x\{1,4}:\)\{,6}\x\{1,4}\|\(\x\{1,4}:\)\{,5}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{1}:\(\(\x\{1,4}:\)\{,5}\x\{1,4}\|\(\x\{1,4}:\)\{,4}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{2}:\(\(\x\{1,4}:\)\{,4}\x\{1,4}\|\(\x\{1,4}:\)\{,3}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{3}:\(\(\x\{1,4}:\)\{,3}\x\{1,4}\|\(\x\{1,4}:\)\{,2}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{4}:\(\(\x\{1,4}:\)\{,2}\x\{1,4}\|\(\x\{1,4}:\)\{,1}\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{5}:\(\(\x\{1,4}:\)\{,1}\x\{1,4}\|\([0-2]\?\d\{1,2}\.\)\{3}[0-2]\?\d\{1,2}\)\]'
syn match caddyfileIPaddr '\[\(\x\{1,4}:\)\{6}:\x\{1,4}\]'
" Highlight wildcard listening signs also as IPaddr
syn match caddyfileIPaddr '\s\zs\[::]'
syn match caddyfileIPaddr '\s\zs\*'


syn match caddyfileDirective "\v^\s*(\w\S*)" contains=caddyfileDirectiveKeyword
syn match caddyfileDirectiveKeyword /\<\w\S*\>/ contained

" Define the default highlighting.
" Only used when an item doesn't have highlighting yet
hi def link caddyfileComment	Comment
hi def link caddyfileTodo	Todo
hi def link caddyfileString	String
hi def link caddyfileQuotedString	String
hi def link caddyfilePlaceholder Special
hi def link caddyfileIPaddr Delimiter
hi def link caddyfileInteger Number
hi def link caddyfileDirectiveKeyword     Identifier

let b:current_syntax = "caddyfile"

