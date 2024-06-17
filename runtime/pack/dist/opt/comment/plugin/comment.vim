vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2024-04-26

import autoload 'comment.vim'
nnoremap <silent> <expr> gc comment.Toggle()
xnoremap <silent> <expr> gc comment.Toggle()
nnoremap <silent> <expr> gcc comment.Toggle() .. '_'
