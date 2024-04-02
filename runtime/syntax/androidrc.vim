" Vim syntax file
" Language: Android Init Language
" Maintainer: Chris McClellan <chris.mcclellan203@gmail.com>

if exists("b:current_syntax")
  finish
endif

syn match rcComment '#.*'
syn keyword rcImport import
syn keyword rcKeyword on
                    \ service

"Options are service modifiers
syn keyword rcOption
                    \ critical
                    \ disabled
                    \ setenv
                    \ socket
                    \ group
                    \ capabilities
                    \ setrlimit
                    \ seclabel
                    \ oneshot
                    \ onrestart
                    \ writepid
                    \ priority
                    \ namespace
                    \ oom_score_adjust
                    \ shutdown

" THese are common in file paths and as parts of other names
syn match rcOption /\sclass\s/
syn match rcOption /\sconsole[\s\n]/
syn match rcOption /\sfile\s/
syn match rcOption /\suser\s/

" These can't be processed as a keyword
"no args, so can be end of line
syn match rcOption /\sanimation class[\s\n]/
"these have value args, so can't be end of line
syn match rcOption /\smemcg.swappiness\s/
syn match rcOption /\smemcg.soft_limit_in_bytes\s/
syn match rcOption /\smemcg.limit_in_bytes\s/

" discovered by grepping init.rc for the word trigger
" keywords struggle with non-alph characters, so we match instead
syn match rcTrigger /\scharger[\s\n]/
syn match rcTrigger /\sinit[\s\n]/
syn match rcTrigger /\searly-init[\s\n]/
syn match rcTrigger /\slate-init[\s\n]/
syn match rcTrigger /\searly-fs[\s\n]/
syn match rcTrigger /\spost-fs-data[\s\n]/
syn match rcTrigger /\spost-fs[\s\n]/
syn match rcTrigger /\slate-fs[\s\n]/
syn match rcTrigger /\sfs[\s\n]/
syn match rcTrigger /\szygote-start[\s\n]/
syn match rcTrigger /\sload_persist_props_action[\s\n]/
syn match rcTrigger /\sfirmware_mounts_complete[\s\n]/
syn match rcTrigger /\searly-boot[\s\n]/
syn match rcTrigger /\sboot[\s\n]/

"Commands are... commands... that can be executed in a trigger
" This is a naive implementation. It would be better to make a trigger a region and only activate these inside.
syn keyword rcCommand
                    \ bootchart
                    \ chmod
                    \ chown
                    \ class_start
                    \ class_stop
                    \ class_reset
                    \ class_restart
                    \ copy
                    \ domainname
                    \ exec
                    \ exec_background
                    \ exec_start
                    \ export
                    \ hostname
                    \ ifup
                    \ insmod
                    \ load_all_props
                    \ load_persist_props
                    \ loglevel
                    \ mkdir
                    \ mount_all
                    \ mount
                    \ restart
                    \ restorecon
                    \ restorecon_recursive
                    \ rm
                    \ rmdir
                    \ readahead
                    \ setprop
                    \ setrlimit
                    \ stop
                    \ swapon_all
                    \ symlink
                    \ sysclktz
                    \ umount
                    \ verity_load_state
                    \ verity_update_state
                    \ wait
                    \ wait_for_prop
                    \ installkey
                    \ init_user0

" These are common in file paths and as parts of other names
syn match rcCommand /\strigger\s/
syn match rcCommand /\senable[\s\n]/
syn match rcCommand /\sstart\s/
syn match rcCommand /\swrite\s/

" Order matters here.
" We must declatre the identifier region after the operators in order to get the identifier highlight.
" The matchgroup allows the operators to not be over-riden by the region while still allowing the end match.
syn match rcInterpolator '${'
syn match rcInterpolator '}'
syn region rcInterpolate matchgroup=rcInterpolator start='${' end='}' contains=rcInterpolator

syn match rcPropertyTrigger '='
syn match rcPropertyTrigger 'property:'
syn region rcProperty matchgroup=rcPropertyTrigger start='property:' end='=' contains=rcPropertyTrigger

syn match rcOperator '&&'
syn match rcNumber /[-]\?\d\+/


hi def link rcComment Comment
hi def link rcImport Include
hi def link rcKeyword Statement
hi def link rcOption Statement
hi def link rcCommand Keyword
hi def link rcTrigger Type

hi def link rcInterpolator Operator
hi def link rcInterpolate Identifier
hi def link rcPropertyTrigger Operator
hi def link rcProperty Identifier

hi def link rcOperator Operator
hi def link rcNumber Number

let b:current_syntax = "androidrc"
