" Vim syntax file
" Language:         readline(3) configuration file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19
"   readline_has_bash - if defined add support for bash specific
"                       settings/functions

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

setlocal iskeyword=@,48-57,-

syn keyword readlineTodo        contained TODO FIXME XXX NOTE

syn region  readlineComment     display oneline start='^\s*#' end='$'
                                \ contains=readlineTodo,@Spell

syn match   readlineString      '^\s*[A-Za-z-]\+:'me=e-1 contains=readlineKeys
syn region  readlineString      display oneline start=+"+ skip=+\\\\\|\\"+
                                \ end=+"+ contains=readlineKeysTwo

syn case ignore
syn keyword readlineKeys        contained Control Meta Del Esc Escape LFD
                                \ Newline Ret Return Rubout Space Spc Tab
syn case match

syn match   readlineKeysTwo     contained display
                                \ +\\\([CM]-\|[e\\"'abdfnrtv]\|\o\{3}\|x\x\{3}\)+

syn match   readlineKeymaps     contained display
                                \ 'emacs\(-standard\|-meta\|-ctlx\)\='
syn match   readlineKeymaps     contained display
                                \ 'vi\(-move\|-command\|-insert\)\='

syn keyword readlineBellStyles  contained audible visible none

syn match   readlineNumber      contained display '\<\d\+\>'

syn case ignore
syn keyword readlineBoolean     contained on off
syn case match

syn keyword readlineIfOps       contained mode term

syn region  readlineConditional display oneline transparent
                                \ matchgroup=readlineConditional
                                \ start='^\s*$if' end="$"
                                \ contains=readlineIfOps,readlineKeymaps
syn match   readlineConditional display '^\s*$\(else\|endif\)\>'

syn match   readlineInclude     display '^\s*$include\>'

syn region  readlineSet         display oneline transparent
                                \ matchgroup=readlineKeyword start='^\s*set\>'
                                \ end="$"me=e-1 contains=readlineNumber,
                                \ readlineBoolean,readlineKeymaps,
                                \ readlineBellStyles,readlineSettings

syn keyword readlineSettings    contained bell-style comment-begin
                                \ completion-ignore-case completion-query-items
                                \ convert-meta disable-completion editing-mode
                                \ enable-keypad expand-tilde
                                \ horizontal-scroll-mode mark-directories
                                \ keymap mark-modified-lines meta-flag
                                \ input-meta output-meta
                                \ print-completions-horizontally
                                \ show-all-if-ambiguous visible-stats
                                \ prefer-visible-bell blink-matching-paren
                                \ match-hidden-files history-preserve-point
                                \ isearch-terminators

syn region  readlineBinding     display oneline transparent
                                \ matchgroup=readlineKeyword start=':' end='$'
                                \ contains=readlineKeys,readlineFunctions

syn keyword readlineFunctions   contained display
                                \ beginning-of-line end-of-line forward-char
                                \ backward-char forward-word backward-word
                                \ clear-screen redraw-current-line
                                \ accept-line previous-history
                                \ next-history beginning-of-history
                                \ end-of-history reverse-search-history
                                \ forward-search-history
                                \ non-incremental-reverse-search-history
                                \ non-incremental-forward-search-history
                                \ history-search-forward
                                \ history-search-backward
                                \ yank-nth-arg yank-last-arg
                                \ delete-char backward-delete-char
                                \ forward-backward-delete-char quoted-insert
                                \ tab-insert self-insert transpose-chars
                                \ transpose-words upcase-word downcase-word
                                \ capitalize-word overwrite-mode kill-line
                                \ backward-kill-line unix-line-discard
                                \ kill-whole-line kill-word backward-kill-word
                                \ unix-word-rubout unix-filename-rubout
                                \ delete-horizontal-space kill-region
                                \ copy-region-as-kill copy-backward-word
                                \ copy-forward-word yank yank-pop
                                \ digit-argument universal-argument complete
                                \ possible-completions insert-completions
                                \ menu-complete delete-char-or-list
                                \ start-kbd-macro end-kbd-macro
                                \ call-last-kbd-macro re-read-init-file
                                \ abort do-uppercase-version prefix-meta
                                \ undo revert-line tilde-expand set-mark
                                \ exchange-point-and-mark character-search
                                \ character-search-backward insert-comment
                                \ dump-functions dump-variables dump-macros
                                \ emacs-editing-mode vi-editing-mode
                                \ vi-complete vi-char-search vi-redo
                                \ vi-search vi-arg-digit vi-append-eol
                                \ vi-prev-word vi-change-to vi-delete-to
                                \ vi-end-word vi-fetch-history vi-insert-beg
                                \ vi-search-again vi-put vi-replace
                                \ vi-subst vi-yank-to vi-first-print
                                \ vi-yank-arg vi-goto-mark vi-append-mode
                                \ vi-insertion-mode prev-history vi-set-mark
                                \ vi-search-again vi-put vi-change-char
                                \ vi-subst vi-delete vi-yank-to
                                \ vi-column vi-change-case vi-overstrike
                                \ vi-overstrike-delete do-lowercase-version
                                \ delete-char-or-list tty-status
                                \ arrow-key-prefix vi-back-to-indent vi-bword
                                \ vi-bWord vi-eword vi-eWord vi-fword vi-fWord
                                \ vi-next-word

if exists("readline_has_bash")
  syn keyword readlineFunctions contained
                                \ shell-expand-line history-expand-line
                                \ magic-space alias-expand-line
                                \ history-and-alias-expand-line
                                \ insert-last-argument operate-and-get-next
                                \ forward-backward-delete-char
                                \ delete-char-or-list complete-filename
                                \ possible-filename-completions
                                \ complete-username
                                \ possible-username-completions
                                \ complete-variable
                                \ possible-variable-completions
                                \ complete-hostname
                                \ possible-hostname-completions
                                \ complete-command
                                \ possible-command-completions
                                \ dynamic-complete-history
                                \ complete-into-braces
                                \ glob-expand-word glob-list-expansions
                                \ display-shell-version glob-complete-word
                                \ edit-and-execute-command
endif

hi def link readlineComment     Comment
hi def link readlineTodo        Todo
hi def link readlineString      String
hi def link readlineKeys        SpecialChar
hi def link readlineKeysTwo     SpecialChar
hi def link readlineKeymaps     Constant
hi def link readlineBellStyles  Constant
hi def link readlineNumber      Number
hi def link readlineBoolean     Boolean
hi def link readlineIfOps       Type
hi def link readlineConditional Conditional
hi def link readlineInclude     Include
hi def link readlineKeyword     Keyword
hi def link readlineSettings    Type
hi def link readlineFunctions   Type

let b:current_syntax = "readline"

let &cpo = s:cpo_save
unlet s:cpo_save
