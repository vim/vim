vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2025-03-20

import autoload 'comment.vim'

nnoremap <silent> <expr> gc comment.Toggle()
xnoremap <silent> <expr> gc comment.Toggle()
nnoremap <silent> <expr> gcc comment.Toggle() .. '_'

onoremap <silent>ic <scriptcmd>comment.ObjComment(v:true)<CR>
onoremap <silent>ac <scriptcmd>comment.ObjComment(v:false)<CR>
xnoremap <silent>ic <esc><scriptcmd>comment.ObjComment(v:true)<CR>
xnoremap <silent>ac <esc><scriptcmd>comment.ObjComment(v:false)<CR>
