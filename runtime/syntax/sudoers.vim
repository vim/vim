" Vim syntax file
" Language:	    sudoers(5) configuration files.
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-12-17
" arch-tag:	    02fc3bc8-4308-466f-b83e-718a7487b198

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" TODO: instead of 'skipnl', we would like to match a specific group that would
" match \\$ and then continue with the nextgroup, actually, the skipnl doesn't
" work...
" TODO: treat 'ALL' like a special (yay, a bundle of new rules!!!)

" User Specs
syn match   sudoersUserSpec '^' nextgroup=@sudoersUserInSpec skipwhite

syn match   sudoersSpecEquals	      contained '=' nextgroup=@sudoersCmndSpecList skipwhite

syn cluster sudoersCmndSpecList	      contains=sudoersUserRunasBegin,sudoersPASSWD,@sudoersCmndInSpec

" Todo
syn keyword sudoersTodo		      contained TODO FIXME XXX NOTE

" Comments
syn region  sudoersComment	      matchgroup=sudoersComment start='#' end='$' contains=sudoersTodo

" Aliases
syn keyword sudoersAlias	      User_Alias Runas_Alias nextgroup=sudoersUserAlias skipwhite skipnl
syn keyword sudoersAlias	      Host_Alias nextgroup=sudoersHostAlias skipwhite skipnl
syn keyword sudoersAlias	      Cmnd_Alias nextgroup=sudoersCmndAlias skipwhite skipnl

" Names
syn match   sudoersUserAlias	      contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersUserAliasEquals  skipwhite skipnl
syn match   sudoersUserNameInList     contained '\<\l\+\>'	    nextgroup=@sudoersUserList	      skipwhite skipnl
syn match   sudoersUIDInList	      contained '#\d\+\>'	    nextgroup=@sudoersUserList	      skipwhite skipnl
syn match   sudoersGroupInList	      contained '%\l\+\>'	    nextgroup=@sudoersUserList	      skipwhite skipnl
syn match   sudoersUserNetgroupInList contained '+\l\+\>'	    nextgroup=@sudoersUserList	      skipwhite skipnl
syn match   sudoersUserAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserList	      skipwhite skipnl

syn match   sudoersUserName	      contained '\<\l\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersUID		      contained '#\d\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersGroup	      contained '%\l\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersUserNetgroup	      contained '+\l\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersUserAliasRef	      contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersParameter	      skipwhite skipnl

syn match   sudoersUserNameInSpec     contained '\<\l\+\>'	    nextgroup=@sudoersUserSpec	      skipwhite skipnl
syn match   sudoersUIDInSpec	      contained '#\d\+\>'	    nextgroup=@sudoersUserSpec	      skipwhite skipnl
syn match   sudoersGroupInSpec	      contained '%\l\+\>'	    nextgroup=@sudoersUserSpec	      skipwhite skipnl
syn match   sudoersUserNetgroupInSpec contained '+\l\+\>'	    nextgroup=@sudoersUserSpec	      skipwhite skipnl
syn match   sudoersUserAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserSpec	      skipwhite skipnl

syn match   sudoersUserNameInRunas    contained '\<\l\+\>'	    nextgroup=@sudoersUserRunas	      skipwhite skipnl
syn match   sudoersUIDInRunas	      contained '#\d\+\>'	    nextgroup=@sudoersUserRunas	      skipwhite skipnl
syn match   sudoersGroupInRunas	      contained '%\l\+\>'	    nextgroup=@sudoersUserRunas	      skipwhite skipnl
syn match   sudoersUserNetgroupInRunas contained '+\l\+\>'	    nextgroup=@sudoersUserRunas	      skipwhite skipnl
syn match   sudoersUserAliasInRunas   contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersUserRunas	      skipwhite skipnl

syn match   sudoersHostAlias	      contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersHostAliasEquals  skipwhite skipnl
syn match   sudoersHostNameInList     contained '\<\l\+\>'	    nextgroup=@sudoersHostList	      skipwhite skipnl
syn match   sudoersIPAddrInList	      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}' nextgroup=@sudoersHostList skipwhite skipnl
syn match   sudoersNetworkInList      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}\%(/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\)\=' nextgroup=@sudoersHostList skipwhite skipnl
syn match   sudoersHostNetgroupInList contained '+\l\+\>'	    nextgroup=@sudoersHostList	      skipwhite skipnl
syn match   sudoersHostAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersHostList	      skipwhite skipnl

syn match   sudoersHostName	      contained '\<\l\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersIPAddr	      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}' nextgroup=@sudoersParameter skipwhite skipnl
syn match   sudoersNetwork	      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}\%(/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\)\=' nextgroup=@sudoersParameter skipwhite skipnl
syn match   sudoersHostNetgroup	      contained '+\l\+\>'	    nextgroup=@sudoersParameter	      skipwhite skipnl
syn match   sudoersHostAliasRef	      contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersParameter	      skipwhite skipnl

syn match   sudoersHostNameInSpec     contained '\<\l\+\>'	    nextgroup=@sudoersHostSpec	      skipwhite skipnl
syn match   sudoersIPAddrInSpec	      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}' nextgroup=@sudoersHostSpec skipwhite skipnl
syn match   sudoersNetworkInSpec      contained '\%(\d\{1,3}\.\)\{3}\d\{1,3}\%(/\%(\%(\d\{1,3}\.\)\{3}\d\{1,3}\|\d\+\)\)\=' nextgroup=@sudoersHostSpec skipwhite skipnl
syn match   sudoersHostNetgroupInSpec contained '+\l\+\>'	    nextgroup=@sudoersHostSpec	      skipwhite skipnl
syn match   sudoersHostAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersHostSpec	      skipwhite skipnl

syn match   sudoersCmndAlias	      contained '\<\u[A-Z0-9_]*\>'  nextgroup=sudoersCmndAliasEquals  skipwhite skipnl
syn match   sudoersCmndNameInList     contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=@sudoersCmndList,sudoersCommandEmpty,sudoersCommandArgs skipwhite
syn match   sudoersCmndAliasInList    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersCmndList	      skipwhite skipnl

syn match   sudoersCmndNameInSpec     contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=@sudoersCmndSpec,sudoersCommandEmptyInSpec,sudoersCommandArgsInSpec skipwhite
syn match   sudoersCmndAliasInSpec    contained '\<\u[A-Z0-9_]*\>'  nextgroup=@sudoersCmndSpec	      skipwhite skipnl

" Delimiters
syn match   sudoersUserAliasEquals  contained '=' nextgroup=@sudoersUserInList	skipwhite skipnl
syn match   sudoersUserListComma    contained ',' nextgroup=@sudoersUserInList	skipwhite skipnl
syn match   sudoersUserListColon    contained ':' nextgroup=sudoersUserAlias	skipwhite skipnl
syn cluster sudoersUserList	    contains=sudoersUserListComma,sudoersUserListColon

syn match   sudoersUserSpecComma    contained ',' nextgroup=@sudoersUserInSpec	skipwhite skipnl
syn cluster sudoersUserSpec	    contains=sudoersUserSpecComma,@sudoersHostInSpec

syn match   sudoersUserRunasBegin   contained '(' nextgroup=@sudoersUserInRunas skipwhite skipnl
syn match   sudoersUserRunasComma   contained ',' nextgroup=@sudoersUserInRunas	skipwhite skipnl
syn match   sudoersUserRunasEnd	    contained ')' nextgroup=sudoersPASSWD,@sudoersCmndInSpec skipwhite skipnl
syn cluster sudoersUserRunas	    contains=sudoersUserRunasComma,@sudoersUserInRunas,sudoersUserRunasEnd


syn match   sudoersHostAliasEquals  contained '=' nextgroup=@sudoersHostInList	skipwhite skipnl
syn match   sudoersHostListComma    contained ',' nextgroup=@sudoersHostInList	skipwhite skipnl
syn match   sudoersHostListColon    contained ':' nextgroup=sudoersHostAlias	skipwhite skipnl
syn cluster sudoersHostList	    contains=sudoersHostListComma,sudoersHostListColon

syn match   sudoersHostSpecComma    contained ',' nextgroup=@sudoersHostInSpec	skipwhite skipnl
syn cluster sudoersHostSpec	    contains=sudoersHostSpecComma,sudoersSpecEquals


syn match   sudoersCmndAliasEquals  contained '=' nextgroup=@sudoersCmndInList	skipwhite skipnl
syn match   sudoersCmndListComma    contained ',' nextgroup=@sudoersCmndInList	skipwhite skipnl
syn match   sudoersCmndListColon    contained ':' nextgroup=sudoersCmndAlias	skipwhite skipnl
syn cluster sudoersCmndList	    contains=sudoersCmndListComma,sudoersCmndListColon

syn match   sudoersCmndSpecComma    contained ',' nextgroup=@sudoersCmndSpecList skipwhite skipnl
syn match   sudoersCmndSpecColon    contained ':' nextgroup=@sudoersUserInSpec	skipwhite skipnl
syn cluster sudoersCmndSpec	    contains=sudoersCmndSpecComma,sudoersCmndSpecColon

" Lists
syn cluster sudoersUserInList	    contains=sudoersUserNegationInList,sudoersUserNameInList,sudoersUIDInList,sudoersGroupInList,sudoersUserNetgroupInList,sudoersUserAliasInList
syn cluster sudoersHostInList	    contains=sudoersHostNegationInList,sudoersHostNameInList,sudoersIPAddrInList,sudoersNetworkInList,sudoersHostNetgroupInList,sudoersHostAliasInList
syn cluster sudoersCmndInList	    contains=sudoersCmndNegationInList,sudoersCmndNameInList,sudoersCmndAliasInList

syn cluster sudoersUser		    contains=sudoersUserNegation,sudoersUserName,sudoersUID,sudoersGroup,sudoersUserNetgroup,sudoersUserAliasRef
syn cluster sudoersHost		    contains=sudoersHostNegation,sudoersHostName,sudoersIPAddr,sudoersNetwork,sudoersHostNetgroup,sudoersHostAliasRef

syn cluster sudoersUserInSpec	    contains=sudoersUserNegationInSpec,sudoersUserNameInSpec,sudoersUIDInSpec,sudoersGroupInSpec,sudoersUserNetgroupInSpec,sudoersUserAliasInSpec
syn cluster sudoersHostInSpec	    contains=sudoersHostNegationInSpec,sudoersHostNameInSpec,sudoersIPAddrInSpec,sudoersNetworkInSpec,sudoersHostNetgroupInSpec,sudoersHostAliasInSpec
syn cluster sudoersUserInRunas	    contains=sudoersUserNegationInRunas,sudoersUserNameInRunas,sudoersUIDInRunas,sudoersGroupInRunas,sudoersUserNetgroupInRunas,sudoersUserAliasInRunas
syn cluster sudoersCmndInSpec	    contains=sudoersCmndNegationInSpec,sudoersCmndNameInSpec,sudoersCmndAliasInSpec

" Operators
syn match   sudoersUserNegationInList contained '!\+' nextgroup=@sudoersUserInList  skipwhite skipnl
syn match   sudoersHostNegationInList contained '!\+' nextgroup=@sudoersHostInList  skipwhite skipnl
syn match   sudoersCmndNegationInList contained '!\+' nextgroup=@sudoersCmndInList  skipwhite skipnl

syn match   sudoersUserNegation	      contained '!\+' nextgroup=@sudoersUser	    skipwhite skipnl
syn match   sudoersHostNegation	      contained '!\+' nextgroup=@sudoersHost	    skipwhite skipnl

syn match   sudoersUserNegationInSpec contained '!\+' nextgroup=@sudoersUserInSpec  skipwhite skipnl
syn match   sudoersHostNegationInSpec contained '!\+' nextgroup=@sudoersHostInSpec  skipwhite skipnl
syn match   sudoersUserNegationInRunas contained '!\+' nextgroup=@sudoersUserInRunas skipwhite skipnl
syn match   sudoersCmndNegationInSpec contained '!\+' nextgroup=@sudoersCmndInSpec  skipwhite skipnl

" Arguments
syn match   sudoersCommandArgs	    contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersCommandArgs,@sudoersCmndList skipwhite
syn match   sudoersCommandEmpty	    contained '""' nextgroup=@sudoersCmndList skipwhite skipnl

syn match   sudoersCommandArgsInSpec contained '[^[:space:],:=\\]\+\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersCommandArgsInSpec,@sudoersCmndSpec skipwhite
syn match   sudoersCommandEmptyInSpec contained '""' nextgroup=@sudoersCmndSpec skipwhite skipnl

" Default Entries
syn keyword sudoersDefaultEntry	Defaults nextgroup=sudoersDefaultTypeAt,sudoersDefaultTypeColon,sudoersDefaultTypeGreaterThan,@sudoersParameter skipwhite skipnl
syn match   sudoersDefaultTypeAt	  contained '@' nextgroup=@sudoersHost skipwhite skipnl
syn match   sudoersDefaultTypeColon	  contained ':' nextgroup=@sudoersUser skipwhite skipnl
syn match   sudoersDefaultTypeGreaterThan contained '>' nextgroup=@sudoersUser skipwhite skipnl

" TODO: could also deal with special characters here
syn keyword sudoersBooleanParameter contained long_opt_prompt ignore_dot mail_always mail_badpass mail_no_user mail_no_perms tty_tickets lecture authenticate root_sudo log_host log_year shell_noargs set_home always_set_home path_info preserve_groups fqdn insults requiretty env_editor rootpw runaspw targetpw set_logname stay_setuid env_reset use_loginclass nextgroup=sudoersParameterListComma skipwhite skipnl
syn keyword sudoersIntegerParameter contained passwd_tries loglinelen timestamp_timeout passwd_timeout umask nextgroup=sudoersIntegerParameterEquals skipwhite skipnl
syn keyword sudoersStringParameter  contained mailsub badpass_message timestampdir timestampowner passprompt runas_default syslog_goodpri syslog_badpri editor logfile syslog mailerpath mailerflags mailto exempt_group verifypw listpw nextgroup=sudoersStringParameterEquals skipwhite skipnl
syn keyword sudoersListParameter    contained env_check env_delete env_keep nextgroup=sudoersListParameterEquals skipwhite skipnl

syn match   sudoersParameterListComma contained ',' nextgroup=@sudoersParameter skipwhite skipnl

syn cluster sudoersParameter	    contains=sudoersBooleanParameter,sudoersIntegerParameterEquals,sudoersStringParameter,sudoersListParameter

syn match   sudoersIntegerParameterEquals contained '[+-]\==' nextgroup=sudoersIntegerValue skipwhite skipnl
syn match   sudoersStringParameterEquals  contained '[+-]\==' nextgroup=sudoersStringValue  skipwhite skipnl
syn match   sudoersListParameterEquals	  contained '[+-]\==' nextgroup=sudoersListValue    skipwhite skipnl

syn match   sudoersIntegerValue	contained '\d\+' nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersStringValue	contained '[^[:space:],:=\\]*\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersParameterListComma skipwhite skipnl
syn region  sudoersStringValue	contained start=+"+ skip=+\\"+ end=+"+ nextgroup=sudoersParameterListComma skipwhite skipnl
syn match   sudoersListValue	contained '[^[:space:],:=\\]*\%(\\[[:space:],:=\\][^[:space:],:=\\]*\)*' nextgroup=sudoersParameterListComma skipwhite skipnl
syn region  sudoersListValue	contained start=+"+ skip=+\\"+ end=+"+ nextgroup=sudoersParameterListComma skipwhite skipnl

" Special for specs
syn match   sudoersPASSWD	      contained '\%(NO\)\=PASSWD:' nextgroup=@sudoersCmndInSpec skipwhite

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_sudoers_syn_inits")
  if version < 508
    let did_sudoers_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink sudoersSpecEquals		Operator
  HiLink sudoersTodo			Todo
  HiLink sudoersComment			Comment
  HiLink sudoersAlias			Keyword
  HiLink sudoersUserAlias		Identifier
  HiLink sudoersUserNameInList		String
  HiLink sudoersUIDInList		Number
  HiLink sudoersGroupInList             PreProc
  HiLink sudoersUserNetgroupInList      PreProc
  HiLink sudoersUserAliasInList         PreProc
  HiLink sudoersUserName		String
  HiLink sudoersUID			Number
  HiLink sudoersGroup			PreProc
  HiLink sudoersUserNetgroup		PreProc
  HiLink sudoersUserAliasRef            PreProc
  HiLink sudoersUserNameInSpec		String
  HiLink sudoersUIDInSpec		Number
  HiLink sudoersGroupInSpec		PreProc
  HiLink sudoersUserNetgroupInSpec	PreProc
  HiLink sudoersUserAliasInSpec		PreProc
  HiLink sudoersUserNameInRunas		String
  HiLink sudoersUIDInRunas		Number
  HiLink sudoersGroupInRunas		PreProc
  HiLink sudoersUserNetgroupInRunas	PreProc
  HiLink sudoersUserAliasInRunas	PreProc
  HiLink sudoersHostAlias               Identifier
  HiLink sudoersHostNameInList          String
  HiLink sudoersIPAddrInList            Number
  HiLink sudoersNetworkInList           Number
  HiLink sudoersHostNetgroupInList      PreProc
  HiLink sudoersHostAliasInList         PreProc
  HiLink sudoersHostName		String
  HiLink sudoersIPAddr			Number
  HiLink sudoersNetwork			Number
  HiLink sudoersHostNetgroup		PreProc
  HiLink sudoersHostAliasRef            PreProc
  HiLink sudoersHostNameInSpec          String
  HiLink sudoersIPAddrInSpec            Number
  HiLink sudoersNetworkInSpec           Number
  HiLink sudoersHostNetgroupInSpec      PreProc
  HiLink sudoersHostAliasInSpec         PreProc
  HiLink sudoersCmndAlias		Identifier
  HiLink sudoersCmndNameInList		String
  HiLink sudoersCmndAliasInList         PreProc
  HiLink sudoersCmndNameInSpec		String
  HiLink sudoersCmndAliasInSpec         PreProc
  HiLink sudoersUserAliasEquals		Operator
  HiLink sudoersUserListComma           Delimiter
  HiLink sudoersUserListColon           Delimiter
  HiLink sudoersUserSpecComma           Delimiter
  HiLink sudoersUserRunasBegin		Delimiter
  HiLink sudoersUserRunasComma		Delimiter
  HiLink sudoersUserRunasEnd		Delimiter
  HiLink sudoersHostAliasEquals         Operator
  HiLink sudoersHostListComma           Delimiter
  HiLink sudoersHostListColon           Delimiter
  HiLink sudoersHostSpecComma           Delimiter
  HiLink sudoersCmndAliasEquals         Operator
  HiLink sudoersCmndListComma           Delimiter
  HiLink sudoersCmndListColon           Delimiter
  HiLink sudoersCmndSpecComma           Delimiter
  HiLink sudoersCmndSpecColon           Delimiter
  HiLink sudoersUserNegationInList      Operator
  HiLink sudoersHostNegationInList      Operator
  HiLink sudoersCmndNegationInList      Operator
  HiLink sudoersUserNegation		Operator
  HiLink sudoersHostNegation		Operator
  HiLink sudoersUserNegationInSpec	Operator
  HiLink sudoersHostNegationInSpec	Operator
  HiLink sudoersUserNegationInRunas	Operator
  HiLink sudoersCmndNegationInSpec	Operator
  HiLink sudoersCommandArgs		String
  HiLink sudoersCommandEmpty		Special
  HiLink sudoersDefaultEntry		Keyword
  HiLink sudoersDefaultTypeAt		Special
  HiLink sudoersDefaultTypeColon	Special
  HiLink sudoersDefaultTypeGreaterThan	Special
  HiLink sudoersBooleanParameter        Identifier
  HiLink sudoersIntegerParameter        Identifier
  HiLink sudoersStringParameter         Identifier
  HiLink sudoersListParameter           Identifier
  HiLink sudoersParameterListComma      Delimiter
  HiLink sudoersIntegerParameterEquals  Operator
  HiLink sudoersStringParameterEquals   Operator
  HiLink sudoersListParameterEquals     Operator
  HiLink sudoersIntegerValue            Number
  HiLink sudoersStringValue             String
  HiLink sudoersListValue               String
  HiLink sudoersPASSWD			Special

  delcommand HiLink
endif

let b:current_syntax = "sudoers"

" vim: set sts=2 sw=2:
