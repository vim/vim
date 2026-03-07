" Vim syntax file
" Language:	ed(1)
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2026 Mar 07

if exists("b:current_syntax")
  finish
endif
let s:cpo_save= &cpo
set cpo&vim

syn match  edLineStart
      \ /^/
      \ skipwhite
      \ nextgroup=@edAddress,edAddressSeparator,@edCommand

" Addresses {{{1

" TODO: Rename edAddress_Line, edAdress_Mark etc?
syn match   edAddress  contained
      \ /[.$]\|\d\+\|'[a-z]/
      \ skipwhite
      \ nextgroup=@edAddressModifier,edAddressSeparator,@edCommand
syn region  edAddress_Pattern  contained
      \ matchgroup=Delimiter
      \ start=+/+
      \ end=+/\|$+
      \ skipwhite nextgroup=@edAddressModifier,edAddressSeparator,@edCommand
      \ contains=edRegex_SlashEscape,edRegex_BracketExpression
syn region  edAddress_Pattern  contained
      \ matchgroup=Delimiter
      \ start=/?/
      \ end=/?\|$/
      \ skipwhite nextgroup=@edAddressModifier,edAddressSeparator,@edCommand
      \ contains=edRegex_QuestionEscape,edRegex_BracketExpression

syn match   edRegex_BracketExpression  contained
      \ "\[\^\=\]\=\%(\[:.\{-}:\]\|\[\..\{-}\.\]\|\[=.\{-}=\]\|[^]]\)*\]"
      \ transparent
syn match   edRegex_SlashEscape     contained +\\/+ transparent
syn match   edRegex_QuestionEscape  contained +\\?+ transparent

syn cluster edAddress
      \ contains=edAddress,edAddress_Pattern,edAddressModifier_Offset

syn match   edAddressModifier_Offset  contained
      \ /[+-]\s*\%(\d\+\)\=/
      \ skipwhite
      \ nextgroup=@edAddressModifier,edAddressSeparator,@edCommand
syn match   edAddressModifier_Count  contained
      \ /\d\+/
      \ skipwhite
      \ nextgroup=@edAddressModifier,edAddressSeparator,@edCommand
syn cluster edAddressModifier
      \ contains=edAddressModifier_Offset,edAddressModifier_Count

syn match   edAddressSeparator  contained
      \ /[,;]/
      \ skipwhite
      \ nextgroup=@edAddress,edAddressSeparator,@edCommand
" GNU extension
syn match   edAddressSeparator  contained
      \ /%/
      \ skipwhite
      \ nextgroup=@edAddress,edAddressSeparator,@edCommand

" Commands {{{1

" Append Command {{{2
syn match   edCommand_a  contained
      \ /a/
      \ skipnl
      \ nextgroup=edCommand_aci_Suffix,
      \           edCommand_aci_Arg_Text,
      \           edCommand_aci_Arg_TextEndMarker,
      \           edCommand_aci_LineContinue

syn match  edCommand_aci_Suffix  contained
      \ /[lnp]/
      \ skipnl
      \ nextgroup=edCommand_aci_Suffix,edCommand_aci_Arg_Text
hi def link edCommand_aci_Suffix Special

syn match  edCommand_aci_Arg_TextEndMarker  contained
      \ /^\.\ze\%(\s*\\\)\=$/
hi def link edCommand_aci_Arg_TextEndMarker edCommand

syn region  edCommand_aci_Arg_Text  contained
      \ start=/^\%(\.$\)\@!./
      "\ end=/^\.$/
      \ matchgroup=edCommand_aci_Arg_TextEndMarker
      "\ TODO: remove \\?
      \ end=/^\.\ze\%(\s*\\\)\=$/
      \ fold
hi def link edCommand_aci_Arg_Text String
syn region  edCommand_aci_Arg_Text_  contained
      \ start=/^\%(\.$\)\@!./
      \ end=/\ze\\\@1<!\n/
      \ matchgroup=edCommand_aci_Arg_TextEndMarker
      \ end=/^\.\ze\%(\s*\\\)\=$/
      \ skipwhite nextgroup=edLineContinue
      \ contains=edCommand_aci_Arg_TextLineContinue
      \ fold
hi def link edCommand_aci_Arg_Text_ String

" Change Command {{{2
syn match   edCommand_c  contained
      \ /c/
      \ skipnl
      \ nextgroup=edCommand_aci_Suffix,
      \           edCommand_aci_Arg_Text,
      \           edCommand_aci_Arg_TextEndMarker,
      \           edCommand_aci_LineContinue

" Delete Command {{{2
syn match   edCommand_d  contained
      \ /d\>/
      \ nextgroup=edCommandSuffix

" TODO: maybe implement as a region so that the command list is contained?
" Global Command {{{2
syn match   edCommand_g  contained
      \ /g/
      \ nextgroup=edCommand_g_Arg_Regexp
syn region  edCommand_g_Arg_Regexp  contained
      \ matchgroup=Delimiter
      \ start=/\z([^\\[:space:]\n]\)/
      \ end=/\z1\|$/
      "\ skipwhite
      "\ nextgroup=@edAddress,edAddressSeparator,@edCommand

syn match   edCommand_aci_LineContinue  /\\$/ contained skipnl nextgroup=edCommand_aci_Arg_Text_,edCommand_aci_Arg_TextEndMarker
hi def link edCommand_aci_LineContinue Delimiter
syn match   edCommand_aci_Arg_TextLineContinue  /\\$/ contained
hi def link edCommand_aci_Arg_TextLineContinue DiffAdd
" TODO: global only for now, separate needed for s///
syn match   edLineContinue  /\\$/ skipnl nextgroup=edLineStart
hi def link edLineContinue DiffChange

" Interactive Global Command {{{2
syn match   edCommand_G  contained
      \ /G/
      \ nextgroup=edCommand_G_Arg_Regexp
syn region  edCommand_G_Arg_Regexp  contained
      \ matchgroup=Delimiter
      \ start=/\z([^\\[:space:]\n]\)/
      \ end=/\z1\|$/

" Insert Command {{{2
syn match   edCommand_i  contained
      \ /i/
      \ skipnl
      \ nextgroup=edCommand_aci_Suffix,
      \           edCommand_aci_Arg_Text,
      \           edCommand_aci_Arg_TextEndMarker,
      \           edCommand_aci_LineContinue

" Join Command {{{2
syn match   edCommand_j  contained
      \ /j\>/
      \ nextgroup=edCommandSuffix

" Mark Command {{{2
syn match   edCommand_k  contained
      \ /k/
      \ nextgroup=edCommand_k_Arg_Mark,edCommandSuffix
syn match   edCommand_k_Arg_Mark  contained /[a-z]/

" List Command {{{2
syn match   edCommand_l  contained
      \ /l\>/
      \ nextgroup=edCommandSuffix

" TODO: skipwhite is GNU extension?
" Move Command {{{2
syn match   edCommand_m  contained
      \ /m/
      \ skipwhite
      \ nextgroup=@edAddress,edAddressSeparator,edCommandSuffix

" Number Command {{{2
syn match   edCommand_n  contained
      \ /n\>/
      \ nextgroup=edCommandSuffix

" Print Command {{{2
syn match   edCommand_p  contained
      \ /p\>/
      \ nextgroup=edCommandSuffix

" Read Command {{{2
syn match   edCommand_r  contained
      \ /r\>/
      \ nextgroup=edArg_File

" Substitute Command {{{2
syn match   edCommand_s  contained
      \ /s/
      \ nextgroup=edCommand_s_Arg_Regexp
syn region  edCommand_s_Arg_Regexp  contained
      \ matchgroup=Delimiter
      \ start=/\z([^\\[:space:]\n]\)/
      \ end=/\ze\z1/
      \ nextgroup=edCommand_s_Arg_Replacement
syn region  edCommand_s_Arg_Replacement  contained
      \ matchgroup=Delimiter
      \ start=/\z(.\)/
      \ skip=/\\\z1\|\\$/
      \ end=/\z1\|$/
      \ nextgroup=edCommand_s_Arg_Flag,edCommand_s_Arg_Count
      \ contains=edCommand_s_Arg_ReplacementEscape,
      \          edCommand_s_Arg_ReplacementContinue

syn match   edCommand_s_Arg_ReplacementEscape    contained /\\./
syn match   edCommand_s_Arg_ReplacementContinue  contained /\\$/

hi def link edCommand_s_Arg_ReplacementEscape Special
hi def link edCommand_s_Arg_ReplacementContinue Special

syn match   edCommand_s_Arg_Flag  contained
      \ /g/
      \ nextgroup=edCommand_s_Arg_Flag,edCommand_s_Arg_Count,edCommandSuffix
hi def link edCommand_s_Arg_Flag Special
" GNU extension
syn match   edCommand_s_Arg_Flag  contained
      \ /[iI]/
      \ nextgroup=edCommand_s_Arg_Flag,edCommand_s_Arg_Count,edCommandSuffix

syn match   edCommand_s_Arg_Count  contained
      \ /\d\+/
      \ nextgroup=edCommand_s_Arg_Flag,edCommandSuffix
hi def link edCommand_s_Arg_Count Special

" TODO: skipwhite is GNU extension?
" Copy Command {{{2
syn match   edCommand_t  contained
      \ /t/
      \ nextgroup=@edAddress,edAddressSeparator,edCommandSuffix

" TODO: maybe implement as a region so that the command list is  contained?
" Global Non-Matched Command {{{2
syn match   edCommand_v  contained
      \ /v/
      \ nextgroup=edCommand_v_Arg_Regexp
syn region  edCommand_v_Arg_Regexp  contained
      \ matchgroup=Delimiter
      \ start=/\z([^\\[:space:]\n]\)/
      \ end=/\z1\|$/
      \ skipwhite
      \ nextgroup=@edAddress,edAddressSeparator,@edCommand

" Interactive Global Non-Matched Command {{{2
syn match   edCommand_V  contained
      \ /V/
      \ nextgroup=edCommand_V_Arg_Regexp
syn region  edCommand_V_Arg_Regexp  contained
      \ matchgroup=Delimiter
      \ start=/\z([^\\[:space:]\n]\)/
      \ end=/\z1\|$/

" Write Command {{{2
syn match   edCommand_w  contained
      \ /w\>/
      \ nextgroup=edArg_File

" Write Append Command {{{2
" BSD/GNU extension
syn match   edCommand_W  contained
      \ /W\>/
      \ nextgroup=edArg_File

" Write Quit Command {{{2
" BSD/GNU extension
syn match   edCommand_wq  contained
      \ /wq\>/
      \ nextgroup=edArg_File

" Paste Cut Buffer Command {{{2
" GNU extension
syn match   edCommand_x  contained
      \ /x\>/

" Yank Cut Buffer Command {{{2
" GNU extension
syn match   edCommand_y  contained
      \ /y\>/

" Scroll Command {{{2
" BSD/GNU extension
syn match   edCommand_z  contained
      \ /z\>/
      \ nextgroup=edCommand_z_Arg_count,edCommandSuffix
syn match   edCommand_z_Arg_count  contained
      \ /\<\d\+\>/
      \ nextgroup=edCommandSuffix
hi def link edCommand_z_Arg_count Number

" Line Number Command {{{2
syn match   edCommand_equals  contained /=/
" }}}

" no address prefix commands
" TODO: use :syn-keyword?

" Edit Command {{{2
syn match   edCommand_e  contained /\<e\>/ skipwhite nextgroup=edArg_File

" Edit Without Checking Command {{{2
syn match   edCommand_E  contained /\<E\>/ skipwhite nextgroup=edArg_File

" Filename Command {{{2
syn match   edCommand_f  contained /\<f\>/ skipwhite nextgroup=edArg_File

" Help Command {{{2
syn match   edCommand_h  contained
      \ /\<h\>/
      \ nextgroup=edCommandSuffix

" Help-Mode Command {{{2
syn match   edCommand_H  contained
      \ /\<H\>/
      \ nextgroup=edCommandSuffix

" Prompt Command {{{2
syn match   edCommand_P  contained
      \ /\<P\>/
      \ nextgroup=edCommandSuffix

" Quit Command {{{2
syn match   edCommand_q  contained
      \ /\<q\>/

" Quit Without Checking Command {{{2
syn match   edCommand_Q  contained
      \ /\<Q\>/

" Undo Command {{{2
syn match   edCommand_u  contained
      \ /\<u\>/
      \ nextgroup=edCommandSuffix

" TODO: handle ! and % in arg
" Shell Escape Command {{{2
syn match   edCommand_exclamation  contained
      \ /!/
      \ skipwhite
      \ nextgroup=edCommand_exclamation_Arg_Command,edShellCommand_Previous
syn match   edCommand_exclamation_Arg_Command   contained
      \ /\S.*$/
syn match   edCommand_exclamation_Arg_Previous  contained
      \ /!/
      \ skipwhite
      \ nextgroup=edShellCommand

" Comment Command {{{2
" GNU extension
syn match   edCommand_hash  contained
      \ /#.*/
" }}}

syn cluster edCommand contains=edCommand_\a\+

" Command Args {{{2
syn match   edArg_File  contained /\S.*$/

" Command Suffixes {{{1
syn match  edCommandSuffix  contained /[lnp]/ nextgroup=edCommandSuffix
hi def link edCommandSuffix Special

" Default Highlighting {{{1

hi def link edAddress		         Constant
hi def link edAddressModifier_Offset	 Special
hi def link edAddressModifier_Count	 Special

hi def link edCommand		  Statement

hi def link edCommand_a		  Statement
hi def link edCommand_c		  Statement
hi def link edCommand_d	          Statement
hi def link edCommand_g	          Statement
hi def link edCommand_G	          Statement
hi def link edCommand_i	          Statement
hi def link edCommand_j	          Statement
hi def link edCommand_k	          Statement
hi def link edCommand_l	          Statement
hi def link edCommand_m	          Statement
hi def link edCommand_n	          Statement
hi def link edCommand_p	          Statement
hi def link edCommand_r	          Statement
hi def link edCommand_s           Statement
hi def link edCommand_t	          Statement
hi def link edCommand_v	          Statement
hi def link edCommand_V	          Statement
hi def link edCommand_w	          Statement
hi def link edCommand_wq          Statement
hi def link edCommand_x           Statement
hi def link edCommand_y           Statement
hi def link edCommand_z           Statement
hi def link edCommand_equals      Statement

" no addresses
hi def link edCommand_e	          Statement
hi def link edCommand_E	          Statement
hi def link edCommand_f	          Statement
hi def link edCommand_h	          Statement
hi def link edCommand_H	          Statement
hi def link edCommand_P	          Statement
hi def link edCommand_q	          Statement
hi def link edCommand_Q	          Statement
hi def link edCommand_u	          Statement
hi def link edCommand_exclamation Statement

hi def link edCommand_hash	  Comment

hi def link edCommand_k_Arg_Mark  Constant
" }}}

let b:current_syntax = "ed"

let &cpo = s:cpo_save
unlet! s:cpo_save

" vim: nowrap sw=2 sts=2 ts=8 noet fdm=marker:
