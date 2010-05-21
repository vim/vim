" Vim syntax file
" Language:     Sass
" Maintainer:   Tim Pope <vimNOSPAM@tpope.org>
" Filenames:    *.sass
" Last Change:	2010 May 21

if exists("b:current_syntax")
  finish
endif

runtime! syntax/css.vim

syn case ignore

syn cluster sassCssProperties contains=cssFontProp,cssFontDescriptorProp,cssColorProp,cssTextProp,cssBoxProp,cssGeneratedContentProp,cssPagingProp,cssUIProp,cssRenderProp,cssAuralProp,cssTableProp
syn cluster sassCssAttributes contains=css.*Attr,cssComment,cssValue.*,cssColor,cssURL,cssImportant,cssError,cssStringQ,cssStringQQ,cssFunction,cssUnicodeEscape,cssRenderProp

syn match sassProperty "^\s*\zs\s\%([[:alnum:]-]\+:\|:[[:alnum:]-]\+\)"hs=s+1 contains=css.*Prop skipwhite nextgroup=sassCssAttribute
syn match sassProperty "^\s*\zs\s\%(:\=[[:alnum:]-]\+\s*=\)"hs=s+1 contains=css.*Prop skipwhite nextgroup=sassScript
syn match sassCssAttribute ".*$" contained contains=@sassCssAttributes,sassConstant
syn match sassScript ".*$" contained contains=@sassCssAttributes,sassConstant
syn match sassConstant "![[:alnum:]_-]\+"
syn match sassConstantAssignment "\%(![[:alnum:]_]\+\s*\)\@<=\%(||\)\==" nextgroup=sassScript skipwhite
syn match sassMixin  "^=.*"
syn match sassMixing "^\s\+\zs+.*"

syn match sassEscape     "^\s*\zs\\"
syn match sassIdChar     "#[[:alnum:]_-]\@=" nextgroup=sassId
syn match sassId         "[[:alnum:]_-]\+" contained
syn match sassClassChar  "\.[[:alnum:]_-]\@=" nextgroup=sassClass
syn match sassClass      "[[:alnum:]_-]\+" contained
syn match sassAmpersand  "&"

" TODO: Attribute namespaces
" TODO: Arithmetic (including strings and concatenation)

syn region sassInclude start="@import" end=";\|$" contains=cssComment,cssURL,cssUnicodeEscape,cssMediaType
syn region sassDebugLine matchgroup=sassDebug start="@debug\>" end="$" contains=@sassCssAttributes,sassConstant
syn region sassControlLine matchgroup=sassControl start="@\%(if\|else\%(\s\+if\)\=\|while\|for\)\>" end="$" contains=sassFor,@sassCssAttributes,sassConstant
syn keyword sassFor from to through contained

syn keyword sassTodo        FIXME NOTE TODO OPTIMIZE XXX contained
syn region  sassComment     start="^\z(\s*\)//"  end="^\%(\z1 \)\@!" contains=sassTodo
syn region  sassCssComment  start="^\z(\s*\)/\*" end="^\%(\z1 \)\@!" contains=sassTodo

hi def link sassCssComment              sassComment
hi def link sassComment                 Comment
hi def link sassConstant                Identifier
hi def link sassMixing                  PreProc
hi def link sassMixin                   PreProc
hi def link sassTodo                    Todo
hi def link sassInclude                 Include
hi def link sassDebug                   Debug
hi def link sassControl                 PreProc
hi def link sassFor                     PreProc
hi def link sassEscape                  Special
hi def link sassIdChar                  Special
hi def link sassClassChar               Special
hi def link sassAmpersand               Character
hi def link sassId                      Identifier
hi def link sassClass                   Type

let b:current_syntax = "sass"

" vim:set sw=2:
