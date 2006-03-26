" Vim syntax file
" Language:         reStructuredText documentation format
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2006-03-26

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword     rstTodo             contained FIXME TODO XXX NOTE

syn case ignore

syn region      rstComment          start='^\.\.\%( \%([a-z0-9_.-]\+::\)\@!\|$\)'
                                    \ end='^\s\@!' contains=rstTodo

syn cluster     rstCruft            contains=rstFootnoteLabel,rstCitationLabel,
                                    \ rstSubstitutionLabel,rstInline,
                                    \ rstHyperlinks,rstInternalTarget

syn region      rstBlock            matchgroup=rstDelimiter
                                    \ start='::$' skip='^$' end='^\s\@!'
syn region      rstDoctestBlock     matchgroup=rstDelimiter
                                    \ start='^>>>\s' end='^$'

" TODO: these may actually be a bit too complicated to match correctly and
" should perhaps be removed.  We won't really needs it anyway?
syn region      rstTable            transparent start='^\n\s*+[-=+]\+' end='^$'
                                    \ contains=rstTableLines,@rstCruft
syn match       rstTableLines       contained '^\s*[|+=-]\+$'
syn region      rstSimpleTable      transparent
                                    \ start='^\n\%(\s*\)\@>\%(\%(=\+\)\@>\%(\s\+\)\@>\)\%(\%(\%(=\+\)\@>\%(\s*\)\@>\)\+\)\@>$'
                                    \ end='^$'
                                    \ contains=rstSimpleTableLines,@rstCruft
syn match       rstSimpleTableLines contained display
                                    \ '^\%(\s*\)\@>\%(\%(=\+\)\@>\%(\s\+\)\@>\)\%(\%(\%(=\+\)\@>\%(\s*\)\@>\)\+\)\@>$'

syn region      rstFootnote         matchgroup=rstDirective
                                    \ start='^\.\. \[\%([#*]\|[0-9]\+\|#[a-z0-9_.-]\+\)\]\s'
                                    \ end='^\s\@!' contains=@rstCruft
syn match       rstFootnoteLabel    '\[\%([#*]\|[0-9]\+\|#[a-z0-9_.-]\+\)\]_'

syn region      rstCitation         matchgroup=rstDirective
                                    \ start='^\.\. \[[a-z0-9_.-]\+\]\s'
                                    \ end='^\s\@!' contains=@rstCruft
syn match       rstCitationLabel    '\[[a-z0-9_.-]\+\]_'

syn region      rstDirectiveBody    matchgroup=rstDirective
                                    \ start='^\.\. [a-z0-9_.-]\+::'
                                    \ end='^\s\@!'

syn region      rstSubstitution     matchgroup=rstDirective
                                    \ start='^\.\. |[a-z0-9_.-]|\s[a-z0-9_.-]\+::\s'
                                    \ end='^\s\@!' contains=@rstCruft
syn match       rstSubstitutionLbl  '|[a-z0-9_.-]|'

syn match       rstInline           '\*\{1,2}\S\%([^*]*\S\)\=\*\{1,2}'
syn match       rstInline           '`\{1,2}\S\%([^`]*\S\)\=`\{1,2}'

syn region      rstHyperlinks       matchgroup=RstDirective
                                    \ start='^\.\. _[a-z0-9_. -]\+:\s'
                                    \ end='^\s\@!' contains=@rstCruft

syn match       rstHyperlinksLabel  '`\S\%([^`]*\S\)\=`__\=\>'
syn match       rstHyperlinksLabel  '\w\+__\=\>'

syn match       rstInternalTarget   '_`\S\%([^`]*\S\)\=`'

syn match       rstListItem         '^:\w\+\%(\s\+\w\+\)*:'
syn match       rstListItem         '^\s*[-*+]\s\+'

syn sync minlines=50

hi def link rstTodo                 Todo
hi def link rstComment              Comment
hi def link rstDelimiter            Delimiter
hi def link rstBlock                String
hi def link rstDoctestBlock         PreProc
hi def link rstTableLines           Delimiter
hi def link rstSimpleTableLines     rstTableLines
hi def link rstFootnote             String
hi def link rstFootnoteLabel        Identifier
hi def link rstCitation             String
hi def link rstCitationLabel        Identifier
hi def link rstDirective            Keyword
hi def link rstDirectiveBody        Type
hi def link rstSubstitution         String
hi def link rstSubstitutionLbl      Identifier
hi def link rstHyperlinks           String
hi def link rstHyperlinksLabel      Identifier
hi def link rstListItem             Identifier
hi def      rstInline               term=italic cterm=italic gui=italic
hi def      rstInternalTarget       term=italic cterm=italic gui=italic

let b:current_syntax = "rst"

let &cpo = s:cpo_save
unlet s:cpo_save
