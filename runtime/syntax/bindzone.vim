" Vim syntax file
" Language:	BIND 8.x zone files (RFC1035)
" Maintainer:	glory hump <rnd@web-drive.ru>
" Last change:	Thu Apr 26 02:16:18 SAMST 2001
" Filenames:	/var/named/*
" URL:	http://rnd.web-drive.ru/vim/syntax/bindzone.vim
" $Id$

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case match

if version >= 600
  setlocal iskeyword=.,-,48-58,A-Z,a-z,_
else
  set iskeyword=.,-,48-58,A-Z,a-z,_
endif


" Master File Format (rfc 1035)

" directives
syn region	zoneRRecord	start=+^+ end=+$+ contains=zoneLHSDomain,zoneLHSIP,zoneIllegalDom,zoneWhitespace,zoneComment,zoneParen,zoneSpecial
syn match	zoneDirective	/\$ORIGIN\s\+/ nextgroup=zoneDomain,zoneIllegalDom
syn match	zoneDirective	/\$TTL\s\+/ nextgroup=zoneTTL
syn match	zoneDirective	/\$INCLUDE\s\+/
syn match	zoneDirective	/\$GENERATE\s/

syn match	zoneWhitespace	contained /^\s\+/ nextgroup=zoneTTL,zoneClass,zoneRRType
syn match	zoneError	"\s\+$"
syn match	zoneSpecial	contained /^[@.]\s\+/ nextgroup=zoneTTL,zoneClass,zoneRRType
syn match	zoneSpecial	contained /@$/

" domains and IPs
syn match	zoneLHSDomain	contained /^[-0-9A-Za-z.]\+\s\+/ nextgroup=zoneTTL,zoneClass,zoneRRType
syn match	zoneLHSIP	contained /^[0-9]\{1,3}\(\.[0-9]\{1,3}\)\{,3}\s\+/ nextgroup=zoneTTL,zoneClass,zoneRRType
syn match	zoneIPaddr	contained /\<[0-9]\{1,3}\(\.[0-9]\{1,3}\)\{,3}\>/
syn match	zoneDomain	contained /\<[0-9A-Za-z][-0-9A-Za-z.]\+\>/

syn match	zoneIllegalDom	contained /\S*[^-A-Za-z0-9.[:space:]]\S*\>/
"syn match	zoneIllegalDom	contained /[0-9]\S*[-A-Za-z]\S*/

" keywords
syn keyword	zoneClass	IN CHAOS nextgroup=zoneRRType

syn match	zoneTTL	contained /\<[0-9HhWwDd]\+\s\+/ nextgroup=zoneClass,zoneRRType
syn match	zoneRRType	contained /\s*\<\(NS\|HINFO\)\s\+/ nextgroup=zoneSpecial,zoneDomain
syn match	zoneRRType	contained /\s*\<CNAME\s\+/ nextgroup=zoneDomain,zoneSpecial
syn match	zoneRRType	contained /\s*\<SOA\s\+/ nextgroup=zoneDomain,zoneIllegalDom
syn match	zoneRRType	contained /\s*\<PTR\s\+/ nextgroup=zoneDomain,zoneIllegalDom
syn match	zoneRRType	contained /\s*\<MX\s\+/ nextgroup=zoneMailPrio
syn match	zoneRRType	contained /\s*\<A\s\+/ nextgroup=zoneIPaddr,zoneIllegalDom

" FIXME: catchup serial number
syn match	zoneSerial	contained /\<[0-9]\{9}\>/

syn match	zoneMailPrio	contained /\<[0-9]\+\s*/ nextgroup=zoneDomain,zoneIllegalDom
syn match	zoneErrParen	/)/
syn region	zoneParen	contained start=+(+ end=+)+ contains=zoneSerial,zoneTTL,zoneComment
syn match	zoneComment	";.*"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_bind_zone_syn_inits")
  if version < 508
    let did_bind_zone_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink zoneComment	Comment
  HiLink zoneDirective	Macro
  HiLink zoneLHSDomain	Statement
  HiLink zoneLHSIP	Statement
  HiLink zoneClass	Include
  HiLink zoneSpecial	Special
  HiLink zoneRRType	Type
  HiLink zoneError	Error
  HiLink zoneErrParen	Error
  HiLink zoneIllegalDom	Error
  HiLink zoneSerial	Todo
  HiLink zoneIPaddr	Number
  HiLink zoneDomain	Identifier

  delcommand HiLink
endif

let b:current_syntax = "bindzone"

" vim: ts=17
