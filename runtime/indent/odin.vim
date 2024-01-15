vim9script

# Vim indent plugin file
# Language: Odin
# Maintainer: Maxim Kim <habamax@gmail.com>
# Website: https://github.com/habamax/vim-odin
# Last Change: 2024-01-15

if exists("b:did_indent")
    finish
endif
b:did_indent = 1

b:undo_indent = 'setlocal cindent< cinoptions< cinkeys< indentexpr<'

setlocal cindent
setlocal cinoptions=L0,m1,(s,j1,J1,l1,+0,:0,#1
setlocal cinkeys=0{,0},0),0],!^F,:,o,O

setlocal indentexpr=GetOdinIndent(v:lnum)

def GetOdinIndent(lnum: number): number
    var plnum = prevnonblank(lnum - 1)

    var pline = getline(plnum)
    var line = getline(lnum)

    var indent = indent(plnum)

    if line =~ '^\s*#\k\+'
        if pline =~ '[{:]\s*$'
            return indent + shiftwidth()
        else
            return indent
        endif
    elseif pline =~ 'case:\s*$'
        return indent + shiftwidth()
    elseif pline =~ '{[^{]*}\s*$' # https://github.com/habamax/vim-odin/issues/2
        return indent
    elseif pline =~ '}\s$' # https://github.com/habamax/vim-odin/issues/3
        # Find line with opening { and check if there is a label:
        # If there is, return indent of the closing }
        silent! $":{plnum}"
        silent! $"$F}%"
        if plnum != line('.') && getline('.') =~ '^\s*\k\+:'
            return indent
        endif
    endif

    return cindent(lnum)
enddef
