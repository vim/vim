vim9script

# Init {{{1

const LINK: string = '->'

var buf: number

# `:help group-name`
const default_syntax_groups: list<string> =<< trim END
    Comment
    Constant
    String
    Character
    Number
    Boolean
    Float
    Identifier
    Function
    Statement
    Conditional
    Repeat
    Label
    Operator
    Keyword
    Exception
    PreProc
    Include
    Define
    Macro
    PreCondit
    Type
    StorageClass
    Structure
    Typedef
    Special
    SpecialChar
    Tag
    Delimiter
    SpecialComment
    Debug
    Underlined
    Ignore
    Error
    Todo
END

# Interface {{{1
export def HighlightTest() # {{{2
    if !DidOpenNewWindow()
        return
    endif

    edit Highlight\ test
    &l:modifiable = true
    silent :% delete _
    SetHighlightGroups()
    &l:modifiable = false
    # needs to be  run *after* all the text  has been set, for the  latter to be
    # correctly folded
    SetOptionsAndMapping()
    normal! 1GzR
enddef

def FoldExpr(): string # {{{2
    if getline(v:lnum + 1) =~ '^---'
        return '>1'
    elseif v:lnum == 1 || v:lnum == 2
        return '0'
    else
        return '1'
    endif
enddef

def FoldText(): string # {{{2
    return getline(v:foldstart)
enddef

def VimHelp(): string # {{{2
    var group: string = GroupUnderCursor()
    if default_syntax_groups->index(group) >= 0
        return $'help group-name | search("{group}") | normal! zz'
    endif
    return $'help hl-{group}'
enddef
# }}}1
# Core {{{1
def DidOpenNewWindow(): bool # {{{2
    # we don't try to handle a second "Highlight test" buffer; too confusing
    if expand('%') != 'Highlight test' && buflisted('Highlight test')
        var b: number = bufnr('Highlight test')
        var winids: list<number> = bufnr('Highlight test')->win_findbuf()
        if !winids->empty()
            winids->get(0)->win_gotoid()
        else
            execute $'buffer {b}'
        endif
        return false
    endif

    # open a new window if the current one isn't empty
    var has_no_name: bool = expand('%') == ''
    var is_empty: bool = (line('$') + 1)->line2byte() <= 2
    var is_highlight: bool = expand('%') == 'Highlight test'
    if !(is_highlight || has_no_name && is_empty)
        new
    endif
    return true
enddef

def SetOptionsAndMapping() # {{{2
    # `:help scratch-buffer`
    &l:bufhidden = 'hide'
    &l:buftype = 'nofile'
    &l:swapfile = false

    &l:foldmethod = 'expr'
    &l:foldexpr = 'FoldExpr()'
    &l:foldtext = 'FoldText()'

    nnoremap <buffer><nowait> K <ScriptCmd>execute VimHelp()<CR>
enddef

def SetHighlightGroups() # {{{2
    var report: list<string> =<< trim END
        Highlighting groups for various occasions
        -----------------------------------------
    END

    var various_groups: list<string> = GetVariousGroups()
        ->filter((_, group: string): bool => group->hlexists() && !group->IsCleared())
        ->sort()
        ->uniq()

    report += various_groups->FollowChains()

    var language_section: list<string> =<< trim END

        Highlighting groups for language syntaxes
        -----------------------------------------
    END
    report += language_section

    var syntax_groups: list<string> = getcompletion('', 'highlight')
        ->filter((_, group: string): bool =>
            various_groups->index(group) == -1
            && !group->IsCleared()
            && group !~ '^HighlightTest')
    report += syntax_groups->FollowChains()

    report->setline(1)

    # highlight the group names
    buf = bufnr('%')
    execute $'silent! global /^\w\+\%(\%(\s*{LINK}\s*\)\w\+\)*$/ Highlight()'
enddef

def Highlight() # {{{2
    var lnum: number = line('.')
    for group: string in getline('.')->split($'\s*{LINK}\s*')
        silent! prop_type_add($'highlight-test-{group}', {
            bufnr: buf,
            highlight: group,
            combine: false,
        })
        prop_add(lnum, col('.'), {
            length: group->strlen(),
            type: $'highlight-test-{group}'
        })
        search('\<\w\+\>', '', lnum)
    endfor
enddef

# }}}1
# Util {{{1
def IsCleared(name: string): bool # {{{2
    return name
        ->hlget()
        ->get(0, {})
        ->get('cleared')
enddef

def FollowChains(groups: list<string>): list<string> # {{{2
    # A group might be linked to another, which itself might be linked...
    # We want the whole chain, for every group.
    var chains: list<string>
    for group: string in groups
        var target: string = group->LinksTo()
        var chain: string = group
        while !target->empty()
            chain ..= $' {LINK} {target}'
            target = target->LinksTo()
        endwhile
        var a_link_is_cleared: bool = chain
            ->split($'\s*{LINK}\s*')
            ->indexof((_, g: string): bool => g->IsCleared()) >= 0
        if a_link_is_cleared
            continue
        endif
        chains->add(chain)
    endfor
    return chains
enddef

def LinksTo(group: string): string # {{{2
    return group
        ->hlget()
        ->get(0, {})
        ->get('linksto', '')
enddef

def GroupUnderCursor(): string # {{{2
    if getline('.')->matchstr('\%.c.') !~ '\w'
        return ''
    endif
    return expand('<cword>')
enddef

def GetVariousGroups(): list<string> # {{{2
    var various_groups: list<string> = getcompletion('hl-', 'help')
        ->filter((_, helptag: string): bool => helptag =~ '^hl-\w\+$')
        ->map((_, helptag: string) => helptag->substitute('^hl-', '', ''))

    various_groups += range(1, 9)
        ->map((_, n: number) => $'User{n}')

    return various_groups
enddef
