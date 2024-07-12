vim9script

# Vim indent file
# Language:	Vim script
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2024 Jul 12
# Former Maintainer:	Bram Moolenaar <Bram@vim.org>

# Only load this indent file when no other was loaded.
if exists('b:did_indent')
    finish
endif

b:did_indent = true
b:undo_indent = 'setlocal indentkeys< indentexpr<'

import autoload '../autoload/dist/vimindent.vim'

# export indentexpr as a global function, so it can be easily manipulated by plugins
g:VimIndent = vimindent.Expr
setlocal indentexpr=g:VimIndent()
setlocal indentkeys+==endif,=enddef,=endfu,=endfor,=endwh,=endtry,=endclass,=endinterface,=endenum,=},=else,=cat,=finall,=END,0\\
execute('setlocal indentkeys+=0=\"\\\ ,0=#\\\ ')
setlocal indentkeys-=0#
setlocal indentkeys-=:
