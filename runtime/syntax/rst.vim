" Vim syntax file
" Language:	    reStructuredText Documentation Format
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/rst/
" Latest Revision:  2004-05-13
" arch-tag:	    6fae09da-d5d4-49d8-aec1-e49008ea21e6

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" todo
syn keyword	rstTodo		contained FIXME TODO XXX NOTE

syn case ignore

" comments
syn region	rstComment 	matchgroup=rstComment start="^\.\.\%( \%([a-z0-9_.-]\+::\)\@!\|$\)" end="^\s\@!" contains=rstTodo

syn cluster	rstCruft    contains=rstFootnoteLabel,rstCitationLabel,rstSubstitutionLabel,rstInline,rstHyperlinks,rstInternalTarget

" blocks
" syn region	rstBlock	matchgroup=rstDelimiter start=":\@<!:$" skip="^$" end="^\s\@!" contains=@rstCruft
syn region	rstBlock	matchgroup=rstDelimiter start="::$" skip="^$" end="^\s\@!"
syn region	rstDoctestBlock	matchgroup=rstDelimiter start="^>>>\s" end="^$"

" tables
" TODO: these may actually be a bit too complicated to match correctly and
" should perhaps be removed.  Whon really needs it anyway?
syn region	rstTable	transparent start="^\n\s*+[-=+]\+" end="^$" contains=rstTableLines,@rstCruft
syn match	rstTableLines	contained "^\s*[|+=-]\+$"
syn region	rstSimpleTable	transparent start="^\n\s*\%(=\+\s\+\)\%(=\+\s*\)\+$" end="^$" contains=rstSimpleTableLines,@rstCruft
syn match	rstSimpleTableLines contained "^\s*\%(=\+\s\+\)\%(=\+\s*\)\+$"

" footnotes
syn region	rstFootnote 	matchgroup=rstDirective start="^\.\. \[\%([#*]\|[0-9]\+\|#[a-z0-9_.-]\+\)\]\s" end="^\s\@!" contains=@rstCruft
syn match	rstFootnoteLabel "\[\%([#*]\|[0-9]\+\|#[a-z0-9_.-]\+\)\]_"

" citations
syn region	rstCitation 	matchgroup=rstDirective start="^\.\. \[[a-z0-9_.-]\+\]\s" end="^\s\@!" contains=@rstCruft
syn match	rstCitationLabel "\[[a-z0-9_.-]\+\]_"

" directives
syn region	rstDirectiveBody matchgroup=rstDirective start="^\.\. [a-z0-9_.-]\+::" end="^\s\@!"

" substitutions
syn region	rstSubstitution matchgroup=rstDirective start="^\.\. |[a-z0-9_.-]|\s[a-z0-9_.-]\+::\s" end="^\s\@!" contains=@rstCruft
syn match	rstSubstitutionLabel "|[a-z0-9_.-]|"

" inline markup
syn match	rstInline	"\*\{1,2}\S\%([^*]*\S\)\=\*\{1,2}"
syn match	rstInline	"`\{1,2}\S\%([^`]*\S\)\=`\{1,2}"

" hyperlinks
syn region	rstHyperlinks	matchgroup=RstDirective start="^\.\. _[a-z0-9_. -]\+:\s" end="^\s\@!" contains=@rstCruft

syn match	rstHyperlinksLabel	"`\S\%([^`]*\S\)\=`__\=\>"
syn match	rstHyperlinksLabel	"\w\+__\=\>"

" internal targets
syn match	rstInternalTarget "_`\S\%([^`]*\S\)\=`"

" lists
syn match	rstListItem	"^:\%(\w\+\s*\)\+:"
syn match	rstListItem	"^\s*[-*+]\s\+"

syn sync minlines=50

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_rst_syn_inits")
  if version < 508
    let did_rst_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink rstTodo Todo
  HiLink rstComment Comment
  HiLink rstDelimiter Delimiter
  HiLink rstBlock String
  HiLink rstDoctestBlock PreProc
  HiLink rstTableLines Delimiter
  HiLink rstSimpleTableLines rstTableLines
  HiLink rstFootnote String
  HiLink rstFootnoteLabel Identifier
  HiLink rstCitation String
  HiLink rstCitationLabel Identifier
  HiLink rstDirective Keyword
  HiLink rstDirectiveBody Type
  HiLink rstSubstitution String
  HiLink rstSubstitutionLabel Identifier
  HiLink rstHyperlinks String
  HiLink rstHyperlinksLabel Identifier
  HiLink rstListItem Identifier
  hi def rstInline term=italic cterm=italic gui=italic
  hi def rstInternalTarget term=italic cterm=italic gui=italic
  delcommand HiLink
endif

let b:current_syntax = "rst"

" vim: set sts=2 sw=2:
