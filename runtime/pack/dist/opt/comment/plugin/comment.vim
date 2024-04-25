vim9script

import autoload 'comment.vim'
nnoremap <silent> <expr> gc comment.Toggle()
xnoremap <silent> <expr> gc comment.Toggle()
nnoremap <silent> <expr> gcc comment.Toggle() .. '_'
