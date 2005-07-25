" Wget syntax file
" Filename:     wget.vim
" Language:     Wget configuration file ( /etc/wgetrc ~/.wgetrc )
" Maintainer:   Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:          http://gus.gscit.monash.edu.au/~djkea2/vim/syntax/wget.vim
" Last Change:  2005 Jul 24

" TODO: all commands are actually underscore and hyphen insensitive, though
"       they are normally named as listed below

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match   wgetComment "^\s*#.*$" contains=wgetTodo

syn keyword wgetTodo TODO NOTE FIXME XXX contained

syn match   wgetAssignment "^\s*[A-Za-z0-9_-]\+\s*=\s*.*$" contains=wgetCommand,wgetAssignmentOperator,wgetString,wgetBoolean,wgetNumber,wgetValue,wgetQuota,wgetRestriction,wgetTime

syn match   wgetAssignmentOperator "=" contained

syn region  wgetString start=+"+ skip=+\\\\\|\\"+ end=+"+ contained oneline
syn region  wgetString start=+'+ skip=+\\\\\|\\'+ end=+'+ contained oneline

" Note: make this a match so that always_rest matches properly
syn case ignore
syn match   wgetBoolean	"\<on\|off\|always\|never\|1\|0\>" contained
syn case match

syn match   wgetNumber	"\<\d\+\|inf\>"		contained
syn match   wgetQuota	"\<\d\+[kKmM]\?\>"	contained
syn match   wgetTime	"\<\d\+[smhdw]\>"	contained

syn case ignore
syn keyword wgetValue	default binary mega giga micro contained
syn case match

syn match   wgetRestriction  "\<\%(windows\|unix\)\%(,nocontrol\)\=\>"	contained
syn match   wgetRestriction  "\<nocontrol\>"				contained

syn case ignore
syn match wgetCommand "^\s*accept" contained
syn match wgetCommand "^\s*add[-_]\=hostdir" contained
syn match wgetCommand "^\s*always[-_]\=rest" contained
syn match wgetCommand "^\s*background" contained
syn match wgetCommand "^\s*backup[-_]\=converted" contained
syn match wgetCommand "^\s*backups" contained
syn match wgetCommand "^\s*base" contained
syn match wgetCommand "^\s*bind[-_]\=address" contained
syn match wgetCommand "^\s*ca[-_]\=certificate" contained
syn match wgetCommand "^\s*ca[-_]\=directory" contained
syn match wgetCommand "^\s*cache" contained
syn match wgetCommand "^\s*certificate" contained
syn match wgetCommand "^\s*certificate[-_]\=type" contained
syn match wgetCommand "^\s*check[-_]\=certificate" contained
syn match wgetCommand "^\s*connect[-_]\=timeout" contained
syn match wgetCommand "^\s*continue" contained
syn match wgetCommand "^\s*convert[-_]\=links" contained
syn match wgetCommand "^\s*cookies" contained
syn match wgetCommand "^\s*cut[-_]\=dirs" contained
syn match wgetCommand "^\s*debug" contained
syn match wgetCommand "^\s*delete[-_]\=after" contained
syn match wgetCommand "^\s*dns[-_]\=cache" contained
syn match wgetCommand "^\s*dns[-_]\=timeout" contained
syn match wgetCommand "^\s*dir[-_]\=prefix" contained
syn match wgetCommand "^\s*dir[-_]\=struct" contained
syn match wgetCommand "^\s*domains" contained
syn match wgetCommand "^\s*dot[-_]\=bytes" contained
syn match wgetCommand "^\s*dots[-_]\=in[-_]\=line" contained
syn match wgetCommand "^\s*dot[-_]\=spacing" contained
syn match wgetCommand "^\s*dot[-_]\=style" contained
syn match wgetCommand "^\s*egd[-_]\=file" contained
syn match wgetCommand "^\s*exclude[-_]\=directories" contained
syn match wgetCommand "^\s*exclude[-_]\=domains" contained
syn match wgetCommand "^\s*follow[-_]\=ftp" contained
syn match wgetCommand "^\s*follow[-_]\=tags" contained
syn match wgetCommand "^\s*force[-_]\=html" contained
syn match wgetCommand "^\s*ftp[-_]\=passw\(or\)\=d" contained
syn match wgetCommand "^\s*ftp[-_]\=user" contained
syn match wgetCommand "^\s*ftp[-_]\=proxy" contained
syn match wgetCommand "^\s*glob" contained
syn match wgetCommand "^\s*header" contained
syn match wgetCommand "^\s*html[-_]\=extension" contained
syn match wgetCommand "^\s*htmlify" contained
syn match wgetCommand "^\s*http[-_]\=keep[-_]\=alive" contained
syn match wgetCommand "^\s*http[-_]\=passwd" contained
syn match wgetCommand "^\s*http[-_]\=password" contained
syn match wgetCommand "^\s*http[-_]\=proxy" contained
syn match wgetCommand "^\s*https[-_]\=proxy" contained
syn match wgetCommand "^\s*http[-_]\=user" contained
syn match wgetCommand "^\s*ignore[-_]\=length" contained
syn match wgetCommand "^\s*ignore[-_]\=tags" contained
syn match wgetCommand "^\s*include[-_]\=directories" contained
syn match wgetCommand "^\s*inet4[-_]\=only" contained
syn match wgetCommand "^\s*inet6[-_]\=only" contained
syn match wgetCommand "^\s*input" contained
syn match wgetCommand "^\s*keep[-_]\=session[-_]\=cookies" contained
syn match wgetCommand "^\s*kill[-_]\=longer" contained
syn match wgetCommand "^\s*limit[-_]\=rate" contained
syn match wgetCommand "^\s*load[-_]\=cookies" contained
syn match wgetCommand "^\s*logfile" contained
syn match wgetCommand "^\s*login" contained
syn match wgetCommand "^\s*mirror" contained
syn match wgetCommand "^\s*netrc" contained
syn match wgetCommand "^\s*no[-_]\=clobber" contained
syn match wgetCommand "^\s*no[-_]\=parent" contained
syn match wgetCommand "^\s*no[-_]\=proxy" contained
" Note: this option is deprecated, use 'tries' instead
syn match wgetCommand "^\s*numtries" contained
syn match wgetCommand "^\s*output[-_]\=document" contained
syn match wgetCommand "^\s*page[-_]\=requisites" contained
syn match wgetCommand "^\s*passive[-_]\=ftp" contained
syn match wgetCommand "^\s*passwd" contained
syn match wgetCommand "^\s*password" contained
syn match wgetCommand "^\s*post[-_]\=data" contained
syn match wgetCommand "^\s*post[-_]\=file" contained
syn match wgetCommand "^\s*prefer[-_]\=family" contained
syn match wgetCommand "^\s*preserve[-_]\=permissions" contained
syn match wgetCommand "^\s*private[-_]\=key" contained
syn match wgetCommand "^\s*private[-_]\=key[-_]\=type" contained
syn match wgetCommand "^\s*progress" contained
syn match wgetCommand "^\s*protocol[-_]\=directories" contained
syn match wgetCommand "^\s*proxy[-_]\=passwd" contained
syn match wgetCommand "^\s*proxy[-_]\=password" contained
syn match wgetCommand "^\s*proxy[-_]\=user" contained
syn match wgetCommand "^\s*quiet" contained
syn match wgetCommand "^\s*quota" contained
syn match wgetCommand "^\s*random[-_]\=file" contained
syn match wgetCommand "^\s*random[-_]\=wait" contained
syn match wgetCommand "^\s*read[-_]\=timeout" contained
syn match wgetCommand "^\s*reclevel" contained
syn match wgetCommand "^\s*recursive" contained
syn match wgetCommand "^\s*referer" contained
syn match wgetCommand "^\s*reject" contained
syn match wgetCommand "^\s*relative[-_]\=only" contained
syn match wgetCommand "^\s*remove[-_]\=listing" contained
syn match wgetCommand "^\s*restrict[-_]\=file[-_]\=names" contained
syn match wgetCommand "^\s*retr[-_]\=symlinks" contained
syn match wgetCommand "^\s*retry[-_]\=connrefused" contained
syn match wgetCommand "^\s*robots" contained
syn match wgetCommand "^\s*save[-_]\=cookies" contained
syn match wgetCommand "^\s*save[-_]\=headers" contained
syn match wgetCommand "^\s*secure[-_]\=protocol" contained
syn match wgetCommand "^\s*server[-_]\=response" contained
" Note: this option was removed in wget 1.8
syn match wgetCommand "^\s*simple[-_]\=host[-_]\=check" contained
syn match wgetCommand "^\s*span[-_]\=hosts" contained
syn match wgetCommand "^\s*spider" contained
syn match wgetCommand "^\s*strict[-_]\=comments" contained
syn match wgetCommand "^\s*sslcertfile" contained
syn match wgetCommand "^\s*sslcertkey" contained
syn match wgetCommand "^\s*timeout" contained
syn match wgetCommand "^\s*time[-_]\=stamping" contained
syn match wgetCommand "^\s*tries" contained
syn match wgetCommand "^\s*user" contained
syn match wgetCommand "^\s*use[-_]\=proxy" contained
syn match wgetCommand "^\s*user[-_]\=agent" contained
syn match wgetCommand "^\s*verbose" contained
syn match wgetCommand "^\s*wait" contained
syn match wgetCommand "^\s*wait[-_]\=retry" contained
syn case match

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_wget_syn_inits")
  if version < 508
    let did_wget_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink wgetAssignmentOperator Special
  HiLink wgetBoolean            Boolean
  HiLink wgetCommand            Identifier
  HiLink wgetComment            Comment
  HiLink wgetNumber             Number
  HiLink wgetQuota              Number
  HiLink wgetString             String
  HiLink wgetTodo               Todo
  HiLink wgetValue              Constant

  delcommand HiLink
endif

let b:current_syntax = "wget"

" vim: ts=8
