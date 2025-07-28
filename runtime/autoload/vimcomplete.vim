vim9script

# Vim completion script
# Language:    Vimscript
# Maintainer:  Maxim Kim <habamax@gmail.com>
# Last Change: 2025-07-28
#
# Usage:
# setlocal omnifunc=vimcomplete#Complete
#
# Simple complete function for the Vimscript

var trigger: string = ""

def GetTrigger(line: string): list<any>
    var result = ""
    var result_len = 0
    if line =~ '->\k*$' || line =~ '\vcall\s+\k*$'
        result = 'func'
    elseif line =~ '\v%(^|\s+)\&\k*$' || line =~ '\vset%(\s+\k*)*$'
        result = 'option'
    elseif line =~ '\vecho%[msg]\s+\k*$' || line =~ '[\[(]\s*$'
        result = 'expr'
    elseif line =~ '[lvgsb]:'
        result = 'var'
        result_len = 2
    elseif line =~ '\vau%[tocmd]\s+\k*$'
        result = 'event'
    elseif line =~ '\vhi%[ghlight]!?\s+%(def%[ault]\s+)?link\s+(\k+\s+)?\k*$'
        result = 'highlight_def_link'
    elseif line =~ '\vhi%[ghlight]!?\s+def%[ault]\s+\k*$'
        result = 'highlight_def'
    elseif line =~ '\vhi%[ghlight]!?\s+%(def%[ault]\s+)?\k+%(\s+\k+\=%(\k+,?)+)*%(\s+%(cterm|gui)\=)%(\k+,)*\k*$'
        result = 'highlight_attr_noncolor'
    elseif line =~ '\vhi%[ghlight]!?\s+%(def%[ault]\s+)?\k+%(\s+\k+\=%(\k+,?)+)*%(\s+cterm[fb]g\=)\k*$'
        result = 'highlight_attr_color_cterm'
    elseif line =~ '\vhi%[ghlight]!?\s+%(def%[ault]\s+)?\k+%(\s+\k+\=%(\k+,?)+)*%(\s+gui[fb]g\=)\k*$'
        result = 'highlight_attr_color_gui'
    elseif line =~ '\vhi%[ghlight]!?\s+%(def%[ault]\s+)?\k+%(\s+\k+\=%(\k+,?)+)*\s+\k*$'
        result = 'highlight_attr'
    elseif line =~ '\vhi%[ghlight]!?\s+\k*$'
        result = 'highlight'
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
        return line->len() - keyword->len() - trigger_len
    endif

    var funcs = getcompletion(base, 'function')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Function', dup: 0}))
    var exprs = getcompletion(base, 'expression')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Expression', dup: 0}))
    var commands = getcompletion(base, 'command')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Command', dup: 0}))
    var options = getcompletion(base, 'option')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Option', dup: 0}))
    var vars = getcompletion(base, 'var')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Variable', dup: 0}))
    var events = getcompletion(base, 'event')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Autocommand event', dup: 0}))
    var highlights = getcompletion(base, 'highlight')
        ->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Highlight group', dup: 0}))

    var items = []
    if trigger == 'func'
        items = funcs
    elseif trigger == 'option'
        items = options
    elseif trigger == 'var'
        items = vars
    elseif trigger == 'expr'
        items = exprs
    elseif trigger == 'event'
        items = events
    elseif trigger == 'highlight_def_link'
        items = highlights
    elseif trigger == 'highlight_def'
        items = [
            {word: 'link', kind: 'v', menu: 'Link first highlight group to the second one.'}
        ] + highlights
    elseif trigger == 'highlight'
        items = [
            {word: 'default', kind: 'v', menu: 'Set default highlighting.'},
            {word: 'link', kind: 'v', menu: 'Link first highlight group to the second one.'}
        ] + highlights
    elseif trigger == 'highlight_attr'
        items = [
            'gui', 'cterm', 'guibg', 'ctermbg', 'guifg', 'ctermfg',
        ]->mapnew((_, v) => ({word: v, kind: 'v', menu: 'Highlight group attribute', dup: 0}))
    elseif trigger == 'highlight_attr_noncolor'
        items = [
            'bold', 'italic', 'underline', 'NONE', 'reverse',
            'undercurl', 'underdouble', 'underdouble', 'strikethrough', 'standout'
        ]->mapnew((_, v) => ({word: v, kind: 'v', menu: 'gui= or term= value', dup: 0}))
    elseif trigger == 'highlight_attr_color_cterm'
        items = [
            'black', 'white', 'red', 'darkred', 'green', 'darkgreen', 'yellow', 'darkyellow',
            'blue', 'darkblue', 'magenta', 'darkmagenta', 'cyan', 'darkcyan', 'gray', 'darkgray'
        ]
    elseif trigger == 'highlight_attr_color_gui'
        items = v:colornames->keys()
            ->mapnew((_, v) => ({word: v:colornames[v], kind: 'v', menu: $"Color: {v}", dup: 0}))
    elseif !empty(base)
        items = commands->extend(funcs)
    endif

    return items->empty() ? v:none : items
enddef
