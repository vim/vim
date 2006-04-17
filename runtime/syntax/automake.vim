" Vim syntax file
" Language:	automake Makefile.am
" Maintainer:   Debian VIM Maintainers <pkg-vim-maintainers@lists.alioth.debian.org>
" Former Maintainer:	John Williams <jrw@pobox.com>
" Last Change:	$LastChangedDate: 2006-04-16 22:06:40 -0400 (dom, 16 apr 2006) $
" URL: http://svn.debian.org/wsvn/pkg-vim/trunk/runtime/syntax/automake.vim?op=file&rev=0&sc=0
"
" XXX This file is in need of a new maintainer, Debian VIM Maintainers maintain
"     it only because patches have been submitted for it by Debian users and the
"     former maintainer was MIA (Missing In Action), taking over its
"     maintenance was thus the only way to include those patches.
"     If you care about this file, and have time to maintain it please do so!
"
" This script adds support for automake's Makefile.am format. It highlights
" Makefile variables significant to automake as well as highlighting
" autoconf-style @variable@ substitutions . Subsitutions are marked as errors
" when they are used in an inappropriate place, such as in defining
" EXTRA_SOURCES.


" Read the Makefile syntax to start with
if version < 600
  source <sfile>:p:h/make.vim
else
  runtime! syntax/make.vim
endif

syn match automakePrimary "^[A-Za-z0-9_]\+\(_PROGRAMS\|LIBRARIES\|_LIST\|_SCRIPTS\|_DATA\|_HEADERS\|_MANS\|_TEXINFOS\|_JAVA\|_LTLIBRARIES\)\s*="me=e-1
syn match automakePrimary "^TESTS\s*="me=e-1
syn match automakeSecondary "^[A-Za-z0-9_]\+\(_SOURCES\|_LDADD\|_LIBADD\|_LDFLAGS\|_DEPENDENCIES\|_CPPFLAGS\)\s*="me=e-1
syn match automakeSecondary "^OMIT_DEPENDENCIES\s*="me=e-1
syn match automakeExtra "^EXTRA_[A-Za-z0-9_]\+\s*="me=e-1
syn match automakeOptions "^\(AUTOMAKE_OPTIONS\|ETAGS_ARGS\|TAGS_DEPENDENCIES\)\s*="me=e-1
syn match automakeClean "^\(MOSTLY\|DIST\|MAINTAINER\)\=CLEANFILES\s*="me=e-1
syn match automakeSubdirs "^\(DIST_\)\=SUBDIRS\s*="me=e-1
syn match automakeConditional "^\(if\s*[a-zA-Z0-9_]\+\|else\|endif\)\s*$"

syn match automakeSubst     "@[a-zA-Z0-9_]\+@"
syn match automakeSubst     "^\s*@[a-zA-Z0-9_]\+@"
syn match automakeComment1 "#.*$" contains=automakeSubst
syn match automakeComment2 "##.*$"

syn match automakeMakeError "$[{(][^})]*[^a-zA-Z0-9_})][^})]*[})]" " GNU make function call

syn region automakeNoSubst start="^EXTRA_[a-zA-Z0-9_]*\s*=" end="$" contains=ALLBUT,automakeNoSubst transparent
syn region automakeNoSubst start="^DIST_SUBDIRS\s*=" end="$" contains=ALLBUT,automakeNoSubst transparent
syn region automakeNoSubst start="^[a-zA-Z0-9_]*_SOURCES\s*=" end="$" contains=ALLBUT,automakeNoSubst transparent
syn match automakeBadSubst  "@\([a-zA-Z0-9_]*@\=\)\=" contained

syn region  automakeMakeDString start=+"+  skip=+\\"+  end=+"+  contains=makeIdent,automakeSubstitution
syn region  automakeMakeSString start=+'+  skip=+\\'+  end=+'+  contains=makeIdent,automakeSubstitution
syn region  automakeMakeBString start=+`+  skip=+\\`+  end=+`+  contains=makeIdent,makeSString,makeDString,makeNextLine,automakeSubstitution

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_automake_syntax_inits")
  if version < 508
    let did_automake_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink automakePrimary     Statement
  HiLink automakeSecondary   Type
  HiLink automakeExtra       Special
  HiLink automakeOptions     Special
  HiLink automakeClean       Special
  HiLink automakeSubdirs     Statement
  HiLink automakeConditional PreProc
  HiLink automakeSubst       PreProc
  HiLink automakeComment1    makeComment
  HiLink automakeComment2    makeComment
  HiLink automakeMakeError   makeError
  HiLink automakeBadSubst    makeError
  HiLink automakeMakeDString makeDString
  HiLink automakeMakeSString makeSString
  HiLink automakeMakeBString makeBString

  delcommand HiLink
endif

let b:current_syntax = "automake"

" vi: ts=8 sw=4 sts=4
