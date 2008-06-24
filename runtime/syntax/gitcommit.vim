" Vim syntax file
" Language:	git commit file
" Maintainer:	Tim Pope <vimNOSPAM@tpope.info>
" Filenames:	*.git/COMMIT_EDITMSG
" Last Change:	2008 Apr 09

if exists("b:current_syntax")
    finish
endif

syn case match
syn sync minlines=50

if has("spell")
    syn spell toplevel
endif

syn include @gitcommitDiff syntax/diff.vim
syn region gitcommitDiff start=/\%(^diff --git \)\@=/ end=/^$\|^#\@=/ contains=@gitcommitDiff

syn match   gitcommitFirstLine	"\%^[^#].*"  nextgroup=gitcommitBlank skipnl
syn match   gitcommitSummary  	"^.\{0,50\}" contained containedin=gitcommitFirstLine nextgroup=gitcommitOverflow contains=@Spell
syn match   gitcommitOverflow	".*" contained contains=@Spell
syn match   gitcommitBlank	"^[^#].*" contained contains=@Spell
syn match   gitcommitComment	"^#.*"
syn region  gitcommitHead	start=/^#   / end=/^#$/ contained transparent
syn match   gitcommitOnBranch	"\%(^# \)\@<=On branch" contained containedin=gitcommitComment nextgroup=gitcommitBranch skipwhite
syn match   gitcommitBranch	"\S\+" contained
syn match   gitcommitHeader	"\%(^# \)\@<=.*:$"	contained containedin=gitcommitComment

syn region  gitcommitUntracked	start=/^# Untracked files:/ end=/^#$\|^#\@!/ contains=gitcommitHeader,gitcommitHead,gitcommitUntrackedFile fold
syn match   gitcommitUntrackedFile  "\t\@<=.*"	contained

syn region  gitcommitDiscarded	start=/^# Changed but not updated:/ end=/^#$\|^#\@!/ contains=gitcommitHeader,gitcommitHead,gitcommitDiscardedType fold
syn region  gitcommitSelected	start=/^# Changes to be committed:/ end=/^#$\|^#\@!/ contains=gitcommitHeader,gitcommitHead,gitcommitSelectedType fold

syn match   gitcommitDiscardedType	"\t\@<=[a-z][a-z ]*[a-z]: "he=e-2	contained containedin=gitcommitComment nextgroup=gitcommitDiscardedFile skipwhite
syn match   gitcommitSelectedType	"\t\@<=[a-z][a-z ]*[a-z]: "he=e-2	contained containedin=gitcommitComment nextgroup=gitcommitSelectedFile skipwhite
syn match   gitcommitDiscardedFile	".\{-\}\%($\| -> \)\@=" contained nextgroup=gitcommitDiscardedArrow
syn match   gitcommitSelectedFile	".\{-\}\%($\| -> \)\@=" contained nextgroup=gitcommitSelectedArrow
syn match   gitcommitDiscardedArrow	" -> " contained nextgroup=gitcommitDiscardedFile
syn match   gitcommitSelectedArrow	" -> " contained nextgroup=gitcommitSelectedFile

hi def link gitcommitSummary		Keyword
hi def link gitcommitComment		Comment
hi def link gitcommitUntracked		gitcommitComment
hi def link gitcommitDiscarded		gitcommitComment
hi def link gitcommitSelected		gitcommitComment
hi def link gitcommitOnBranch		Comment
hi def link gitcommitBranch		Special
hi def link gitcommitDiscardedType	gitcommitType
hi def link gitcommitSelectedType	gitcommitType
hi def link gitcommitType		Type
hi def link gitcommitHeader		PreProc
hi def link gitcommitUntrackedFile	gitcommitFile
hi def link gitcommitDiscardedFile	gitcommitFile
hi def link gitcommitSelectedFile	gitcommitFile
hi def link gitcommitFile		Constant
hi def link gitcommitDiscardedArrow	gitcommitArrow
hi def link gitcommitSelectedArrow	gitcommitArrow
hi def link gitcommitArrow		gitcommitComment
"hi def link gitcommitOverflow		Error
hi def link gitcommitBlank		Error

let b:current_syntax = "gitcommit"
