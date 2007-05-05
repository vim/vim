" Vim syntax file
" Language:	Lua 4.0, Lua 5.0 and Lua 5.1
" Maintainer:	Marcus Aurelius Farias <marcus.cf 'at' bol com br>
" First Author:	Carlos Augusto Teixeira Mendes <cmendes 'at' inf puc-rio br>
" Last Change:	2006 Aug 10
" Options:	lua_version = 4 or 5
"		lua_subversion = 0 (4.0, 5.0) or 1 (5.1)
"		default 5.1

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if !exists("lua_version")
  " Default is lua 5.1
  let lua_version = 5
  let lua_subversion = 1
elseif !exists("lua_subversion")
  " lua_version exists, but lua_subversion doesn't. So, set it to 0
  let lua_subversion = 0
endif

syn case match

" syncing method
syn sync minlines=100

" Comments
syn keyword luaTodo             contained TODO FIXME XXX
syn match   luaComment          "--.*$" contains=luaTodo,@Spell
if lua_version == 5 && lua_subversion == 0
  syn region  luaComment        matchgroup=luaComment start="--\[\[" end="\]\]" contains=luaTodo,luaInnerComment,@Spell
  syn region  luaInnerComment   contained transparent start="\[\[" end="\]\]"
elseif lua_version > 5 || (lua_version == 5 && lua_subversion >= 1)
  " Comments in Lua 5.1: --[[ ... ]], [=[ ... ]=], [===[ ... ]===], etc.
  syn region  luaComment        matchgroup=luaComment start="--\[\z(=*\)\[" end="\]\z1\]" contains=luaTodo,@Spell
endif

" First line may start with #!
syn match luaComment "\%^#!.*"

" catch errors caused by wrong parenthesis and wrong curly brackets or
" keywords placed outside their respective blocks

syn region luaParen transparent start='(' end=')' contains=ALLBUT,luaError,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaCondStart,luaBlock,luaRepeatBlock,luaRepeat,luaStatement
syn match  luaError ")"
syn match  luaError "}"
syn match  luaError "\<\%(end\|else\|elseif\|then\|until\|in\)\>"

" Function declaration
syn region luaFunctionBlock transparent matchgroup=luaFunction start="\<function\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat

" if then else elseif end
syn keyword luaCond contained else

" then ... end
syn region luaCondEnd contained transparent matchgroup=luaCond start="\<then\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial,luaRepeat

" elseif ... then
syn region luaCondElseif contained transparent matchgroup=luaCond start="\<elseif\>" end="\<then\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat

" if ... then
syn region luaCondStart transparent matchgroup=luaCond start="\<if\>" end="\<then\>"me=e-4 contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat nextgroup=luaCondEnd skipwhite skipempty

" do ... end
syn region luaBlock transparent matchgroup=luaStatement start="\<do\>" end="\<end\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat

" repeat ... until
syn region luaRepeatBlock transparent matchgroup=luaRepeat start="\<repeat\>" end="\<until\>" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat

" while ... do
syn region luaRepeatBlock transparent matchgroup=luaRepeat start="\<while\>" end="\<do\>"me=e-2 contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaRepeat nextgroup=luaBlock skipwhite skipempty

" for ... do and for ... in ... do
syn region luaRepeatBlock transparent matchgroup=luaRepeat start="\<for\>" end="\<do\>"me=e-2 contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd nextgroup=luaBlock skipwhite skipempty

" Following 'else' example. This is another item to those
" contains=ALLBUT,... because only the 'for' luaRepeatBlock contains it.
syn keyword luaRepeat contained in

" other keywords
syn keyword luaStatement return local break
syn keyword luaOperator  and or not
syn keyword luaConstant  nil
if lua_version > 4
  syn keyword luaConstant true false
endif

" Strings
if lua_version < 5
  syn match  luaSpecial contained "\\[\\abfnrtv\'\"]\|\\\d\{,3}"
elseif lua_version == 5 && lua_subversion == 0
  syn match  luaSpecial contained "\\[\\abfnrtv\'\"[\]]\|\\\d\{,3}"
  syn region luaString2 matchgroup=luaString start=+\[\[+ end=+\]\]+ contains=luaString2,@Spell
elseif lua_version > 5 || (lua_version == 5 && lua_subversion >= 1)
  syn match  luaSpecial contained "\\[\\abfnrtv\'\"]\|\\\d\{,3}"
  syn region luaString2 matchgroup=luaString start="\[\z(=*\)\[" end="\]\z1\]" contains=@Spell
endif
syn region luaString  start=+'+ end=+'+ skip=+\\\\\|\\'+ contains=luaSpecial,@Spell
syn region luaString  start=+"+ end=+"+ skip=+\\\\\|\\"+ contains=luaSpecial,@Spell

" integer number
syn match luaNumber "\<\d\+\>"
" floating point number, with dot, optional exponent
syn match luaFloat  "\<\d\+\.\d*\%(e[-+]\=\d\+\)\=\>"
" floating point number, starting with a dot, optional exponent
syn match luaFloat  "\.\d\+\%(e[-+]\=\d\+\)\=\>"
" floating point number, without dot, with exponent
syn match luaFloat  "\<\d\+e[-+]\=\d\+\>"

" hex numbers
if lua_version > 5 || (lua_version == 5 && lua_subversion >= 1)
  syn match luaNumber "\<0x\x\+\>"
endif

" tables
syn region  luaTableBlock transparent matchgroup=luaTable start="{" end="}" contains=ALLBUT,luaTodo,luaSpecial,luaCond,luaCondElseif,luaCondEnd,luaCondStart,luaBlock,luaRepeatBlock,luaRepeat,luaStatement

syn keyword luaFunc assert collectgarbage dofile error next
syn keyword luaFunc print rawget rawset tonumber tostring type _VERSION

if lua_version == 4
  syn keyword luaFunc _ALERT _ERRORMESSAGE gcinfo
  syn keyword luaFunc call copytagmethods dostring
  syn keyword luaFunc foreach foreachi getglobal getn
  syn keyword luaFunc gettagmethod globals newtag
  syn keyword luaFunc setglobal settag settagmethod sort
  syn keyword luaFunc tag tinsert tremove
  syn keyword luaFunc _INPUT _OUTPUT _STDIN _STDOUT _STDERR
  syn keyword luaFunc openfile closefile flush seek
  syn keyword luaFunc setlocale execute remove rename tmpname
  syn keyword luaFunc getenv date clock exit
  syn keyword luaFunc readfrom writeto appendto read write
  syn keyword luaFunc PI abs sin cos tan asin
  syn keyword luaFunc acos atan atan2 ceil floor
  syn keyword luaFunc mod frexp ldexp sqrt min max log
  syn keyword luaFunc log10 exp deg rad random
  syn keyword luaFunc randomseed strlen strsub strlower strupper
  syn keyword luaFunc strchar strrep ascii strbyte
  syn keyword luaFunc format strfind gsub
  syn keyword luaFunc getinfo getlocal setlocal setcallhook setlinehook
elseif lua_version == 5
  " Not sure if all these functions need to be highlighted...
  syn keyword luaFunc _G getfenv getmetatable ipairs loadfile
  syn keyword luaFunc loadstring pairs pcall rawequal
  syn keyword luaFunc require setfenv setmetatable unpack xpcall
  if lua_subversion == 0
    syn keyword luaFunc gcinfo loadlib LUA_PATH _LOADED _REQUIREDNAME
  elseif lua_subversion == 1
    syn keyword luaFunc load module select
    syn match luaFunc /package\.cpath/
    syn match luaFunc /package\.loaded/
    syn match luaFunc /package\.loadlib/
    syn match luaFunc /package\.path/
    syn match luaFunc /package\.preload/
    syn match luaFunc /package\.seeall/
    syn match luaFunc /coroutine\.running/
  endif
  syn match   luaFunc /coroutine\.create/
  syn match   luaFunc /coroutine\.resume/
  syn match   luaFunc /coroutine\.status/
  syn match   luaFunc /coroutine\.wrap/
  syn match   luaFunc /coroutine\.yield/
  syn match   luaFunc /string\.byte/
  syn match   luaFunc /string\.char/
  syn match   luaFunc /string\.dump/
  syn match   luaFunc /string\.find/
  syn match   luaFunc /string\.len/
  syn match   luaFunc /string\.lower/
  syn match   luaFunc /string\.rep/
  syn match   luaFunc /string\.sub/
  syn match   luaFunc /string\.upper/
  syn match   luaFunc /string\.format/
  syn match   luaFunc /string\.gsub/
  if lua_subversion == 0
    syn match luaFunc /string\.gfind/
    syn match luaFunc /table\.getn/
    syn match luaFunc /table\.setn/
    syn match luaFunc /table\.foreach/
    syn match luaFunc /table\.foreachi/
  elseif lua_subversion == 1
    syn match luaFunc /string\.gmatch/
    syn match luaFunc /string\.match/
    syn match luaFunc /string\.reverse/
    syn match luaFunc /table\.maxn/
  endif
  syn match   luaFunc /table\.concat/
  syn match   luaFunc /table\.sort/
  syn match   luaFunc /table\.insert/
  syn match   luaFunc /table\.remove/
  syn match   luaFunc /math\.abs/
  syn match   luaFunc /math\.acos/
  syn match   luaFunc /math\.asin/
  syn match   luaFunc /math\.atan/
  syn match   luaFunc /math\.atan2/
  syn match   luaFunc /math\.ceil/
  syn match   luaFunc /math\.sin/
  syn match   luaFunc /math\.cos/
  syn match   luaFunc /math\.tan/
  syn match   luaFunc /math\.deg/
  syn match   luaFunc /math\.exp/
  syn match   luaFunc /math\.floor/
  syn match   luaFunc /math\.log/
  syn match   luaFunc /math\.log10/
  syn match   luaFunc /math\.max/
  syn match   luaFunc /math\.min/
  if lua_subversion == 0
    syn match luaFunc /math\.mod/
  elseif lua_subversion == 1
    syn match luaFunc /math\.fmod/
    syn match luaFunc /math\.modf/
    syn match luaFunc /math\.cosh/
    syn match luaFunc /math\.sinh/
    syn match luaFunc /math\.tanh/
  endif
  syn match   luaFunc /math\.pow/
  syn match   luaFunc /math\.rad/
  syn match   luaFunc /math\.sqrt/
  syn match   luaFunc /math\.frexp/
  syn match   luaFunc /math\.ldexp/
  syn match   luaFunc /math\.random/
  syn match   luaFunc /math\.randomseed/
  syn match   luaFunc /math\.pi/
  syn match   luaFunc /io\.stdin/
  syn match   luaFunc /io\.stdout/
  syn match   luaFunc /io\.stderr/
  syn match   luaFunc /io\.close/
  syn match   luaFunc /io\.flush/
  syn match   luaFunc /io\.input/
  syn match   luaFunc /io\.lines/
  syn match   luaFunc /io\.open/
  syn match   luaFunc /io\.output/
  syn match   luaFunc /io\.popen/
  syn match   luaFunc /io\.read/
  syn match   luaFunc /io\.tmpfile/
  syn match   luaFunc /io\.type/
  syn match   luaFunc /io\.write/
  syn match   luaFunc /os\.clock/
  syn match   luaFunc /os\.date/
  syn match   luaFunc /os\.difftime/
  syn match   luaFunc /os\.execute/
  syn match   luaFunc /os\.exit/
  syn match   luaFunc /os\.getenv/
  syn match   luaFunc /os\.remove/
  syn match   luaFunc /os\.rename/
  syn match   luaFunc /os\.setlocale/
  syn match   luaFunc /os\.time/
  syn match   luaFunc /os\.tmpname/
  syn match   luaFunc /debug\.debug/
  syn match   luaFunc /debug\.gethook/
  syn match   luaFunc /debug\.getinfo/
  syn match   luaFunc /debug\.getlocal/
  syn match   luaFunc /debug\.getupvalue/
  syn match   luaFunc /debug\.setlocal/
  syn match   luaFunc /debug\.setupvalue/
  syn match   luaFunc /debug\.sethook/
  syn match   luaFunc /debug\.traceback/
  if lua_subversion == 1
    syn match luaFunc /debug\.getfenv/
    syn match luaFunc /debug\.getmetatable/
    syn match luaFunc /debug\.getregistry/
    syn match luaFunc /debug\.setfenv/
    syn match luaFunc /debug\.setmetatable/
  endif
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_lua_syntax_inits")
  if version < 508
    let did_lua_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink luaStatement		Statement
  HiLink luaRepeat		Repeat
  HiLink luaString		String
  HiLink luaString2		String
  HiLink luaNumber		Number
  HiLink luaFloat		Float
  HiLink luaOperator		Operator
  HiLink luaConstant		Constant
  HiLink luaCond		Conditional
  HiLink luaFunction		Function
  HiLink luaComment		Comment
  HiLink luaTodo		Todo
  HiLink luaTable		Structure
  HiLink luaError		Error
  HiLink luaSpecial		SpecialChar
  HiLink luaFunc		Identifier

  delcommand HiLink
endif

let b:current_syntax = "lua"

" vim: et ts=8
