" Vim syntax file
" Language:	git send-email message
" Maintainer:	Tim Pope
" Filenames:	.gitsendemail.*
" Last Change:	2016 Aug 29

if exists("b:current_syntax")
  finish
endif

runtime! syntax/mail.vim
unlet! b:current_syntax
syn include @gitsendemailDiff syntax/diff.vim
syn region gitsendemailDiff start=/\%(^diff --\%(git\|cc\|combined\) \)\@=/ end=/^-- %/ fold contains=@gitsendemailDiff

syn case match

syn match   gitsendemailComment "\%^From.*#.*"
syn match   gitsendemailComment "^GIT:.*"
" list extracted from https://www.kernel.org/doc/html/latest/process/submitting-patches.html#when-to-use-acked-by-cc-and-co-developed-by
syn match   gitsendemailPseudoHeader "^\(Signed-off-by\|Acked-by\|Cc\|Co-Developed-by\|Co-authored-by\|Fixes\|Reported-by\|Reviewed-by\|Suggested-by\|Tested-by\): .*$"

hi def link gitsendemailComment Comment
hi def link gitsendemailPseudoHeader mailHeaderKey

let b:current_syntax = "gitsendemail"
