" Vim syntax file
" Language:	    readline configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/readline/
" Latest Revision:  2004-05-22
" arch-tag:	    6d8e7da4-b39c-4bf7-8e6a-d9135f993457
" Variables:
"   readline_has_bash - if defined add support for bash specific
"			settings/functions

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk 48-57,65-90,97-122,-
delcommand SetIsk

" comments
syn region  readlineComment	display oneline matchgroup=readlineComment start="^\s*#" end="$" contains=readlineTodo

" todo
syn keyword readlineTodo	contained TODO FIXME XXX NOTE

" strings (argh...not the way i want it, but fine..."
syn match   readlineString	"^\s*[A-Za-z-]\+:"me=e-1 contains=readlineKeys
syn region  readlineString	display oneline start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=readlineKeysTwo

" special key
syn case ignore
syn keyword readlineKeys	contained Control Meta Del Esc Escape LFD Newline Ret Return Rubout Space Spc Tab
syn case match

syn match   readlineKeysTwo	contained +\\\([CM]-\|[e\\"'abdfnrtv]\|\o\{3}\|x\x\{3}\)+

" keymaps
syn match   readlineKeymaps	contained "emacs\(-standard\|-meta\|-ctlx\)\="
syn match   readlineKeymaps	contained "vi\(-move\|-command\|-insert\)\="

" bell styles
syn keyword readlineBellStyles	contained audible visible none

" numbers
syn match   readlineNumber	contained "\<\d\+\>"

" booleans
syn case ignore
syn keyword readlineBoolean	contained on off
syn case match

" conditionals
syn keyword readlineIfOps	contained mode term

syn region  readlineConditional display oneline transparent matchgroup=readlineConditional start="^\s*$if" end="$" contains=readlineIfOps,readlineKeymaps
syn match   readlineConditional	"^\s*$\(else\|endif\)\>"

" include
syn match   readlineInclude	"^\s*$include\>"

" settings

syn region  readlineSet		display oneline transparent matchgroup=readlineKeyword start="^\s*set\>" end="$"me=e-1 contains=readlineNumber,readlineBoolean,readlineKeymaps,readlineBellStyles,readlineSettings

syn keyword readlineSettings	contained bell-style comment-begin completion-ignore-case
syn keyword readlineSettings	contained completion-query-items convert-meta disable-completion editing-mode enable-keypad
syn keyword readlineSettings	contained expand-tilde horizontal-scroll-mode mark-directories keymap mark-modified-lines meta-flag
syn keyword readlineSettings	contained input-meta output-meta print-completions-horizontally show-all-if-ambiguous visible-stats
syn keyword readlineSettings	contained prefer-visible-bell blink-matching-paren
syn keyword readlineSettings	contained match-hidden-files history-preserve-point isearch-terminators

" bash extensions
if exists("readline_has_bash")
  "syn keyword readlineSettings	contained
endif

" key bindings
syn region  readlineBinding	display oneline transparent matchgroup=readlineKeyword start=":" end="$" contains=readlineKeys,readlineFunctions

syn match   readlineFunctions	contained "\<\(beginning\|end\)-of-line\>"
syn match   readlineFunctions	contained "\<\(backward\|forward\)-\(char\|word\)\>"
syn match   readlineFunctions	contained "\<\(previous\|next\|\(beginning\|end\)-of\|\(non-incremental-\)\=\(reverse\|forward\)-search\)-history\>"
syn match   readlineFunctions	contained "\<history-search-\(forward\|backward\)\>"
syn match   readlineFunctions	contained "\<yank-\(nth\|last\)-arg\>"
syn match   readlineFunctions	contained "\<\(backward-\)\=kill-\(\(whole-\)\=line\|word\)\>"
syn match   readlineFunctions	contained "\<\(start\|end\|call-last\)-kbd-macro\>"
syn match   readlineFunctions	contained "\<dump-\(functions\|variables\|macros\)\>"
syn match   readlineFunctions	contained "\<non-incremental-\(reverse\|forward\)-search-history-again\>"
syn keyword readlineFunctions	contained clear-screen redraw-current-line accept-line delete-char backward-delete-char quoted-insert tab-insert
syn keyword readlineFunctions	contained self-insert transpose-chars transpose-words downcase-word capitalize-word unix-word-rubout
syn keyword readlineFunctions	contained delete-horizontal-space kill-region copy-region-as-kill copy-backward-word copy-forward-word yank yank-pop
syn keyword readlineFunctions	contained digit-argument universal-argument complete possible-completions insert-completions menu-complete
syn keyword readlineFunctions	contained re-read-init-file abort do-uppercase-version prefix-meta undo revert-line tilde-expand set-mark
syn keyword readlineFunctions	contained exchange-point-and-mark character-search character-search-backward insert-comment emacs-editing-mode vi-editing-mode
syn keyword readlineFunctions	contained unix-line-discard upcase-word backward-delete-word vi-eof-maybe vi-movement-mode vi-match vi-tilde-expand
syn keyword readlineFunctions	contained vi-complete vi-char-search vi-redo vi-search vi-arg-digit vi-append-eol vi-prev-word vi-change-to vi-delete-to
syn keyword readlineFunctions	contained vi-end-word vi-fetch-history vi-insert-beg vi-search-again vi-put vi-replace vi-subst vi-yank-to vi-first-print
syn keyword readlineFunctions	contained vi-yank-arg vi-goto-mark vi-append-mode vi-insertion-mode prev-history vi-set-mark vi-search-again vi-put vi-change-char
syn keyword readlineFunctions	contained vi-subst vi-delete vi-yank-to vi-column vi-change-case vi-overstrike vi-overstrike-delete
syn keyword readlineFunctions	contained do-lowercase-version delete-char-or-list tty-status arrow-key-prefix
syn keyword readlineFunctions	contained vi-back-to-indent vi-bword vi-bWord vi-eword vi-eWord vi-fword vi-fWord vi-next-word

" bash extensions
if exists("readline_has_bash")
  syn keyword readlineFunctions	contained shell-expand-line history-expand-line magic-space alias-expand-line history-and-alias-expand-line insert-last-argument
  syn keyword readlineFunctions	contained operate-and-get-next forward-backward-delete-char delete-char-or-list complete-filename possible-filename-completions
  syn keyword readlineFunctions	contained complete-username possible-username-completions complete-variable possible-variable-completions complete-hostname
  syn keyword readlineFunctions	contained possible-hostname-completions complete-command possible-command-completions dynamic-complete-history complete-into-braces
  syn keyword readlineFunctions	contained glob-expand-word glob-list-expansions display-shell-version
  syn keyword readlineFunctions	contained glob-complete-word edit-and-execute-command
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_readline_syn_inits")
  if version < 508
    let did_readline_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink readlineComment	Comment
  HiLink readlineTodo		Todo
  HiLink readlineString		String
  HiLink readlineKeys		SpecialChar
  HiLink readlineKeysTwo	SpecialChar
  HiLink readlineKeymaps	Constant
  HiLink readlineBellStyles	Constant
  HiLink readlineNumber		Number
  HiLink readlineBoolean	Boolean
  HiLink readlineIfOps		Type
  HiLink readlineConditional	Conditional
  HiLink readlineInclude	Include
  HiLink readlineKeyword	Keyword
  HiLink readlineSettings	Type
  HiLink readlineFunctions	Type
  delcommand HiLink
endif

let b:current_syntax = "readline"

" vim: set sts=2 sw=2:
