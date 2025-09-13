" Vim syntax file
" Language:          Kitty configuration files
" Maintainer:        MD. Mouinul Hossain Shawon <mdmouinulhossainshawon [at] gmail.com>
" Last Change:       Sat Sep 13 21:20:23 +06 2025

if exists("b:current_syntax")
  finish
endif

syn sync fromstart

" Option """"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Format: `<option_name> ...`<

syn match String /\S\+/ contains=Alpha contained
syn match Number /[+\-*\/]\{0,1}[0-9.]\+/ contained
syn match Alpha /@[0-9.]\+/ contained
syn match Color /#[0-9a-fA-F]\{3,6}/ nextgroup=Alpha contained
syn keyword Boolean contained yes no
syn keyword Constant contained none auto monospace bold italic ratio always never

syn cluster kittyPrimitive contains=Number,Boolean,Constant,Color,String,Flag,Parameter,Alpha

syn match Flag /[+-]\{1,2}[a-zA-Z0-9-_]\+/ contained
syn match Parameter /-\{1,2}[a-zA-Z0-9-]\+=\S\+/ contained

syn region kittyOption start="^\w" skip="[\n\r][ \t]*\\" end="[\r\n]" contains=kittyOptionName
syn match kittyOptionName /\w\+/ nextgroup=kittyOptionValue skipwhite contained
syn region kittyOptionValue start="\S" skip="[\r\n][ \t]*\\" end="\ze[\r\n]" contains=@kittyPrimitive contained

" Keyboard shortcut """""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Format: `map <KEYS> <action>?`

syn match KEY /[^ \t\r\n+>]\+/ contained
syn match CTRL /\<\(ctrl\|control\)\>\|\^/ contained
syn match ALT /\<\(alt\|opt\|option\)\>\|⌥/ contained
syn match SHIFT /\<\(shift\)\>\|⇧/ contained
syn match SUPER /\<\(cmd\|super\|command\)\>\|⌘/ contained
syn match AND /+/ contained
syn match WITH />/ contained

syn region kittyMap start="^\s*map" skip="[\r\n][ \t]*\\" end="[\r\n]" contains=kittyMapName,kittyMapValue

syn keyword kittyMapName nextgroup=kittyMapValue skipwhite contained map
syn region kittyMapValue start="\S" skip="[\r\n][ \t]*\\" end="\ze[\r\n]" contains=kittyMapSeq,kittyMapAction contained

syn region kittyMapAction start="\S" skip="[\r\n][ \t]*\\" end="\ze[\r\n]" contains=@kittyPrimitive contained
syn region kittyMapSeq start="\S" end="\ze\s\|^\ze[ \t]*\\" nextgroup=kittyMapAction,kittyMouseMapType skipwhite contains=CTRL,ALT,SHIFT,SUPER,AND,WITH,KEY contained

" Mouse shortcut """"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Format: `mouse_map <KEYS> <type> <grabbed> <action>?`

syn region kittyMouseMap start="^\s*mouse_map" skip="[\r\n][ \t]*\\" end="[\r\n]" contains=kittyMouseMapName,kittyMouseMapValue

syn keyword kittyMouseMapName nextgroup=kittyMouseMapValue contained mouse_map
syn region kittyMouseMapValue start="\S" skip="[\r\n][ \t]*\\" end="\ze[\r\n]" contains=kittyMapSeq,kittyMouseMapType,kittyMouseMapGrabbed contained

syn region kittyMouseMapAction start="\S" skip="[\r\n][ \t]*\\" end="\ze[\r\n]" contains=@kittyPrimitive contained

syn keyword kittyMouseMapType nextgroup=kittyMouseMapGrabbed skipwhite contained press release doublepress triplepress click doubleclick
syn match kittyMouseMapGrabbed /\(grabbed\|ungrabbed\)\%(,\(grabbed\|ungrabbed\)\)\?/ nextgroup=kittyMouseMapAction skipwhite contained

" Kitty modifier """"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Format: `kitty_mod <KEYS>`

syn region kittyMod start="^\s*kitty_mod" end="[\r\n]" contains=kittyModName,kittyMapSeq

syn keyword kittyModName nextgroup=kittyMapSeq contained kitty_mod

" Comment """""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Format: `# <content>``

syn match Comment /^#.*$/

" Line continuation """""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Allows continuing lines by adding `\` at the start of a line.
" May have leading spaces & tabs.

syn match kittyLineContinue /^[ \t]*\\[ \t]*/ containedin=kittyOptionValue,kittyMap,kittyMapAction,kittyMouseMap,kittyMouseMapValue contained

" Highlight groups """"""""""""""""""""""""""""""""""""""""""""""""""""""""""""

hi link Color Constant
hi link Flag Constant
hi link Parameter Special
hi link Alpha Type

hi link kittyOptionName Keyword
hi link kittyModName Keyword

hi link CTRL Constant
hi link ALT Constant
hi link SHIFT Constant
hi link SUPER Constant

hi link AND Comment
hi link WITH Comment

hi link KEY Special

hi link kittyMapName Function

hi link kittyMouseMapName Function
hi link kittyMouseMapType Type
hi link kittyMouseMapGrabbed Constant

hi link Comment Comment
hi link kittyLineContinue Comment
