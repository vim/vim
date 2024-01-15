vim9script

# Vim filetype plugin file
# Language: odin
# Maintainer: Maxim Kim <habamax@gmail.com>
# Website: https://github.com/habamax/vim-odin
# Last Change: 2024-01-15

if exists("b:did_ftplugin")
    finish
endif
b:did_ftplugin = 1

b:undo_ftplugin = 'setlocal commentstring<'
      \ .. '| setlocal suffixesadd<'

setlocal suffixesadd=.odin
setlocal commentstring=//%s
