" Vim filetype plugin file
" Language:     XS (Perl extension interface language)
" Maintainer:   Andy Lester <andy@petdance.com>
" URL:          http://github.com/petdance/vim-perl
" Last Change:  2009-08-14

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
    finish
endif

" Just use the C plugin for now.
runtime! ftplugin/c.vim ftplugin/c_*.vim ftplugin/c/*.vim
