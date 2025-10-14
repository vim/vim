vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2025 Mar 21
# 2025 Jun 22 by Vim Project: add <Plug> mappings #17563

import autoload 'comment.vim'

nnoremap <silent> <expr> <Plug>(comment-toggle) comment.Toggle()
xnoremap <silent> <expr> <Plug>(comment-toggle) comment.Toggle()
nnoremap <silent> <expr> <Plug>(comment-toggle-line) comment.Toggle() .. '_'
nnoremap <silent> <expr> <Plug>(comment-toggle-end) comment.Toggle() .. '$'

onoremap <silent> <Plug>(comment-text-object-inner) <scriptcmd>comment.ObjComment(v:true)<CR>
onoremap <silent> <Plug>(comment-text-object-outer) <scriptcmd>comment.ObjComment(v:false)<CR>
xnoremap <silent> <Plug>(comment-text-object-inner) <esc><scriptcmd>comment.ObjComment(v:true)<CR>
xnoremap <silent> <Plug>(comment-text-object-outer) <esc><scriptcmd>comment.ObjComment(v:false)<CR>

if get(g:, 'comment_mappings', true)
  nmap gc <Plug>(comment-toggle)
  xmap gc <Plug>(comment-toggle)
  nmap gcc <Plug>(comment-toggle-line)
  nmap gC <Plug>(comment-toggle-end)

  omap ic <Plug>(comment-text-object-inner)
  omap ac <Plug>(comment-text-object-outer)
  xmap ic <Plug>(comment-text-object-inner)
  xmap ac <Plug>(comment-text-object-outer)
endif
