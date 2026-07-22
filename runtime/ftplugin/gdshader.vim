vim9script

# Vim filetype plugin file
# Language: Godot shading language
# Maintainer: Maxim Kim <habamax@gmail.com>
# Website: https://github.com/habamax/vim-gdscript
# Last Update: 2025-06-09

if exists("b:did_ftplugin") | finish | endif
b:did_ftplugin = 1

b:undo_ftplugin = 'setlocal suffixesadd< comments< commentstring<'

setlocal suffixesadd=.gdshader
setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://
setlocal commentstring=//\ %s
