" Vim syntax file
" Language:		eRuby
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Info:			$Id$
" URL:			http://vim-ruby.rubyforge.org
" Anon CVS:		See above site
" Release Coordinator:	Doug Kearns <dougkearns@gmail.com>

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if !exists("main_syntax")
  let main_syntax = 'eruby'
endif

if version < 600
  so <sfile>:p:h/html.vim
  syn include @rubyTop <sfile>:p:h/ruby.vim
else
  runtime! syntax/html.vim
  unlet b:current_syntax
  syn include @rubyTop syntax/ruby.vim
endif

syn cluster erubyRegions contains=erubyOneLiner,erubyBlock,erubyExpression,erubyComment

syn region  erubyOneLiner   matchgroup=erubyDelimiter start="^%%\@!" end="$"  contains=@rubyTop	       containedin=ALLBUT,@erubyRegions keepend oneline
syn region  erubyBlock	    matchgroup=erubyDelimiter start="<%%\@!" end="%>" contains=@rubyTop	       containedin=ALLBUT,@erubyRegions
syn region  erubyExpression matchgroup=erubyDelimiter start="<%="    end="%>" contains=@rubyTop	       containedin=ALLBUT,@erubyRegions
syn region  erubyComment    matchgroup=erubyDelimiter start="<%#"    end="%>" contains=rubyTodo,@Spell containedin=ALLBUT,@erubyRegions keepend

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_eruby_syntax_inits")
  if version < 508
    let did_ruby_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink erubyDelimiter		Delimiter
  HiLink erubyComment		Comment

  delcommand HiLink
endif
let b:current_syntax = "eruby"

if main_syntax == 'eruby'
  unlet main_syntax
endif

" vim: nowrap sw=2 sts=2 ts=8 ff=unix:
