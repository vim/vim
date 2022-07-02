" Vim syntax file
" Language:	GDB command files
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/gdb.vim
" Last Change:	2021 Nov 15
" 		Additional changes by Simon Sobisch

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword gdbInfo contained address args auxv bookmarks breakpoints b checkpoints classes common connections copying dcache
syn keyword gdbInfo contained display exceptions extensions files float frame f functions guile gu inferiors line locals macro
syn keyword gdbInfo contained macros mem module modules os probes proc program record rec registers r scope selectors set
syn keyword gdbInfo contained sharedlibrary dll signals handle skip source sources stack s symbol target tasks terminal threads
syn keyword gdbInfo contained tracepoints tvariables types variables vector vtbl warranty watchpoints win
syn match gdbInfo contained "\<all-registers\>"
syn match gdbInfo contained "\<auto-load\>"
syn match gdbInfo contained "\<static-tracepoint-markers\>"
" obsolete?
syn keyword gdbInfo architecture catch udot

syn keyword gdbStatement contained actions apply attach awatch backtrace break bt call catch cd clear collect commands
syn keyword gdbStatement contained complete condition continue delete detach directory disable disas[semble] disp[lay] down
syn keyword gdbStatement contained echo else enable end file finish frame handle hbreak help if ignore
syn keyword gdbStatement contained inspect jump kill list load maintenance make next nexti ni output overlay
syn keyword gdbStatement contained passcount path print printf ptype pwd quit rbreak remote return run rwatch
syn keyword gdbStatement contained search section set sharedlibrary shell show si signal skip source step stepi stepping
syn keyword gdbStatement contained stop target tbreak tdump tfind thbreak thread tp trace tstart tstatus tstop
syn keyword gdbStatement contained tty und[isplay] unset until up watch whatis where while ws x
syn match gdbFuncDef "\<define\>.*"
syn match gdbStatementContainer "^\s*\S\+" contains=gdbStatement,gdbMultilineStatement,gdbFuncDef
syn match gdbStatement "^\s*info" nextgroup=gdbInfo skipwhite skipempty

" some commonly used abbreviations
syn keyword gdbStatement c cont p

syn region gdbDocument matchgroup=gdbFuncDef start="\<document\>.*$" matchgroup=gdbFuncDef end="^end\s*$"

" Guile
syn include @gdbGuile syntax/scheme.vim
unlet b:current_syntax
syn match   gdbStatement contained "\<guile-repl\>"
syn keyword gdbStatement contained gr
syn region  gdbStatement contained matchgroup=gdbStatement start="\<gu\%(ile\)\=\ze\s" skip="\\$" end="$" contains=@gdbGuile keepend transparent fold
syn region  gdbMultilineStatement contained matchgroup=gdbStatement start="\<gu\%(ile\)\=\ze\s*$" end="^\s*\zsend\ze\s*$" contains=@gdbGuile transparent fold

" Python
syn include @gdbPython syntax/python.vim
unlet b:current_syntax
syn region gdbStatement contained matchgroup=gdbStatement start="\<py\%(thon\)\=\ze\s" start="\<\%(python-interactive\|pi\)\ze\s" skip="\\$" end="$" contains=@gdbPython keepend transparent fold
syn region gdbMultilineStatement contained matchgroup=gdbStatement start="\<py\%(thon\)\=\ze\s*$" end="^\s*\zsend\ze\s*$" contains=@gdbPython transparent fold
syn match  gdbStatement contained "\<\%(python-interactive\|pi\)\s*$"

syn match gdbStatement "\<add-shared-symbol-files\>"
syn match gdbStatement "\<add-symbol-file\>"
syn match gdbStatement "\<core-file\>"
syn match gdbStatement "\<dont-repeat\>"
syn match gdbStatement "\<down-silently\>"
syn match gdbStatement "\<exec-file\>"
syn match gdbStatement "\<forward-search\>"
syn match gdbStatement "\<reverse-search\>"
syn match gdbStatement "\<save-tracepoints\>"
syn match gdbStatement "\<select-frame\>"
syn match gdbStatement "\<symbol-file\>"
syn match gdbStatement "\<up-silently\>"
syn match gdbStatement "\<while-stepping\>"

syn keyword gdbSet annotate architecture args check complaints confirm editing endian
syn keyword gdbSet environment gnutarget height history language listsize print prompt
syn keyword gdbSet radix remotebaud remotebreak remotecache remotedebug remotedevice remotelogbase
syn keyword gdbSet remotelogfile remotetimeout remotewritesize targetdebug variable verbose
syn keyword gdbSet watchdog width write
syn match gdbSet "\<auto-solib-add\>"
syn match gdbSet "\<solib-absolute-prefix\>"
syn match gdbSet "\<solib-search-path\>"
syn match gdbSet "\<stop-on-solib-events\>"
syn match gdbSet "\<symbol-reloading\>"
syn match gdbSet "\<input-radix\>"
syn match gdbSet "\<demangle-style\>"
syn match gdbSet "\<output-radix\>"

syn match gdbComment "^\s*#.*" contains=@Spell

syn match gdbVariable "\$\K\k*"

" Strings and constants
syn region  gdbString		start=+"+  skip=+\\\\\|\\"+  end=+"+ contains=@Spell
syn match   gdbCharacter	"'[^']*'" contains=gdbSpecialChar,gdbSpecialCharError
syn match   gdbCharacter	"'\\''" contains=gdbSpecialChar
syn match   gdbCharacter	"'[^\\]'"
syn match   gdbNumber		"\<[0-9_]\+\>"
syn match   gdbNumber		"\<0x[0-9a-fA-F_]\+\>"


if !exists("gdb_minlines")
  let gdb_minlines = 10
endif
exec "syn sync ccomment gdbComment minlines=" . gdb_minlines

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
hi def link gdbFuncDef	Function
hi def link gdbComment	Comment
hi def link gdbStatement	Statement
hi def link gdbString	String
hi def link gdbCharacter	Character
hi def link gdbVariable	Identifier
hi def link gdbSet		Constant
hi def link gdbInfo	Type
hi def link gdbDocument	Special
hi def link gdbNumber	Number

let b:current_syntax = "gdb"

let &cpo = s:cpo_save
unlet s:cpo_save
" vim: ts=8
