" Vim syntax file
" Language:	    Linux modutils modules.conf File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/modconf/
" Latest Revision:  2004-05-22
" arch-tag:	    b7981bdb-daa3-41d1-94b5-a3d60b627916

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" comments
syn region  modconfComment  start="#" skip="\\$" end="$" contains=modconfTodo

" todo
syn keyword modconfTodo	    FIXME TODO XXX NOTE

" keywords and similar
syn match   modconfBegin    "^" skipwhite nextgroup=modconfCommand,modconfComment

syn match   modconfCommand  "\(add\s\+\)\=(above\|below\|probe\|probeall\}"
syn region  modconfCommand  transparent matchgroup=modconfCommand start="\(add\s\+\)\=options" skip="\\$" end="$" contains=modconfModOpt
syn keyword modconfCommand  define remove keep install insmod_opt else endif
syn keyword modconfCommand  nextgroup=modconfPath skipwhite alias depfile generic_stringfile pcimapfile include isapnpmapfile usbmapfile parportmapfile ieee1394mapfile pnpbiosmapfile persistdir prune
syn match   modconfCommand  "path\(\[\w\+\]\)\=" nextgroup=modconfPath skipwhite
syn region  modconfCommand  transparent matchgroup=modconfCommand start="^\s*\(if\|elseif\)" skip="\\$" end="$" contains=modconfOp
syn region  modconfCommand  transparent matchgroup=modconfCommand start="^\s*\(post\|pre\)-\(install\|remove\)" skip="\\$" end="$"


" expressions and similay
syn match   modconfOp	    contained "\s-[fnk]\>"
syn region  modconfPath	    contained start="\(=\@=\)\=/" skip="\\$" end="\\\@!\_s"
syn match   modconfModOpt   contained "\<\w\+=\@="

if exists("modconf_minlines")
    let b:modconf_minlines = modconf_minlines
else
    let b:modconf_minlines = 50
endif
exec "syn sync minlines=" . b:modconf_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_modconf_syn_inits")
  if version < 508
    let did_modconf_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink modconfComment Comment
  HiLink modconfTodo	Todo
  HiLink modconfCommand Keyword
  HiLink modconfPath	String
  HiLink modconfOp	Identifier
  HiLink modconfModOpt  Identifier
  delcommand HiLink
endif

let b:current_syntax = "modconf"

" vim: set sts=2 sw=2:
