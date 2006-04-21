" Vim syntax file
" Language:         udev(8) rules file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword udevrulesTodo       contained TODO FIXME XXX NOTE

syn region  udevrulesComment    display oneline start='^\s*#' end='$'
                                \ contains=udevrulesTodo,@Spell

syn keyword udevrulesRuleKey    BUS KERNEL SUBSYSTEM DRIVER ID RESULT
                                \ nextgroup=udevrulesRuleEq

syn keyword udevrulesRuleKey    SYSFS nextgroup=udevrulesSysFSPath

syn region  udevrulesSysFSPath  display transparent
                                \ matchgroup=udevrulesDelimiter start='{'
                                \ matchgroup=udevrulesDelimiter end='}'
                                \ contains=udevrulesPath
                                \ nextgroup=udevrulesRuleEq

syn keyword udevrulesRuleKey    ENV nextgroup=udevrulesEnvVar

syn region  udevrulesEnvVar     display transparent
                                \ matchgroup=udevrulesDelimiter start='{'
                                \ matchgroup=udevrulesDelimiter end='}'
                                \ contains=udevrulesVariable
                                \ nextgroup=udevrulesRuleEq

syn keyword udevrulesRuleKey    PROGRAM
                                \ nextgroup=udevrulesEStringEq

syn keyword udevrulesAssignKey  NAME SYMLINK OWNER GROUP
                                \ nextgroup=udevrulesEStringEq

syn keyword udevrulesAssignKey  MODE
                                \ nextgroup=udevrulesRuleEq

syn keyword udevrulesAssignKey  OPTIONS
                                \ nextgroup=udevrulesOptionsEq

syn match   udevrulesPath       contained display '[^}]\+'

syn match   udevrulesVariable   contained display '[^}]\+'

syn match   udevrulesRuleEq     contained '[[:space:]=]'
                                \ nextgroup=udevrulesString skipwhite

syn match   udevrulesEStringEq  contained '[[:space:]=]'
                                \ nextgroup=udevrulesEString skipwhite

syn match   udevrulesOptionsEq  contained '[[:space:]=]'
                                \ nextgroup=udevrulesOptions skipwhite

syn region  udevrulesEString    contained display oneline start=+"+ end=+"+
                                \ contains=udevrulesStrEscapes

syn match   udevrulesStrEscapes contained '%[nkpMmbcNPe%]'

syn region  udevrulesStrEscapes contained start='%c{' end='}'
                                \ contains=udevrulesStrNumber

syn region  udevrulesStrEscapes contained start='%s{' end='}'
                                \ contains=udevrulesPath

syn match   udevrulesStrNumber  contained '\d\++\='

syn region  udevrulesString     contained display oneline start=+"+ end=+"+
                                \ contains=udevrulesPattern

syn match   udevrulesPattern    contained '[*?]'
syn region  udevrulesPattern    contained start='\[!\=' end='\]'
                                \ contains=udevrulesPatRange

syn match   udevrulesPatRange   contained '[^[-]-[^]-]'

syn region  udevrulesOptions    contained display oneline start=+"+ end=+"+
                                \ contains=udevrulesOption,udevrulesOptionSep

syn keyword udevrulesOption     contained last_rule ignore_device ignore_remove
                                \ all_partitions

syn match   udevrulesOptionSep  contained ','

hi def link udevrulesTodo       Todo
hi def link udevrulesComment    Comment
hi def link udevrulesRuleKey    Keyword
hi def link udevrulesDelimiter  Delimiter
hi def link udevrulesAssignKey  Identifier
hi def link udevrulesPath       Identifier
hi def link udevrulesVariable   Identifier
" XXX: setting this to Operator makes for extremely intense highlighting.
hi def link udevrulesEq         Normal
hi def link udevrulesRuleEq     udevrulesEq
hi def link udevrulesEStringEq  udevrulesEq
hi def link udevrulesOptionsEq  udevrulesEq
hi def link udevrulesEString    udevrulesString
hi def link udevrulesStrEscapes SpecialChar
hi def link udevrulesStrNumber  Number
hi def link udevrulesString     String
hi def link udevrulesPattern    SpecialChar
hi def link udevrulesPatRange   SpecialChar
hi def link udevrulesOptions    udevrulesString
hi def link udevrulesOption     Type
hi def link udevrulesOptionSep  Delimiter

let b:current_syntax = "udevrules"

let &cpo = s:cpo_save
unlet s:cpo_save
