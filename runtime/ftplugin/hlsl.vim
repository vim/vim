vim9script
# Language:      HLSL (High-Level Shader Language)
# Maintainer:    Maxim Kim <habamax@gmail.com>
# Last Change:   2026 Aug 11

if exists("b:did_ftplugin")
    finish
endif
b:did_ftplugin = 1

var undo_opts = "setl commentstring< comments<"

setlocal commentstring=//\ %s
setlocal comments=

if exists('b:undo_ftplugin')
    b:undo_ftplugin ..= "|" .. undo_opts
else
    b:undo_ftplugin = undo_opts
endif
