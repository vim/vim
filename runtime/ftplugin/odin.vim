vim9script

# Vim filetype plugin file
# Language: Odin
# Maintainer: Maxim Kim <habamax@gmail.com>
# Website: https://github.com/habamax/vim-odin
# Last Change: 2024-01-15
#              2024-05-23 by Riley Bruins <ribru17@gmail.com> ('commentstring')

if exists("b:did_ftplugin")
    finish
endif
b:did_ftplugin = 1

b:undo_ftplugin = 'setlocal commentstring<'
      \ .. '| setlocal comments<'
      \ .. '| setlocal suffixesadd<'

setlocal suffixesadd=.odin
setlocal commentstring=//\ %s
setlocal comments=s1:/*,mb:*,ex:*/,://
