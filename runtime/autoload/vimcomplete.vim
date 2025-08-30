vim9script

# Vim completion script
# Language:    Vim script
# Maintainer:  Maxim Kim <habamax@gmail.com>
# Last Change: 2025-08-27
#
# Usage:
# setlocal omnifunc=vimcomplete#Complete
#
# Simple complete function for Vim script

var trigger: string = ""
var prefix: string = ""


def GetTrigger(line: string): list<any>
    var result = ""
    var result_len = 0

    if line =~ '->\k*$'
        result = 'function'
    elseif line =~ '\v%(^|\s+)\&\k*$'
        result = 'option'
    elseif line =~ '[\[(]\s*$'
        result = 'expression'
    elseif line =~ '[lvgsb]:\k*$'
        result = 'var'
        result_len = 2
    else
        result = getcompletiontype(line) ?? 'cmdline'
    endif
    return [result, result_len]
enddef

export def Complete(findstart: number, base: string): any
    if findstart > 0
        var line = getline('.')->strpart(0, col('.') - 1)
        var keyword = line->matchstr('\k\+$')
        var stx = synstack(line('.'), col('.') - 1)->map('synIDattr(v:val, "name")')->join()
        if stx =~? 'Comment' || (stx =~ 'String' && stx !~ 'vimStringInterpolationExpr')
            return -2
        endif
        var trigger_len: number = 0
        [trigger, trigger_len] = GetTrigger(line)
        if keyword->empty() && trigger->empty()
            return -2
        endif
        prefix = line
        return line->len() - keyword->len() - trigger_len
    endif

    var items = []
    if trigger == 'function'
        items = getcompletion(base, 'function')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Function', dup: 0}))
    elseif trigger == 'option'
        items = getcompletion(base, 'option')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Option', dup: 0}))
    elseif trigger == 'var'
        items = getcompletion(base, 'var')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Variable', dup: 0}))
    elseif trigger == 'expression'
        items = getcompletion(base, 'expression')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Expression', dup: 0}))
    elseif trigger == 'command'
        var commands = getcompletion(base, 'command')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Command', dup: 0}))
        var functions = getcompletion(base, 'function')
            ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Function', dup: 0}))
        items = commands + functions
    else
        try
            items = getcompletion(prefix, 'cmdline')
                ->mapnew((_, v) => ({word: v->matchstr('\k\+'), kind: 'v', dup: 0}))
        catch /E220/
        endtry

        if empty(items) && !empty(base)
            items = getcompletion(base, 'expression')
                ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Expression', dup: 0}))
        endif
    endif

    return items->empty() ? v:none : items
enddef
