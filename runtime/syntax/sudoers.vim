" Vim syntax file
" Language:             sudoers(5) configuration files
" Maintainer:           Eisuke Kawashima ( e.kawaschima+vim AT gmail.com )
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2024 Sep 02
" Recent Changes:	Support for #include and #includedir.
" 2018 Aug 28 by Vim project Added many new options (Samuel D. Leslie)
" 2024 Sep 09 by Vim project Update allowed Tag_Spec Runas_Spec syntax items
" 2026 Feb 13 by Vim project update regex for matching usernames #19396

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

" TODO: instead of 'skipnl', we would like to match a specific group that would
" match \\$ and then continue with the nextgroup, actually, the skipnl doesn't
" work...
" TODO: treat 'ALL' like a special (yay, a bundle of new rules!!!)

syn match   sudoersUserSpec '^' nextgroup=@sudoersUserInSpec skipwhite

syn match   sudoersSpecEquals         contained '=' nextgroup=@sudoersCmndSpecList skipwhite

syn cluster sudoersCmndSpecList       contains=sudoersUserRunasBegin,sudoersTagSpec,@sudoersCmndInSpec

syn keyword sudoersTodo               contained TODO FIXME XXX NOTE

syn region  sudoersComment            display oneline start='#' end='$' contains=sudoersTodo
syn region  sudoersInclude            display oneline start='[#@]\%(include\|includedir\)\s\+\S\+' end='$'

syn keyword sudoersAlias              User_Alias Runas_Alias nextgroup=sudoersUserAlias skipwhite skipnl
syn keyword sudoersAlias              Host_Alias nextgroup=sudoersHostAlias skipwhite skipnl
syn keyword sudoersAlias              Cmnd_Alias nextgroup=sudoersCmndAlias skipwhite skipnl

syn match   sudoersUserAlias          contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersUserAliasEquals  skipwhite skipnl
syn match   sudoersUserNameInList     contained '\<\l[-a-z0-9_]*\>' nextgroup=@sudoersUserList        skipwhite skipnl
syn match   sudoersUIDInList          contained '#\d\+\>'           nextgroup=@sudoersUserList        skipwhite skipnl
syn match   sudoersGroupInList        contained '%\l[-a-z0-9_]*\>'  nextgroup=@sudoersUserList        skipwhite skipnl
syn match   sudoersGIDInList          contained '%#\d\+\>'          nextgroup=@sudoersUserList        skipwhite skipnl
syn match   sudoersUserNetgroupInList contained '+\l[-a-z0-9_]*\>'  nextgroup=@sudoersUserList        skipwhite skipnl
syn match   sudoersUserAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserList        skipwhite skipnl

syn match   sudoersUserName           contained '\<\l[-a-z0-9_]*\>' nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersUID                contained '#\d\+\>'           nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersGroup              contained '%\l[-a-z0-9_]*\>'  nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersGID                contained '%#\d\+\>'          nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersUserNetgroup       contained '+\l[-a-z0-9_]*\>'  nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersUserAliasRef       contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersParameter       skipwhite skipnl

syn match   sudoersUserNameInSpec     contained '\<\l[-a-z0-9_]*\>' nextgroup=@sudoersUserSpec        skipwhite skipnl
syn region  sudoersUIDInSpec          display oneline start='#\d\+\>' end='' nextgroup=@sudoersUserSpec skipwhite skipnl
syn match   sudoersGroupInSpec        contained '%\l[-a-z0-9_]*\>'  nextgroup=@sudoersUserSpec        skipwhite skipnl
syn match   sudoersGIDInSpec          contained '%#\d\+\>'          nextgroup=@sudoersUserSpec        skipwhite skipnl
syn match   sudoersUserNetgroupInSpec contained '+\l[-a-z0-9_]*\>'  nextgroup=@sudoersUserSpec        skipwhite skipnl
syn match   sudoersUserAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserSpec        skipwhite skipnl

syn match   sudoersUserNameInRunas    contained '\<\l[-a-z0-9_]*\>' nextgroup=@sudoersUserRunas       skipwhite skipnl
syn match   sudoersUIDInRunas         contained '#\d\+\>'           nextgroup=@sudoersUserRunas       skipwhite skipnl
syn match   sudoersGroupInRunas       contained '%\l[-a-z0-9_]*\>'  nextgroup=@sudoersUserRunas       skipwhite skipnl
syn match   sudoersGIDInRunas         contained '%#\d\+\>'          nextgroup=@sudoersUserRunas       skipwhite skipnl
syn match   sudoersUserNetgroupInRunas contained '+\l[-a-z0-9_]*\>' nextgroup=@sudoersUserRunas       skipwhite skipnl
syn match   sudoersUserAliasInRunas   contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserRunas       skipwhite skipnl

syn match   sudoersHostAlias          contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersHostAliasEquals  skipwhite skipnl
syn match   sudoersHostNameInList     contained '\<\l[a-z0-9_-]*\>' nextgroup=@sudoersHostList        skipwhite skipnl
syn match   sudoersIPAddrInList       contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}' nextgroup=@sudoersHostList skipwhite skipnl
syn match   sudoersNetworkInList      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}\%(/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\)\=' nextgroup=@sudoersHostList skipwhite skipnl
syn match   sudoersHostNetgroupInList contained '+\l\+\>'           nextgroup=@sudoersHostList        skipwhite skipnl
syn match   sudoersHostAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersHostList        skipwhite skipnl

syn match   sudoersHostName           contained '\<\l[a-z0-9_-]*\>'           nextgroup=@sudoersParameter skipwhite skipnl
syn match   sudoersIPAddr             contained '\<\%(\d\{1,3}\.\)\{3}\d\{1,3}\>' nextgroup=@sudoersParameter skipwhite skipnl
syn match   sudoersNetwork            contained '\<\%(\d\{1,3}\.\)\{3}\d\{1,3}/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\>' nextgroup=@sudoersParameter skipwhite skipnl
syn match   sudoersHostNetgroup       contained '+\l\+\>'           nextgroup=@sudoersParameter       skipwhite skipnl
syn match   sudoersHostAliasRef       contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersParameter       skipwhite skipnl
syn keyword sudoersHostAll            contained ALL                 nextgroup=@sudoersParameter       skipwhite skipnl

syn match   sudoersHostNameInSpec     contained '\<\l[a-z0-9_-]*\>' nextgroup=@sudoersHostSpec        skipwhite skipnl
syn match   sudoersIPAddrInSpec       contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}' nextgroup=@sudoersHostSpec skipwhite skipnl
syn match   sudoersNetworkInSpec      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}\%(/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\)\=' nextgroup=@sudoersHostSpec skipwhite skipnl
syn match   sudoersHostNetgroupInSpec contained '+\l\+\>'           nextgroup=@sudoersHostSpec        skipwhite skipnl
syn match   sudoersHostAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersHostSpec        skipwhite skipnl

syn match   sudoersCmndAlias          contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersCmndAliasEquals  skipwhite skipnl
syn match   sudoersCmndNameInList     contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=@sudoersCmndList,sudoersCommandEmpty,sudoersCommandArgs skipwhite
syn match   sudoersCmndAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersCmndList        skipwhite skipnl

syn match   sudoersCmndNameInSpec     contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=@sudoersCmndSpec,sudoersCommandEmptyInSpec,sudoersCommandArgsInSpec skipwhite
syn match   sudoersCmndAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersCmndSpec        skipwhite skipnl

syn match   sudoersUserAliasEquals  contained '=' nextgroup=@sudoersUserInList  skipwhite skipnl
syn match   sudoersUserListComma    contained ',' nextgroup=@sudoersUserInList  skipwhite skipnl
syn match   sudoersUserListColon    contained ':' nextgroup=sudoersUserAlias    skipwhite skipnl
syn cluster sudoersUserList         contains=sudoersUserListComma,sudoersUserListColon

syn match   sudoersUserSpecComma    contained ',' nextgroup=@sudoersUserInSpec  skipwhite skipnl
syn cluster sudoersUserSpec         contains=sudoersUserSpecComma,@sudoersHostInSpec

syn match   sudoersUserRunasBegin   contained '(' nextgroup=@sudoersUserInRunas,sudoersUserRunasColon skipwhite skipnl
syn match   sudoersUserRunasComma   contained ',' nextgroup=@sudoersUserInRunas skipwhite skipnl
syn match   sudoersUserRunasColon   contained ':' nextgroup=@sudoersUserInRunas skipwhite skipnl
syn match   sudoersUserRunasEnd     contained ')' nextgroup=sudoersTagSpec,@sudoersCmndInSpec skipwhite skipnl
syn cluster sudoersUserRunas        contains=sudoersUserRunasComma,sudoersUserRunasColon,@sudoersUserInRunas,sudoersUserRunasEnd


syn match   sudoersHostAliasEquals  contained '=' nextgroup=@sudoersHostInList  skipwhite skipnl
syn match   sudoersHostListComma    contained ',' nextgroup=@sudoersHostInList  skipwhite skipnl
syn match   sudoersHostListColon    contained ':' nextgroup=sudoersHostAlias    skipwhite skipnl
syn cluster sudoersHostList         contains=sudoersHostListComma,sudoersHostListColon

syn match   sudoersHostSpecComma    contained ',' nextgroup=@sudoersHostInSpec  skipwhite skipnl
syn cluster sudoersHostSpec         contains=sudoersHostSpecComma,sudoersSpecEquals


syn match   sudoersCmndAliasEquals  contained '=' nextgroup=@sudoersCmndInList  skipwhite skipnl
syn match   sudoersCmndListComma    contained ',' nextgroup=@sudoersCmndInList  skipwhite skipnl
syn match   sudoersCmndListColon    contained ':' nextgroup=sudoersCmndAlias    skipwhite skipnl
syn cluster sudoersCmndList         contains=sudoersCmndListComma,sudoersCmndListColon

syn match   sudoersCmndSpecComma    contained ',' nextgroup=@sudoersCmndSpecList skipwhite skipnl
syn match   sudoersCmndSpecColon    contained ':' nextgroup=@sudoersUserInSpec  skipwhite skipnl
syn cluster sudoersCmndSpec         contains=sudoersCmndSpecComma,sudoersCmndSpecColon

syn cluster sudoersUserInList       contains=sudoersUserNegationInList,sudoersUserNameInList,sudoersUIDInList,sudoersGroupInList,sudoersGIDInList,sudoersUserNetgroupInList,sudoersUserAliasInList
syn cluster sudoersHostInList       contains=sudoersHostNegationInList,sudoersHostNameInList,sudoersIPAddrInList,sudoersNetworkInList,sudoersHostNetgroupInList,sudoersHostAliasInList
syn cluster sudoersCmndInList       contains=sudoersCmndNegationInList,sudoersCmndNameInList,sudoersCmndAliasInList

syn cluster sudoersUser             contains=sudoersUserNegation,sudoersUserName,sudoersUID,sudoersGroup,sudoersGID,sudoersUserNetgroup,sudoersUserAliasRef
syn cluster sudoersHost             contains=sudoersHostNegation,sudoersHostName,sudoersIPAddr,sudoersNetwork,sudoersHostNetgroup,sudoersHostAll,sudoersHostAliasRef

syn cluster sudoersUserInSpec       contains=sudoersUserNegationInSpec,sudoersUserNameInSpec,sudoersUIDInSpec,sudoersGroupInSpec,sudoersGIDInSpec,sudoersUserNetgroupInSpec,sudoersUserAliasInSpec
syn cluster sudoersHostInSpec       contains=sudoersHostNegationInSpec,sudoersHostNameInSpec,sudoersIPAddrInSpec,sudoersNetworkInSpec,sudoersHostNetgroupInSpec,sudoersHostAliasInSpec
syn cluster sudoersUserInRunas      contains=sudoersUserNegationInRunas,sudoersUserNameInRunas,sudoersUIDInRunas,sudoersGroupInRunas,sudoersGIDInRunas,sudoersUserNetgroupInRunas,sudoersUserAliasInRunas
syn cluster sudoersCmndInSpec       contains=sudoersCmndNegationInSpec,sudoersCmndNameInSpec,sudoersCmndAliasInSpec

syn match   sudoersUserNegationInList contained '!\+' nextgroup=@sudoersUserInList  skipwhite skipnl
syn match   sudoersHostNegationInList contained '!\+' nextgroup=@sudoersHostInList  skipwhite skipnl
syn match   sudoersCmndNegationInList contained '!\+' nextgroup=@sudoersCmndInList  skipwhite skipnl

syn match   sudoersUserNegation       contained '!\+' nextgroup=@sudoersUser        skipwhite skipnl
syn match   sudoersHostNegation       contained '!\+' nextgroup=@sudoersHost        skipwhite skipnl

syn match   sudoersUserNegationInSpec contained '!\+' nextgroup=@sudoersUserInSpec  skipwhite skipnl
syn match   sudoersHostNegationInSpec contained '!\+' nextgroup=@sudoersHostInSpec  skipwhite skipnl
syn match   sudoersUserNegationInRunas contained '!\+' nextgroup=@sudoersUserInRunas skipwhite skipnl
syn match   sudoersCmndNegationInSpec contained '!\+' nextgroup=@sudoersCmndInSpec  skipwhite skipnl

syn match   sudoersCommandArgs      contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersCommandArgs,@sudoersCmndList skipwhite
syn match   sudoersCommandEmpty     contained '""' nextgroup=@sudoersCmndList skipwhite skipnl

syn match   sudoersCommandArgsInSpec contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersCommandArgsInSpec,@sudoersCmndSpec skipwhite
syn match   sudoersCommandEmptyInSpec contained '""' nextgroup=@sudoersCmndSpec skipwhite skipnl

syn keyword sudoersDefaultEntry Defaults nextgroup=sudoersDefaultTypeAt,sudoersDefaultTypeColon,sudoersDefaultTypeGreaterThan,@sudoersParameter skipwhite skipnl
syn match   sudoersDefaultTypeAt          contained '@' nextgroup=@sudoersHost skipwhite skipnl
syn match   sudoersDefaultTypeColon       contained ':' nextgroup=@sudoersUser skipwhite skipnl
syn match   sudoersDefaultTypeGreaterThan contained '>' nextgroup=@sudoersUser skipwhite skipnl

" TODO: could also deal with special characters here
syn match   sudoersBooleanParameter contained '!' nextgroup=sudoersBooleanParameter,sudoersIntegerOrBooleanParameter,sudoersModeOrBooleanParameter,sudoersFloatOrBooleanParameter,sudoersTimeoutOrBooleanParameter,sudoersStringOrBooleanParameter skipwhite skipnl
syn keyword sudoersBooleanParameter contained skipwhite skipnl
                                  \ always_query_group_plugin
                                  \ always_set_home
                                  \ authenticate
                                  \ case_insensitive_group
                                  \ case_insensitive_user
                                  \ closefrom_override
                                  \ compress_io
                                  \ env_editor
                                  \ env_reset
                                  \ exec_background
                                  \ fast_glob
                                  \ fqdn
                                  \ ignore_audit_errors
                                  \ ignore_dot
                                  \ ignore_iolog_errors
                                  \ ignore_local_sudoers
                                  \ ignore_logfile_errors
                                  \ ignore_unknown_defaults
                                  \ insults
                                  \ intercept
                                  \ intercept_allow_setid
                                  \ intercept_authenticate
                                  \ intercept_verify
                                  \ iolog_flush
                                  \ log_allowed
                                  \ log_denied
                                  \ log_exit_status
                                  \ log_host
                                  \ log_input
                                  \ log_output
                                  \ log_passwords
                                  \ log_server_keepalive
                                  \ log_server_verify
                                  \ log_stderr
                                  \ log_stdin
                                  \ log_stdout
                                  \ log_subcmds
                                  \ log_ttyin
                                  \ log_ttyout
                                  \ log_year
                                  \ long_otp_prompt
                                  \ mail_all_cmnds
                                  \ mail_always
                                  \ mail_badpass
                                  \ mail_no_host
                                  \ mail_no_perms
                                  \ mail_no_user
                                  \ match_group_by_gid
                                  \ netgroup_tuple
                                  \ noexec
                                  \ noninteractive_auth
                                  \ pam_acct_mgmt
                                  \ pam_rhost
                                  \ pam_ruser
                                  \ pam_session
                                  \ pam_setcred
                                  \ pam_silent
                                  \ passprompt_override
                                  \ path_info
                                  \ preserve_groups
                                  \ pwfeedback
                                  \ requiretty
                                  \ root_sudo
                                  \ rootpw
                                  \ runas_allow_unknown_id
                                  \ runas_check_shell
                                  \ runaspw
                                  \ selinux
                                  \ set_home
                                  \ set_logname
                                  \ set_utmp
                                  \ setenv
                                  \ shell_noargs
                                  \ stay_setuid
                                  \ sudoedit_checkdir
                                  \ sudoedit_follow
                                  \ syslog_pid
                                  \ targetpw
                                  \ tty_tickets
                                  \ umask_override
                                  \ use_loginclass
                                  \ use_netgroups
                                  \ use_pty
                                  \ user_command_timeouts
                                  \ utmp_runas
                                  \ visiblepw

syn keyword sudoersIntegerParameter contained
                                  \ nextgroup=sudoersIntegerParameterEquals
                                  \ skipwhite skipnl
                                  \ closefrom
                                  \ maxseq
                                  \ passwd_tries
                                  \ syslog_maxlen

syn keyword sudoersIntegerOrBooleanParameter contained
                                  \ nextgroup=sudoersIntegerParameterEquals
                                  \ skipwhite skipnl
                                  \ loglinelen

syn keyword sudoersFloatOrBooleanParameter contained
                                  \ nextgroup=sudoersFloatParameterEquals
                                  \ skipwhite skipnl
                                  \ passwd_timeout
                                  \ timestamp_timeout

syn keyword sudoersModeParameter contained
                                  \ nextgroup=sudoersModeParameterEquals
                                  \ skipwhite skipnl
                                  \ iolog_mode

syn keyword sudoersModeOrBooleanParameter contained
                                  \ nextgroup=sudoersModeParameterEquals
                                  \ skipwhite skipnl
                                  \ umask

syn keyword sudoersTimeoutOrBooleanParameter contained
                                  \ nextgroup=sudoersTimeoutParameterEquals
                                  \ skipwhite skipnl
                                  \ command_timeout
                                  \ log_server_timeout

syn keyword sudoersStringParameter  contained
                                  \ nextgroup=sudoersStringParameterEquals
                                  \ skipwhite skipnl
                                  \ apparmor_profile
                                  \ askpass
                                  \ authfail_message
                                  \ badpass_message
                                  \ cmddenial_message
                                  \ group_plugin
                                  \ intercept_type
                                  \ iolog_file
                                  \ limitprivs
                                  \ log_format
                                  \ mailsub
                                  \ noexec_file
                                  \ pam_askpass_service
                                  \ pam_login_service
                                  \ pam_service
                                  \ passprompt
                                  \ privs
                                  \ role
                                  \ runas_default
                                  \ sudoers_locale
                                  \ timestamp_type
                                  \ timestampowner
                                  \ type

syn keyword sudoersStringOrBooleanParameter contained
                                  \ nextgroup=sudoersStringParameterEquals
                                  \ skipwhite skipnl
                                  \ admin_flag
                                  \ editor
                                  \ env_file
                                  \ exempt_group
                                  \ fdexec
                                  \ iolog_dir
                                  \ iolog_group
                                  \ iolog_user
                                  \ lecture
                                  \ lecture_file
                                  \ lecture_status_dir
                                  \ listpw
                                  \ log_server_cabundle
                                  \ log_server_peer_cert
                                  \ log_server_peer_key
                                  \ logfile
                                  \ mailerflags
                                  \ mailerpath
                                  \ mailfrom
                                  \ mailto
                                  \ restricted_env_file
                                  \ rlimit_as
                                  \ rlimit_core
                                  \ rlimit_cpu
                                  \ rlimit_data
                                  \ rlimit_fsize
                                  \ rlimit_locks
                                  \ rlimit_memlock
                                  \ rlimit_nofile
                                  \ rlimit_nproc
                                  \ rlimit_rss
                                  \ rlimit_stack
                                  \ runcwd
                                  \ secure_path
                                  \ syslog
                                  \ syslog_badpri
                                  \ syslog_goodpri
                                  \ timestampdir
                                  \ verifypw

syn keyword sudoersListParameter    contained
                                  \ nextgroup=sudoersListParameterEquals
                                  \ skipwhite skipnl
                                  \ env_check
                                  \ env_delete
                                  \ env_keep
                                  \ log_servers
                                  \ passprompt_regex

syn match   sudoersParameterListComma contained ',' nextgroup=@sudoersParameter skipwhite skipnl

syn cluster sudoersParameter        contains=sudoersBooleanParameter,sudoersIntegerParameter,sudoersIntegerOrBooleanParameter,sudoersModeParameter,sudoersModeOrBooleanParameter,sudoersFloatOrBooleanParameter,sudoersTimeoutOrBooleanParameter,sudoersStringParameter,sudoersStringOrBooleanParameter,sudoersListParameter

syn match   sudoersIntegerParameterEquals contained       '=' nextgroup=sudoersIntegerValue skipwhite skipnl
syn match   sudoersModeParameterEquals    contained       '=' nextgroup=sudoersModeValue    skipwhite skipnl
syn match   sudoersFloatParameterEquals   contained       '=' nextgroup=sudoersFloatValue   skipwhite skipnl
syn match   sudoersTimeoutParameterEquals contained       '=' nextgroup=sudoersTimeoutValue skipwhite skipnl
syn match   sudoersStringParameterEquals  contained       '=' nextgroup=sudoersStringValue  skipwhite skipnl
syn match   sudoersListParameterEquals    contained '[+-]\==' nextgroup=sudoersListValue    skipwhite skipnl

syn match   sudoersIntegerValue contained '\<\d\+\>' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersModeValue    contained '\<\o\+\>' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersFloatValue   contained '-\?\%(\<\d\+\>\|\<\d\+\%(\.\%(\d\+\>\)\?\)\?\|\.\d\+\>\)' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersTimeoutValue contained '\<\d\+\>' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersTimeoutValue contained '\<\%(\d\+[dDhHmMsS]\)\+\>' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersStringValue  contained '\s*\zs[^[:space:],:=\\]*\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersParameterListComma skipwhite skipnl
syn region  sudoersStringValue  contained start=+\s*\zs"+ skip=+\\"+ end=+"+ nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersListValue    contained '\s*\zs[^[:space:],:=\\]*\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersParameterListComma skipwhite skipnl
syn region  sudoersListValue    contained start=+\s*\zs"+ skip=+\\"+ end=+"+ nextgroup=sudoersParameterListComma skipwhite skipnl

syn match   sudoersTagSpec      contained '\%(NO\)\=\%(EXEC\|FOLLOW\|LOG_\%(INPUT\|OUTPUT\)\|MAIL\|INTERCEPT\|PASSWD\|SETENV\):' nextgroup=sudoersTagSpec,@sudoersCmndInSpec skipwhite

hi def link sudoersSpecEquals               Operator
hi def link sudoersTodo                     Todo
hi def link sudoersComment                  Comment
hi def link sudoersAlias                    Keyword
hi def link sudoersUserAlias                Identifier
hi def link sudoersUserNameInList           String
hi def link sudoersUIDInList                Number
hi def link sudoersGroupInList              PreProc
hi def link sudoersGIDInList                Number
hi def link sudoersUserNetgroupInList       PreProc
hi def link sudoersUserAliasInList          PreProc
hi def link sudoersUserName                 String
hi def link sudoersUID                      Number
hi def link sudoersGroup                    PreProc
hi def link sudoersGID                      Number
hi def link sudoersUserNetgroup             PreProc
hi def link sudoersUserAliasRef             PreProc
hi def link sudoersUserNameInSpec           String
hi def link sudoersUIDInSpec                Number
hi def link sudoersGroupInSpec              PreProc
hi def link sudoersGIDInSpec                Number
hi def link sudoersUserNetgroupInSpec       PreProc
hi def link sudoersUserAliasInSpec          PreProc
hi def link sudoersUserNameInRunas          String
hi def link sudoersUIDInRunas               Number
hi def link sudoersGroupInRunas             PreProc
hi def link sudoersGIDInRunas               Number
hi def link sudoersUserNetgroupInRunas      PreProc
hi def link sudoersUserAliasInRunas         PreProc
hi def link sudoersHostAlias                Identifier
hi def link sudoersHostNameInList           String
hi def link sudoersIPAddrInList             Number
hi def link sudoersNetworkInList            Number
hi def link sudoersHostNetgroupInList       PreProc
hi def link sudoersHostAliasInList          PreProc
hi def link sudoersHostName                 String
hi def link sudoersIPAddr                   Number
hi def link sudoersNetwork                  Number
hi def link sudoersHostNetgroup             PreProc
hi def link sudoersHostAll                  Special
hi def link sudoersHostAliasRef             PreProc
hi def link sudoersHostNameInSpec           String
hi def link sudoersIPAddrInSpec             Number
hi def link sudoersNetworkInSpec            Number
hi def link sudoersHostNetgroupInSpec       PreProc
hi def link sudoersHostAliasInSpec          PreProc
hi def link sudoersCmndAlias                Identifier
hi def link sudoersCmndNameInList           String
hi def link sudoersCmndAliasInList          PreProc
hi def link sudoersCmndNameInSpec           String
hi def link sudoersCmndAliasInSpec          PreProc
hi def link sudoersUserAliasEquals          Operator
hi def link sudoersUserListComma            Delimiter
hi def link sudoersUserListColon            Delimiter
hi def link sudoersUserSpecComma            Delimiter
hi def link sudoersUserRunasBegin           Delimiter
hi def link sudoersUserRunasComma           Delimiter
hi def link sudoersUserRunasColon           Delimiter
hi def link sudoersUserRunasEnd             Delimiter
hi def link sudoersHostAliasEquals          Operator
hi def link sudoersHostListComma            Delimiter
hi def link sudoersHostListColon            Delimiter
hi def link sudoersHostSpecComma            Delimiter
hi def link sudoersCmndAliasEquals          Operator
hi def link sudoersCmndListComma            Delimiter
hi def link sudoersCmndListColon            Delimiter
hi def link sudoersCmndSpecComma            Delimiter
hi def link sudoersCmndSpecColon            Delimiter
hi def link sudoersUserNegationInList       Operator
hi def link sudoersHostNegationInList       Operator
hi def link sudoersCmndNegationInList       Operator
hi def link sudoersUserNegation             Operator
hi def link sudoersHostNegation             Operator
hi def link sudoersUserNegationInSpec       Operator
hi def link sudoersHostNegationInSpec       Operator
hi def link sudoersUserNegationInRunas      Operator
hi def link sudoersCmndNegationInSpec       Operator
hi def link sudoersCommandArgs              String
hi def link sudoersCommandEmpty             Special
hi def link sudoersDefaultEntry             Keyword
hi def link sudoersDefaultTypeAt            Special
hi def link sudoersDefaultTypeColon         Special
hi def link sudoersDefaultTypeGreaterThan   Special
hi def link sudoersBooleanParameter         Identifier
hi def link sudoersIntegerParameter         Identifier
hi def link sudoersIntegerOrBooleanParameter Identifier
hi def link sudoersModeParameter            Identifier
hi def link sudoersModeOrBooleanParameter   Identifier
hi def link sudoersFloatOrBooleanParameter  Identifier
hi def link sudoersTimeoutOrBooleanParameter Identifier
hi def link sudoersStringParameter          Identifier
hi def link sudoersStringOrBooleanParameter Identifier
hi def link sudoersListParameter            Identifier
hi def link sudoersParameterListComma       Delimiter
hi def link sudoersIntegerParameterEquals   Operator
hi def link sudoersModeParameterEquals      Operator
hi def link sudoersFloatParameterEquals     Operator
hi def link sudoersTimeoutParameterEquals   Operator
hi def link sudoersStringParameterEquals    Operator
hi def link sudoersListParameterEquals      Operator
hi def link sudoersIntegerValue             Number
hi def link sudoersModeValue                Number
hi def link sudoersFloatValue               Float
hi def link sudoersTimeoutValue             Number
hi def link sudoersStringValue              String
hi def link sudoersListValue                String
hi def link sudoersTagSpec                  Special
hi def link sudoersInclude                  Statement

let b:current_syntax = "sudoers"

let &cpo = s:cpo_save
unlet s:cpo_save
