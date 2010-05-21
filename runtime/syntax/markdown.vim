" Vim syntax file
" Language:     Markdown
" Maintainer:   Tim Pope <vimNOSPAM@tpope.org>
" Filenames:    *.markdown
" Last Change:	2010 May 21

if exists("b:current_syntax")
  finish
endif

runtime! syntax/html.vim
unlet! b:current_syntax

syn sync minlines=10
syn case ignore

syn match markdownValid '[<>]\S\@!'
syn match markdownValid '&\%(#\=\w*;\)\@!'

syn match markdownLineStart "^[<@]\@!" nextgroup=@markdownBlock

syn cluster markdownBlock contains=markdownH1,markdownH2,markdownH3,markdownH4,markdownH5,markdownH6,markdownBlockquote,markdownListMarker,markdownOrderedListMarker,markdownCodeBlock,markdownRule
syn cluster markdownInline contains=markdownLineBreak,markdownLinkText,markdownItalic,markdownBold,markdownCode,markdownEscape,@htmlTop

syn match markdownH1 ".\+\n=\+$" contained contains=@markdownInline,markdownHeadingRule
syn match markdownH2 ".\+\n-\+$" contained contains=@markdownInline,markdownHeadingRule

syn match markdownHeadingRule "^[=-]\+$" contained

syn region markdownH1 matchgroup=markdownHeadingDelimiter start="##\@!"      end="#*\s*$" keepend oneline contains=@markdownInline contained
syn region markdownH2 matchgroup=markdownHeadingDelimiter start="###\@!"     end="#*\s*$" keepend oneline contains=@markdownInline contained
syn region markdownH3 matchgroup=markdownHeadingDelimiter start="####\@!"    end="#*\s*$" keepend oneline contains=@markdownInline contained
syn region markdownH4 matchgroup=markdownHeadingDelimiter start="#####\@!"   end="#*\s*$" keepend oneline contains=@markdownInline contained
syn region markdownH5 matchgroup=markdownHeadingDelimiter start="######\@!"  end="#*\s*$" keepend oneline contains=@markdownInline contained
syn region markdownH6 matchgroup=markdownHeadingDelimiter start="#######\@!" end="#*\s*$" keepend oneline contains=@markdownInline contained

syn match markdownBlockquote ">\s" contained nextgroup=@markdownBlock

syn region markdownCodeBlock start="    \|\t" end="$" contained

" TODO: real nesting
syn match markdownListMarker " \{0,4\}[-*+]\%(\s\+\S\)\@=" contained
syn match markdownOrderedListMarker " \{0,4}\<\d\+\.\%(\s*\S\)\@=" contained

syn match markdownRule "\* *\* *\*[ *]*$" contained
syn match markdownRule "- *- *-[ -]*$" contained

syn match markdownLineBreak "\s\{2,\}$"

syn region markdownIdDeclaration matchgroup=markdownLinkDelimiter start="^ \{0,3\}!\=\[" end="\]:" oneline keepend nextgroup=markdownUrl skipwhite
syn match markdownUrl "\S\+" nextgroup=markdownUrlTitle skipwhite contained
syn region markdownUrl matchgroup=markdownUrlDelimiter start="<" end=">" oneline keepend nextgroup=markdownUrlTitle skipwhite contained
syn region markdownUrlTitle matchgroup=markdownUrlTitleDelimiter start=+"+ end=+"+ keepend contained
syn region markdownUrlTitle matchgroup=markdownUrlTitleDelimiter start=+'+ end=+'+ keepend contained
syn region markdownUrlTitle matchgroup=markdownUrlTitleDelimiter start=+(+ end=+)+ keepend contained

syn region markdownLinkText matchgroup=markdownLinkTextDelimiter start="!\=\[\%(\_[^]]*]\%( \=[[(]\)\)\@=" end="\]\%( \=[[(]\)\@=" keepend nextgroup=markdownLink,markdownId skipwhite contains=@markdownInline,markdownLineStart
syn region markdownLink matchgroup=markdownLinkDelimiter start="(" end=")" contains=markdownUrl keepend contained
syn region markdownId matchgroup=markdownIdDelimiter start="\[" end="\]" keepend contained
syn region markdownAutomaticLink matchgroup=markdownUrlDelimiter start="<\%(\w\+:\|[[:alnum:]_+-]\+@\)\@=" end=">" keepend oneline

syn region markdownItalic start="\S\@<=\*\|\*\S\@=" end="\S\@<=\*\|\*\S\@=" keepend contains=markdownLineStart
syn region markdownItalic start="\S\@<=_\|_\S\@=" end="\S\@<=_\|_\S\@=" keepend contains=markdownLineStart
syn region markdownBold start="\S\@<=\*\*\|\*\*\S\@=" end="\S\@<=\*\*\|\*\*\S\@=" keepend contains=markdownLineStart
syn region markdownBold start="\S\@<=__\|__\S\@=" end="\S\@<=__\|__\S\@=" keepend contains=markdownLineStart
syn region markdownBoldItalic start="\S\@<=\*\*\*\|\*\*\*\S\@=" end="\S\@<=\*\*\*\|\*\*\*\S\@=" keepend contains=markdownLineStart
syn region markdownBoldItalic start="\S\@<=___\|___\S\@=" end="\S\@<=___\|___\S\@=" keepend contains=markdownLineStart
syn region markdownCode matchgroup=markdownCodeDelimiter start="`" end="`" transparent keepend contains=markdownLineStart
syn region markdownCode matchgroup=markdownCodeDelimiter start="`` \=" end=" \=``" keepend contains=markdownLineStart

syn match markdownEscape "\\[][\\`*_{}()#+.!-]"

hi def link markdownH1                    htmlH1
hi def link markdownH2                    htmlH2
hi def link markdownH3                    htmlH3
hi def link markdownH4                    htmlH4
hi def link markdownH5                    htmlH5
hi def link markdownH6                    htmlH6
hi def link markdownHeadingRule           markdownRule
hi def link markdownHeadingDelimiter      Delimiter
hi def link markdownOrderedListMarker     markdownListMarker
hi def link markdownListMarker            htmlTagName
hi def link markdownBlockquote            Comment
hi def link markdownRule                  PreProc

hi def link markdownLinkText              htmlLink
hi def link markdownIdDeclaration         Typedef
hi def link markdownId                    Type
hi def link markdownAutomaticLink         markdownUrl
hi def link markdownUrl                   Float
hi def link markdownUrlTitle              String
hi def link markdownIdDelimiter           markdownLinkDelimiter
hi def link markdownUrlDelimiter          htmlTag
hi def link markdownUrlTitleDelimiter     Delimiter

hi def link markdownItalic                htmlItalic
hi def link markdownBold                  htmlBold
hi def link markdownBoldItalic            htmlBoldItalic
hi def link markdownCodeDelimiter         Delimiter

hi def link markdownEscape                Special

let b:current_syntax = "markdown"

" vim:set sw=2:
