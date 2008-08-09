" Vim syntax file
" Language:     Haml
" Maintainer:   Tim Pope <vimNOSPAM@tpope.info>
" Filenames:    *.haml

if exists("b:current_syntax")
  finish
endif

if !exists("main_syntax")
  let main_syntax = 'haml'
endif
let b:ruby_no_expensive = 1

runtime! syntax/html.vim
unlet! b:current_syntax
silent! syn include @hamlSassTop syntax/sass.vim
unlet! b:current_syntax
syn include @hamlRubyTop syntax/ruby.vim

syn case match

syn cluster hamlComponent    contains=hamlAttributes,hamlClassChar,hamlIdChar,hamlObject,hamlDespacer,hamlSelfCloser,hamlRuby,hamlPlainChar,hamlInterpolatable
syn cluster hamlEmbeddedRuby contains=hamlAttributes,hamlObject,hamlRuby,hamlRubyFilter
syn cluster hamlTop          contains=hamlBegin,hamlPlainFilter,hamlRubyFilter,hamlSassFilter,hamlComment,hamlHtmlComment

syn match   hamlBegin "^\s*[<>&]\@!" nextgroup=hamlTag,hamlAttributes,hamlClassChar,hamlIdChar,hamlObject,hamlRuby,hamlPlainChar,hamlInterpolatable

syn match   hamlTag        "%\w\+" contained contains=htmlTagName,htmlSpecialTagName nextgroup=@hamlComponent
syn region  hamlAttributes matchgroup=hamlAttributesDelimiter start="{" end="}" contained contains=@hamlRubyTop nextgroup=@hamlComponent
syn region  hamlObject     matchgroup=hamlObjectDelimiter   start="\[" end="\]" contained contains=@hamlRubyTop nextgroup=@hamlComponent
syn match   hamlDespacer "[<>]" contained nextgroup=hamlDespacer,hamlSelfCloser,hamlRuby,hamlPlainChar,hamlInterpolatable
syn match   hamlSelfCloser "/" contained
syn match   hamlClassChar "\." contained nextgroup=hamlClass
syn match   hamlIdChar    "#"  contained nextgroup=hamlId
syn match   hamlClass "\%(\w\|-\)\+" contained nextgroup=@hamlComponent
syn match   hamlId    "\%(\w\|-\)\+" contained nextgroup=@hamlComponent
syn region  hamlDocType start="^\s*!!!" end="$"

syn region  hamlRuby   matchgroup=hamlRubyOutputChar start="[=~]" end="$" contained contains=@hamlRubyTop keepend
syn region  hamlRuby   matchgroup=hamlRubyChar       start="-"    end="$" contained contains=@hamlRubyTop keepend
syn match   hamlPlainChar "\\" contained
syn region hamlInterpolatable matchgroup=hamlInterpolatableChar start="==" end="$" keepend contained contains=hamlInterpolation
syn region hamlInterpolation matchgroup=hamlInterpolationDelimiter start="#{" end="}" contained contains=@hamlRubyTop
syn region hamlErbInterpolation matchgroup=hamlInterpolationDelimiter start="<%[=-]\=" end="-\=%>" contained contains=@hamlRubyTop

syn match   hamlHelper  "\<action_view?\|\.\@<!\<\%(flatten\|open\|puts\)" contained containedin=@hamlEmbeddedRuby,@hamlRubyTop,rubyInterpolation
syn keyword hamlHelper   capture_haml find_and_preserve  html_attrs init_haml_helpers list_of preced preserve succeed surround tab_down tab_up page_class contained containedin=@hamlEmbeddedRuby,@hamlRubyTop,rubyInterpolation

syn cluster hamlHtmlTop contains=@htmlTop,htmlBold,htmlItalic,htmlUnderline
syn region  hamlPlainFilter matchgroup=hamlFilter start="^\z(\s*\):\%(plain\|preserve\|erb\|redcloth\|textile\|markdown\)\s*$" end="^\%(\z1 \)\@!" contains=@hamlHtmlTop,rubyInterpolation
syn region  hamlEscapedFilter matchgroup=hamlFilter start="^\z(\s*\):\%(escaped\)\s*$" end="^\%(\z1 \)\@!" contains=rubyInterpolation
syn region  hamlErbFilter  matchgroup=hamlFilter start="^\z(\s*\):erb\s*$" end="^\%(\z1 \)\@!" contains=@hamlHtmlTop,hamlErbInterpolation
syn region  hamlRubyFilter  matchgroup=hamlFilter start="^\z(\s*\):ruby\s*$" end="^\%(\z1 \)\@!" contains=@hamlRubyTop
syn region  hamlSassFilter  matchgroup=hamlFilter start="^\z(\s*\):sass\s*$" end="^\%(\z1 \)\@!" contains=@hamlSassTop

syn region  hamlJavascriptBlock start="^\z(\s*\)%script" nextgroup=@hamlComponent,hamlError end="^\%(\z1 \)\@!" contains=@hamlTop,@htmlJavaScript keepend
syn region  hamlCssBlock        start="^\z(\s*\)%style" nextgroup=@hamlComponent,hamlError end="^\%(\z1 \)\@!" contains=@hamlTop,@htmlCss keepend
syn match   hamlError "\$" contained

syn region  hamlComment     start="^\z(\s*\)-#" end="^\%(\z1 \)\@!" contains=rubyTodo
syn region  hamlHtmlComment start="^\z(\s*\)/" end="^\%(\z1 \)\@!" contains=@hamlTop,rubyTodo
syn match   hamlIEConditional "\%(^\s*/\)\@<=\[if\>[^]]*]" contained containedin=hamlHtmlComment

hi def link hamlSelfCloser             Special
hi def link hamlDespacer               Special
hi def link hamlClassChar              Special
hi def link hamlIdChar                 Special
hi def link hamlTag                    Special
hi def link hamlClass                  Type
hi def link hamlId                     Identifier
hi def link hamlPlainChar              Special
hi def link hamlInterpolatableChar     hamlRubyChar
hi def link hamlRubyOutputChar         hamlRubyChar
hi def link hamlRubyChar               Special
hi def link hamlInterpolationDelimiter Delimiter
hi def link hamlDocType                PreProc
hi def link hamlFilter                 PreProc
hi def link hamlAttributesDelimiter    Delimiter
hi def link hamlObjectDelimiter        Delimiter
hi def link hamlHelper                 Function
hi def link hamlHtmlComment            hamlComment
hi def link hamlComment                Comment
hi def link hamlIEConditional          SpecialComment
hi def link hamlError                  Error

let b:current_syntax = "haml"

" vim:set sw=2:
