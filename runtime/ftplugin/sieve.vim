" Vim filetype plugin file
" Language:             Sieve filtering language input file
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2025 Feb 20

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl com< cms< fo< ff<"

setlocal comments=s1:/*,mb:*,ex:*/,:# commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croql

" https://datatracker.ietf.org/doc/html/rfc5228#section-2.2 says "newlines
" (CRLF, never just CR or LF)"
setlocal fileformat=dos

let &cpo = s:cpo_save
unlet s:cpo_save
