" Vim syntax file
" Language:     DNS/BIND Zone File
" Maintainer:   jehsom@jehsom.com
" URL:		http://scripts.jehsom.com
" Last Change:  2001 Sep 02

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syntax clear
elseif exists("b:current_syntax")
    finish
endif

" Last match is taken!
syn match	dnsKeyword	"\<\(IN\|A\|SOA\|NS\|CNAME\|MX\|PTR\|SOA\|MB\|MG\|MR\|NULL\|WKS\|HINFO\|TXT\|CS\|CH\|CPU\|OS\)\>"
syn match   dnsRecordName       "^[^ 	]*"
syn match   dnsPreProc		"^\$[^ ]*"
syn match   dnsComment		";.*$"
syn match   dnsDataFQDN		"\<[^ 	]*\.[ 	]*$"
syn match   dnsConstant			"\<\([0-9][0-9.]*\|[0-9.]*[0-9]\)\>"
syn match   dnsIPaddr		"\<\(\([0-2]\)\{0,1}\([0-9]\)\{1,2}\.\)\{3}\([0-2]\)\{0,1}\([0-9]\)\{1,2}\>[ 	]*$"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet.
if version >= 508 || !exists("did_dns_syntax_inits")
    if version < 508
	let did_dns_syntax_inits = 1
	command -nargs=+ HiLink hi link <args>
    else
	command -nargs=+ HiLink hi def link <args>
    endif

    HiLink dnsComment     Comment
    HiLink dnsDataFQDN    Identifier
    HiLink dnsPreProc     PreProc
    HiLink dnsKeyword     Keyword
    HiLink dnsRecordName  Type
    HiLink dnsIPaddr      Type
    HiLink dnsIPerr       Error
    HiLink dnsConstant	  Constant

    delcommand HiLink
endif

let b:current_syntax = "dns"
