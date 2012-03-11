" Vim filetype plugin
" Language:      Lisp
" Maintainer:    Sergey Khorev <sergey.khorev@gmail.com>
" URL:		 http://sites.google.com/site/khorser/opensource/vim
" Original author:    Dorai Sitaram <ds26@gte.com>
" Original URL:		 http://www.ccs.neu.edu/~dorai/vimplugins/vimplugins.html
" Last Change:   Mar 5, 2012

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

setl comments=:;
setl define=^\\s*(def\\k*
setl formatoptions-=t
setl iskeyword+=+,-,*,/,%,<,=,>,:,$,?,!,@-@,94
setl lisp

" make comments behaviour like in c.vim
" e.g. insertion of ;;; and ;; on normal "O" or "o" when staying in comment
setl comments^=:;;;,:;;,sr:#\|,mb:\|,ex:\|#
setl formatoptions+=croql

let b:undo_ftplugin = "setlocal comments< define< formatoptions< iskeyword< lisp<"
