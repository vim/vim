" Vim syntax file
" Language:	dnsmasq(8) configuration file
" Maintainer:	Thilo Six <T.Six@gmx.de>
" Last Change:	2011 Apr 28
" Credits:	This file is a mix of cfg.vim, wget.vim and xf86conf.vim, credits go to:
"		Igor N. Prischepoff
"		Doug Kearns
"		David Ne\v{c}as
"
" Options: 	let dnsmasq_backrgound_light = 1
"

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syntax clear
elseif exists ("b:current_syntax")
    finish
endif

if !exists("b:dnsmasq_backrgound_light")
	if exists("dnsmasq_backrgound_light")
		let b:dnsmasq_backrgound_light = dnsmasq_backrgound_light
	else
		let b:dnsmasq_backrgound_light = 0
	endif
endif


" case on
syn case match

"Parameters
syn match   DnsmasqParams   "^.\{-}="me=e-1 contains=DnsmasqComment
"... and their values (don't want to highlight '=' sign)
syn match   DnsmasqValues   "=.*"hs=s+1 contains=DnsmasqComment,DnsmasqSpecial

"...because we do it here.
syn match   DnsmasqEq	    display '=\|@\|/\|,' nextgroup=DnsmasqValues

syn match   DnsmasqSpecial    "#"

" String
syn match   DnsmasqString    "\".*\""
syn match   DnsmasqString    "'.*'"

" Comments
syn match   DnsmasqComment   "^#.*$" contains=DnsmasqTodo
syn match   DnsmasqComment   "[ \t]#.*$" contains=DnsmasqTodo

syn keyword DnsmasqTodo	    FIXME TODO XXX NOT contained

syn match DnsmasqKeyword    "^\s*add-mac\>"
syn match DnsmasqKeyword    "^\s*all-servers\>"
syn match DnsmasqKeyword    "^\s*bind-interfaces\>"
syn match DnsmasqKeyword    "^\s*bogus-priv\>"
syn match DnsmasqKeyword    "^\s*clear-on-reload\>"
syn match DnsmasqKeyword    "^\s*dhcp-authoritative\>"
syn match DnsmasqKeyword    "^\s*dhcp-fqdn\>"
syn match DnsmasqKeyword    "^\s*dhcp-no-override\>"
syn match DnsmasqKeyword    "^\s*dhcp-scriptuser\>"
syn match DnsmasqKeyword    "^\s*domain-needed\>"
syn match DnsmasqKeyword    "^\s*enable-dbus\>"
syn match DnsmasqKeyword    "^\s*enable-tftp\>"
syn match DnsmasqKeyword    "^\s*expand-hosts\>"
syn match DnsmasqKeyword    "^\s*filterwin2k\>"
syn match DnsmasqKeyword    "^\s*keep-in-foreground\>"
syn match DnsmasqKeyword    "^\s*leasefile-ro\>"
syn match DnsmasqKeyword    "^\s*localise-queries\>"
syn match DnsmasqKeyword    "^\s*localmx\>"
syn match DnsmasqKeyword    "^\s*log-dhcp\>"
syn match DnsmasqKeyword    "^\s*log-queries\>"
syn match DnsmasqKeyword    "^\s*no-daemon\>"
syn match DnsmasqKeyword    "^\s*no-hosts\>"
syn match DnsmasqKeyword    "^\s*no-negcache\>"
syn match DnsmasqKeyword    "^\s*no-ping\>"
syn match DnsmasqKeyword    "^\s*no-poll\>"
syn match DnsmasqKeyword    "^\s*no-resolv\>"
syn match DnsmasqKeyword    "^\s*proxy-dnssec\>"
syn match DnsmasqKeyword    "^\s*read-ethers\>"
syn match DnsmasqKeyword    "^\s*rebind-localhost-ok\>"
syn match DnsmasqKeyword    "^\s*selfmx\>"
syn match DnsmasqKeyword    "^\s*stop-dns-rebind\>"
syn match DnsmasqKeyword    "^\s*strict-order\>"
syn match DnsmasqKeyword    "^\s*tftp-no-blocksize\>"
syn match DnsmasqKeyword    "^\s*tftp-secure\>"
syn match DnsmasqKeyword    "^\s*tftp-unique-root\>"


if b:dnsmasq_backrgound_light == 1
    hi def DnsmasqParams    ctermfg=DarkGreen guifg=DarkGreen
    hi def DnsmasqKeyword   ctermfg=DarkGreen guifg=DarkGreen
else
    hi def link DnsmasqKeyword  Keyword
    hi def link DnsmasqParams   Keyword
endif
hi def link DnsmasqTodo	    Todo
hi def link DnsmasqSpecial  Constant
hi def link DnsmasqComment  Comment
hi def link DnsmasqString   Constant
hi def link DnsmasqValues   Normal
hi def link DnsmasqEq	    Constant

let b:current_syntax = "dnsmasq"
