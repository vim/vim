" Vim indent file
" Author: koe <ukoe@protonmail.com>
" Maintainer: FabricSoul <fabric.soul7@gmail.com>
" Language:	cobweb
" Last Change:	2025 Apr 20
" For bugs, patches and license go to https://github.com/UkoeHB/vim-cob/tree/main
if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal cindent
setlocal cinoptions=L0,(0,Ws,J1,j1,m1
setlocal cinkeys=0{,0},!^F,o,O,0[,0]
