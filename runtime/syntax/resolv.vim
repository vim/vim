" Vim syntax file
" Language:     resolver configuration file
" Maintaner:    Radu Dineiu <littledragon@altern.org>
" URL:		http://ld.yi.org/vim/resolv.vim
" ChangeLog:    http://ld.yi.org/vim/resolv.ChangeLog
" Last Change:  2003 May 11
" Version:      0.1

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

" Errors, comments and operators
syn match resolvError /./
syn match resolvNull /^\s*$/
syn match resolvComment /^\s*#.*$/
syn match resolvOperator /[\/:]/ contained

" IP

syn cluster resolvIPCluster contains=resolvIPError,resolvIPSpecial
syn match resolvIPError /\%(\d\{4,}\|25[6-9]\|2[6-9]\d\|[3-9]\d\{2}\)[\.0-9]*/ contained
syn match resolvIPSpecial /\%(127\.\d\{1,3}\.\d\{1,3}\.\d\{1,3}\)/ contained

" General
syn match resolvIP contained /\%(\d\{1,4}\.\)\{3}\d\{1,4}/ contains=@resolvIPCluster
syn match resolvIPNetmask contained /\%(\d\{1,4}\.\)\{3}\d\{1,4}\%(\/\%(\%(\d\{1,4}\.\)\{,3}\d\{1,4}\)\)\?/ contains=resolvOperator,@resolvIPCluster
syn match resolvHostname contained /\w\{-}\.[-0-9A-Za-z_\.]*/

" Particular
syn match resolvIPNameserver contained /\%(\%(\d\{1,4}\.\)\{3}\d\{1,4}\%(\s\|$\)\)\{1,3}/ contains=@resolvIPCluster
syn match resolvHostnameSearch contained /\%(\w\{-}\.[-0-9A-Za-z_\.]\{-}\%(\s\|$\)\)\{1,6}/
syn match resolvIPNetmaskSortList contained /\%(\%(\d\{1,4}\.\)\{3}\d\{1,4}\%(\/\%(\%(\d\{1,4}\.\)\{,3}\d\{1,4}\)\)\?\%(\s\|$\)\)\{1,10}/ contains=resolvOperator,@resolvIPCluster

" Identifiers
syn match resolvNameserver /^nameserver / nextgroup=resolvIPNameserver
syn match resolvDomain /^domain / nextgroup=resolvHostname
syn match resolvSearch /^search / nextgroup=resolvHostnameSearch
syn match resolvSortList /^sortlist / nextgroup=resolvIPNetmaskSortList
syn match resolvOptions /^options / nextgroup=resolvOption

" Options
syn match resolvOption /\%(debug\|ndots:\d\)/ contained contains=resolvOperator

" Additional errors
syn match resolvError /^search .\{257,}/
syn match resolvNull /\s\{1,}$/

if version >= 508 || !exists("did_config_syntax_inits")
	if version < 508
		let did_config_syntax_inits = 1
		command! -nargs=+ HiLink hi link <args>
	else
		command! -nargs=+ HiLink hi def link <args>
	endif

	HiLink resolvIP Number
	HiLink resolvIPNetmask Number
	HiLink resolvHostname String
	HiLink resolvOption String

	HiLink resolvIPNameserver Number
	HiLink resolvHostnameSearch String
	HiLink resolvIPNetmaskSortList Number

	HiLink resolvNameServer Identifier
	HiLink resolvDomain Identifier
	HiLink resolvSearch Identifier
	HiLink resolvSortList Identifier
	HiLink resolvOptions Identifier

	HiLink resolvComment Comment
	HiLink resolvOperator Operator
	HiLink resolvError Error
	HiLink resolvIPError Error
	HiLink resolvIPSpecial Special

	delcommand HiLink
endif

let b:current_syntax = "resolv"

" vim: ts=8 ft=vim
