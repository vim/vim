vim9script

# Vim indent file
# Language:	Vim script
# Maintainer:	Bram Moolenaar <Bram@vim.org>
# Last Change:	2023 Feb 02

# Only load this indent file when no other was loaded.
if exists('b:did_indent')
    finish
endif

b:did_indent = true
b:undo_indent = 'setlocal indentkeys< indentexpr<'

import autoload '../autoload/dist/vimindent.vim'

setlocal indentexpr=vimindent.Expr()
setlocal indentkeys+==endif,=enddef,=endfu,=endfor,=endwh,=endtry,=endclass,=endinterface,=endenum,=},=else,=cat,=finall,=END,0\\
execute('setlocal indentkeys+=0=\"\\\ ,0=#\\\ ')
setlocal indentkeys-=0#
setlocal indentkeys-=:
