vim9script

# Config {{{1

const HELP: list<string> =<< trim END
    normal commands in regular window
    ─────────────────────────────────
    <CR>   change attribute of highlight group under cursor
    C      clear highlight group under cursor
    u      undo last change
    <C-R>  redo last change
    R      reload buffer
    r      rename highlight group under cursor
    D      duplicate highlight group under cursor
    p      put/create new highlight group
    K      open Vim help tag for highlight group under cursor
    g?     toggle this help

    normal commands in popup menu
    ─────────────────────────────
    +      add new attribute to highlight group
    -      remove attribute from highlight group

    Ex commands
    ───────────
    :Save     save all highlight groups attributes into given file
    :Restore  restore all highlight groups attributes from given file

    :ColorScheme   only display highlight groups relevant to a color scheme
    :ColorScheme!  display all groups again

    :ColorScheme save /path/to/script.vim
                 save the current colors as a Vim script
END

const LINK: string = '->'

const HL_PUM: dict<string> = {
    Equal: 'Identifier',
    Number: 'Number',
    Delimiter: 'Delimiter',
    Bool: 'Boolean',
    TermAttr: 'Type',
    LinkedGroup: 'Type',
}

const INTRO: list<string> =<< trim END
    press g? to toggle the help

END

# Init {{{1

var buf: number
var help_winid: number
var undolist: dict<any> = {states: [hlget()], pos: 0}
var want_colorscheme: bool
var winid: number
var written_files: list<string>

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

const term_attr: list<string> =<< trim END
    NONE
    bold
    italic
    nocombine
    reverse
    standout
    strikethrough
    undercurl
    underline
END

# `:help cterm-colors`
const color_names: list<string> =<< trim END
    Black
    Blue
    LightBlue
    Brown
    DarkYellow
    Cyan
    LightCyan
    DarkBlue
    DarkCyan
    DarkGray
    DarkGrey
    DarkGreen
    DarkMagenta
    DarkRed
    Green
    LightGreen
    LightGray
    LightGrey
    Gray
    Grey
    Magenta
    LightMagenta
    Red
    LightRed
    White
    Yellow
    LightYellow
    fg
    bg
    ul
    NONE
END

# `:help highlight-guifg`
# `:help gui-colors`
var gui_color_names: list<string> =<< trim END
    NONE
    bg
    background
    fg
    foreground
    Red
    LightRed
    DarkRed
    Green
    LightGreen
    DarkGreen
    SeaGreen
    Blue
    LightBlue
    DarkBlue
    SlateBlue
    Cyan
    LightCyan
    DarkCyan
    Magenta
    LightMagenta
    DarkMagenta
    Yellow
    LightYellow
    Brown
    DarkYellow
    Gray
    LightGray
    DarkGray
    Black
    White
    Orange
    Purple
    Violet
END
gui_color_names += v:colornames->keys()
lockvar gui_color_names

const attribute_names: list<string> =<< trim END
    term
    cterm
    gui
    ctermbg
    ctermfg
    ctermul
    guibg
    guifg
    guisp
    font
    linksto
END

const inputs: dict<list<string>> = {
    attribute: attribute_names,
    color: color_names,
    highlight: getcompletion('', 'highlight'),
    style: term_attr,
}

const pum_syntax_highlighting =<< trim eval END
    syntax clear

    syntax keyword HighlightTestBool true false
    syntax cluster HighlightTestValues contains=HighlightTestBool
    syntax cluster HighlightTestValues add=HighlightTestLinkedGroup
    syntax cluster HighlightTestValues add=HighlightTestNumber
    syntax match HighlightTestEqual /=/ nextgroup=@HighlightTestValues skipwhite
    syntax match HighlightTestLinkedGroup /\w\+/ contained
    syntax match HighlightTestNumber /\d\+/ contained
    syntax match HighlightTestNumber /#\x\+/ contained
    syntax match HighlightTestDelimiter /,/
    syntax match HighlightTestTermAttr /{term_attr->join('\|')}/
    syntax match HighlightTestFontName /font=\w\+/ transparent

    # We don't pass the `default` argument to `:highlight`, to be sure the
    # groups are restored after sth like `:highlight clear`.
    highlight link HighlightTestEqual {HL_PUM.Equal}
    highlight link HighlightTestNumber {HL_PUM.Number}
    highlight link HighlightTestDelimiter {HL_PUM.Delimiter}
    highlight link HighlightTestBool {HL_PUM.Bool}
    highlight link HighlightTestTermAttr {HL_PUM.TermAttr}
    highlight link HighlightTestLinkedGroup {HL_PUM.LinkedGroup}
END

# Interface {{{1
export def HighlightTest() #{{{2
    if !DidOpenNewWindow()
        return
    endif

    edit Highlight\ test
    &l:modifiable = true
    silent :% delete _
    if want_colorscheme
        ColorSchemeHandler()
    else
        SetHighlightGroups()
    endif
    &l:modifiable = false
    # needs to be  run *after* all the text  has been set, for the  latter to be
    # correctly folded
    SetOptionsAndInterface()
    normal! 1GzR
enddef

def FoldExpr(): string #{{{2
    if getline(v:lnum + 1) =~ '^---'
        return '>1'
    elseif v:lnum == 1 || v:lnum == 2
        return '0'
    else
        return '1'
    endif
enddef

def FoldText(): string #{{{2
    return getline(v:foldstart)
enddef

def Change() #{{{2
    var group: string = GroupUnderCursor()
    if !group->hlexists()
        return
    endif
    var attributes: list<string> = group->GetAttributesFromGroup()

    winid = attributes
        ->popup_atcursor({
            border: [],
            borderchars: ['─', '│', '─', '│', '┌', '┐', '┘', '└'],
            callback: function(ChangeAttribute, [group, attributes]),
            cursorline: true,
            filter: MenuFilter,
            highlight: 'Normal',
            mapping: false,
        })
    # Make sure Vim doesn't wait for a character.{{{
    #
    # Otherwise, after having  changed an attribute, if we press  `C-n` or `C-p`
    # to select  another one, the pum  is automatically closed.  But  only if we
    # press  the  key before  `&updatetime`  ms.   If  we wait  `&updatime`  ms,
    # `<CursorHold>`  is automatically  pressed which  seems to  have a  similar
    # effect as `<Ignore>`.
    #
    # Possibly relevant: https://github.com/vim/vim/issues/7011#issuecomment-700974772
    #}}}
    feedkeys("\<Ignore>", 'in')

    win_execute(winid, pum_syntax_highlighting)
enddef

def MenuFilter(_, key: string): bool #{{{2
    var group: string = expand('<cword>')

    # add a new attribute to the highlight group
    if key == '+'
        # ask which attribute should be added
        var Wrapper: func = (arglead, ..._) => Complete('attribute', arglead)
        var attr: string = input('attribute to add: ', '', $'custom,{Wrapper->string()}') | redraw

        if attribute_names->index(attr) == -1
            echo attr->printf('"%s" is not a valid attribute name')
            return true
        endif

        # update the popup
        var set_attributes: list<string> = winid
            ->winbufnr()
            ->getbufline(1, '$')
        set_attributes += [$'{attr}=']
        set_attributes->sort()->uniq()
        popup_settext(winid, set_attributes)
        var choice: number = set_attributes->match('=$') + 1
        win_execute(winid, $'normal! {choice}G')
        redraw

        # update the highlight group
        ChangeAttribute(group, set_attributes, 0, choice)
        popup_settext(winid, group->GetAttributesFromGroup())
        redraw
        return true

    # remove the selected attribute
    elseif key == '-'
        # grab selected attribute
        var to_remove: string = win_execute(winid, 'echo getline(".")', 'silent')
            ->trim()
            ->matchstr('[^=]\+')

        # remove the selected attribute
        var set_attributes: dict<any> = group
            ->hlget()
            ->get(0, {})
        set_attributes
            ->filter((attr: string, _): bool => attr != to_remove)
        # `hlset()` does not clear the group; it only sets attributes.
        # If we  want our  selected attribute  to be removed,  we first  need to
        # clear the group.
        execute $'highlight clear {group}'
        execute $'highlight link {group} NONE'
        # now, we can reset the group
        [set_attributes]->Hlset('change')


        # update the popup
        popup_settext(winid, group->GetAttributesFromGroup())
        redraw

        # the existing callback needs to know that the menu has lost 1 entry
        var Callback: func = function(ChangeAttribute, [group, group->GetAttributesFromGroup()])
        popup_setoptions(winid, {callback: Callback})

        # close the popup if the group has no attributes anymore
        if set_attributes->keys()->sort() == ['id', 'name']
            Reload()
            # `-1` disables the `ChangeAttribute()` callback
            popup_close(winid, -1)
        endif
        return true
    endif

    return popup_filter_menu(winid, key)
enddef

def Undo() #{{{2
    if undolist.pos == 0
        echo 'Highlight test: Already at oldest change'
        return
    endif

    --undolist.pos
    undolist.states
        ->get(undolist.pos)
        ->Hlset('undo')

    Reload()
    PrintStatesPos()
enddef

def Redo() #{{{2
    if undolist.pos == undolist.states->len() - 1
        echo 'Highlight test: Already at newest change'
        return
    endif

    ++undolist.pos
    undolist.states
        ->get(undolist.pos)
        ->Hlset('redo')

    Reload()
    PrintStatesPos()
enddef

def Clear() #{{{2
    var group: string = GroupUnderCursor()
    if !group->hlexists()
        return
    endif

    var cleared_group: dict<any> = group
        ->hlget()
        ->get(0, {})
        ->extend({cleared: true})
    [cleared_group]->Hlset('clear')
    Reload()
enddef

def Reload() #{{{2
    var view: dict<number> = winsaveview()
    HighlightTest()
    view->winrestview()
    normal! zv
enddef

def NewGroup(cmd: string) #{{{2
# - create new highlight group
# - duplicate highlight group under cursor
# - rename highlight group under cursor
    var group: string = GroupUnderCursor()
    if !group->hlexists()
        return
    endif

    var prompt: string = {
        create: 'name of created group',
        duplicate: 'name of duplicated group',
        rename: 'renaming group into',
    }[cmd]
    var newname: string = input($'{prompt}: ') | redraw
    if newname == ''
        return
    elseif newname !~ '^\w\+$'
        # `:help group-name`
        echo newname->printf('"%s" is not a valid name; only use word characters')
        return
    endif

    var hl: dict<any>
    if cmd == 'create'
        # It would be better to use `cleared: true` instead of `cterm: 'fg'`.{{{
        #
        # Unfortunately, it  would cause  `SetHighlightGroups()` to not  put the
        # group, because it checks that `IsCleared()` is false.  It does that to
        # remain  compatible  with  the  legacy script,  which  ignores  cleared
        # groups.
        #}}}
        hl = {name: newname, ctermfg: 'fg'}
    elseif cmd == 'duplicate' || cmd == 'rename'
        hl = group
            ->hlget()
            ->get(0, {})
            ->extend({name: newname})
    endif

    if cmd == 'rename'
        execute $'highlight clear {group}'
        execute $'highlight link {group} NONE'
    endif

    [hl]->Hlset(cmd)
    Reload()
    search($'^{newname}\_s')
    normal! zv
enddef

def Save(fname: string) #{{{2
    var object: list<string> = [hlget()->json_encode()]
    object->WriteFile(fname)
enddef

def Restore(fname: string) #{{{2
    var file: string = fname->expand()
    if !file->filereadable()
        echo file->printf('"%s" is not readable')
        return
    endif

    try
        file
            ->readfile()
            ->get(0, '')
            ->json_decode()
            ->Hlset('restore')
    catch
        Error(v:exception)
        return
    endtry

    Reload()
enddef

def ColorSchemeHandler(args = '', bang = false) #{{{2
# only print highlight groups which are relevant to a color scheme

    if args =~ '^save'
        args->matchstr('save\s*\zs.*')
            ->SaveColorsAsVimScript()
        return
    endif

    if bang
        want_colorscheme = false
        HighlightTest()
        return
    endif

    want_colorscheme = true

    &l:modifiable = true
    silent :% delete _

    var section_heading: list<string> =<< trim END
        Highlighting groups for a color scheme
        --------------------------------------
    END
    (INTRO + section_heading)->setline(1)

    var groups: list<string> = GetColorSchemeGroups()
    groups->append('$')
    # We  don't filter  out  non-existing/cleared groups,  because  it might  be
    # useful to know  about their existence, and set their  attributes for a new
    # color scheme which we want to develop.

    # give at least  1 attribute to a  non-existing group, so that  we can start
    # customizing it by pressing Enter
    for group: string in groups
        if !group->hlexists()
            [{name: group, cleared: true}]
                ->hlset()
        endif
    endfor

    execute 'silent! global /^\w\+$/ Highlight()'
    &l:modifiable = false
enddef

def VimHelp(): string #{{{2
    var group: string = GroupUnderCursor()
    if default_syntax_groups->index(group) >= 0
        return $'help group-name | search("{group}") | normal! zz'
    endif
    return $'help hl-{group}'
enddef

def ToggleHelp() #{{{2
    if help_winid == 0
        var height: number = [HELP->len(), winheight(0) * 2 / 3]->min()
        var longest_line: number = HELP
            ->copy()
            ->map((_, line: string) => line->strcharlen())
            ->max()
        var width: number = [longest_line, winwidth(0) * 2 / 3]->min()
        help_winid = HELP->popup_create({
            line: 2,
            col: &columns,
            pos: 'topright',
            minheight: height,
            maxheight: height,
            minwidth: width,
            maxwidth: width,
            border: [],
            borderchars: ['─', '│', '─', '│', '┌', '┐', '┘', '└'],
            highlight: 'Normal',
        })
        matchadd('Special', '^<\S\+\|^\S\{,2}  \@=', 0, -1, {window: help_winid})
        matchadd('Statement', '^:\S\+\%( \S\+\)\=', 0, -1, {window: help_winid})
        matchadd('Title', '^\w\{2,}.*', 0, -1, {window: help_winid})
    else
        if help_winid->IsVisible()
            popup_hide(help_winid)
        else
            popup_show(help_winid)
        endif
    endif
enddef
#}}}1
# Core {{{1
def DidOpenNewWindow(): bool #{{{2
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

def SetOptionsAndInterface() #{{{2
    # `:help scratch-buffer`
    &l:bufhidden = 'hide'
    &l:buftype = 'nofile'
    &l:swapfile = false

    &l:foldmethod = 'expr'
    &l:foldexpr = 'FoldExpr()'
    &l:foldtext = 'FoldText()'

    nnoremap <buffer><nowait> <CR> <ScriptCmd>Change()<CR>
    nnoremap <buffer><nowait> C <ScriptCmd>Clear()<CR>
    nnoremap <buffer><nowait> u <ScriptCmd>Undo()<CR>
    nnoremap <buffer><nowait> <C-R> <ScriptCmd>Redo()<CR>
    nnoremap <buffer><nowait> R <ScriptCmd>Reload()<CR>
    nnoremap <buffer><nowait> r <ScriptCmd>NewGroup('rename')<CR>
    nnoremap <buffer><nowait> D <ScriptCmd>NewGroup('duplicate')<CR>
    nnoremap <buffer><nowait> p <ScriptCmd>NewGroup('create')<CR>
    nnoremap <buffer><nowait> K <ScriptCmd>execute VimHelp()<CR>
    nnoremap <buffer><nowait> g? <ScriptCmd>ToggleHelp()<CR>

    command! -bar -buffer -complete=custom,CompleteFile -nargs=1 Save Save(<q-args>)
    command! -bar -buffer -complete=custom,CompleteFile -nargs=1 Restore Restore(<q-args>)
    command! -bang -bar -buffer -complete=custom,CompleteColorScheme -nargs=? ColorScheme {
        ColorSchemeHandler(<q-args>, <bang>0)
        # reset undolist
        undolist = {states: [hlget()], pos: 0}
    }
enddef

def SetHighlightGroups() #{{{2
    INTRO->setline(1)
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

    report->append('$')

    # highlight the group names
    buf = bufnr('%')
    execute $'silent! global /^\w\+\%(\%(\s*{LINK}\s*\)\w\+\)*$/ Highlight()'
enddef

def Highlight() #{{{2
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

def ChangeAttribute( #{{{2
    group: string,
    lines: list<string>,
    _: number,
    choice: number,
)
    if choice == -1
        return
    endif

    var attribute_to_change: string = lines
        ->get(choice - 1, '')
        ->matchstr('[^=]*')
    if attribute_to_change == ''
        return
    endif

    var new_value: any
    var old_value: any = group
        ->hlget()
        ->get(0, {})
        ->get(attribute_to_change, '')
    if ['term', 'cterm', 'gui']->index(attribute_to_change) >= 0
        var n: string = input('how many attributes in the new value? ', '1') | redraw
        if n !~ '^\d\+$'
            echo n->printf('"%s" is not a valid number')
            return
        elseif n->str2nr() > attribute_names->len()
            echo $'cannot set more than {attribute_names->len()} attributes'
            return
        endif
        new_value = {}
        for i in range(1, n->str2nr())
            var prompt: string = i->printf('attribute %s: ')
            var Wrapper: func = (arglead, ..._) => Complete('style', arglead)
            var input: string = input(prompt, '', $'custom,{Wrapper->string()}') | redraw
            if term_attr->index(input) == -1
                echo printf('"%s" is not a valid attribute for %s', input, attribute_to_change)
                return
            endif
            new_value->extend({[input]: true})
        endfor

    elseif ['ctermbg', 'ctermfg', 'ctermul']->index(attribute_to_change) >= 0
        var Wrapper: func = (arglead, ..._) => Complete('color', arglead)
        new_value = input('color: ', old_value, $'custom,{Wrapper->string()}') | redraw
        if new_value =~ '^\d\+$'
            var n: number = new_value->str2nr()
            if !(0 <= n && n < 256)
                echo printf('"%s" is not a valid color number for %s', new_value, attribute_to_change)
                return
            endif
        elseif color_names->index(new_value, 0, true) == -1
            echo printf('"%s" is not a valid color name for %s', new_value, attribute_to_change)
            return
        endif

    elseif ['guibg', 'guifg', 'guisp']->index(attribute_to_change) >= 0
        new_value = input('color: ', old_value) | redraw
        if new_value =~ '^#'
            if new_value =~ '^#\x\{6}$'
                # valid
            else
                echo printf('"%s" is not a valid color number for %s', new_value, attribute_to_change)
                return
            endif
        elseif gui_color_names->index(new_value, 0, true) == -1
            echo printf('"%s" is not a valid color name for %s', new_value, attribute_to_change)
            return
        endif

    elseif attribute_to_change == 'font'
        new_value = input('font: ', old_value) | redraw

    elseif attribute_to_change == 'linksto'
        var Wrapper: func = (arglead, ..._) => Complete('highlight', arglead)
        new_value = input('highlight group: ', old_value, $'custom,{Wrapper->string()}') | redraw
        if !new_value->hlexists()
            echo printf('"%s" is not an existing highlight group', new_value)
            return
        endif

    else
        return
    endif

    if new_value->empty()
        return
    endif

    var new_hl: dict<any> = group
            ->hlget()
            ->get(0, {})
            ->extend({[attribute_to_change]: new_value})

    # to be  able to set the  attributes of a cleared  group, when we work  on a
    # color scheme
    if new_hl->has_key('cleared')
        new_hl->remove('cleared')
    endif

    [new_hl]->Hlset('change')

    if attribute_to_change == 'linksto'
        Reload()
    endif

    # Re-open the popup menu, in case we want to change one more attribute.{{{
    #
    # For  some reason,  we need  a  timer.  I  guess  the popup  is not  closed
    # immediately when the callback is invoked.
    #}}}
    timer_start(0, (_) => ReOpenMenu())
enddef

def ReOpenMenu() #{{{2
    # Need this test because we don't want to re-open the menu when we press `+`/`-`.
    # There is no need to, and it would cause issues.
    if !winid->IsVisible()
        Change()
    endif
enddef

def Hlset(state: list<dict<any>>, cmd: string) #{{{2
    # clear all the groups  before setting a *set* of highlights
    if ['undo', 'redo', 'restore']->index(cmd) >= 0
        hlget()
            ->map((_, group: dict<any>) => group->extend({
                cleared: true,
                linksto: 'NONE',
                force: true
            }))->hlset()
    endif

    # set the highlight(s) from the given state
    state
        ->map((_, d: dict<any>) => d
            ->extend(d->has_key('linksto') ? {force: true} : {})
            ->extend(d->has_key('cleared') ? {linksto: 'NONE', force: true} : {})
        )->hlset()

    if cmd == 'undo' || cmd == 'redo'
        return
    endif

    if undolist.pos < undolist.states->len() - 1
        undolist.states->remove(undolist.pos + 1, undolist.states->len() - 1)
    endif
    undolist.states += [hlget()]
    undolist.pos = undolist.states->len() - 1
enddef

def PrintStatesPos() #{{{2
    var curpos: number = undolist.pos + 1
    var maxpos: number = undolist.states->len()
    var numwidth: number = maxpos->len()
    redraw
    echo printf('Highlight test: [%0*d/%d]', numwidth, curpos, maxpos)
enddef

def WriteFile(object: list<string>, fname: string) #{{{2
    var file: string = fname->expand()
    if file->filereadable()
        var prompt: string = file->printf('overwrite existing file "%s"? y/n ')
        var YesNo: func = (..._) => ['yes', 'no']->join("\n")
        var answer: string = input(prompt, '', $'custom,{YesNo->string()}') | redraw
        if ['y', 'yes']->index(answer, 0, true) == -1
            echo 'cancelled'
            return
        endif
    endif

    try
        object->writefile(file)
    catch
        Error(v:exception)
        return
    endtry
    written_files += [file]
enddef

def SaveColorsAsVimScript(fname: string) #{{{2
    if fname->empty()
        echo 'usage:  :ColorScheme save /path/to/script.vim'
        return
    endif

    var colorscheme_name: string = input('name of the new color scheme: ') | redraw
    if colorscheme_name == ''
        return
    elseif colorscheme_name !~ '^\w\+$'
        echo colorscheme_name->printf('"%s" is not a valid name; only use word characters')
        return
    endif

    var script: list<string> =<< trim eval ENDD
        set background={&background}

        hi clear
        let g:colors_name = '{colorscheme_name}'

        if (has('termguicolors') && &termguicolors) || has('gui_running')
            " TODO: see :help g:terminal_ansi_colors
            let g:terminal_ansi_colors =<< trim END
            END
        endif

    ENDD

    var colorscheme_groups: list<string> = GetColorSchemeGroups()

    for group: string in colorscheme_groups
        var hl: dict<any> = group->hlget()->get(0, {})
        # only keep existing highlight groups, relevant to a color scheme
        if hl->empty()
        || hl->has_key('cleared')
        || colorscheme_groups->index(hl.name) == -1
            continue
        endif

        var tokens: list<string>
        if hl->has_key('linksto')
            tokens += [
                'hi!',
                'link',
                hl.name,
                hl->has_key('linksto') ? hl.linksto : '',
            ]
        else
            tokens += [
                'hi',
                hl.name,
                hl->has_key('term') ? $'term={hl.term->keys()->join(",")}' : '',
                hl->has_key('cterm') ? $'cterm={hl.cterm->keys()->join(",")}' : '',
                hl->has_key('gui') ? $'gui={hl.gui->keys()->join(",")}' : '',
                hl->has_key('ctermbg') ? $'ctermbg={hl.ctermbg}' : '',
                hl->has_key('ctermfg') ? $'ctermfg={hl.ctermfg}' : '',
                hl->has_key('ctermul') ? $'ctermul={hl.ctermul}' : '',
                hl->has_key('guibg') ? $'guibg={hl.guibg}' : '',
                hl->has_key('guifg') ? $'guifg={hl.guifg}' : '',
                hl->has_key('guisp') ? $'guisp={hl.guisp}' : '',
                hl->has_key('font') ? $'guisp={hl.font}' : '',
            ]
        endif
        script += [
            tokens
            ->join(' ')
            ->substitute('\s\{2,}', ' ', 'g')
            ->trim()
        ]
    endfor

    script->WriteFile(fname)
enddef
#}}}1
# Util {{{1
def IsCleared(name: string): bool #{{{2
    return name
        ->hlget()
        ->get(0, {})
        ->get('cleared')
enddef

def IsVisible(win: number): bool #{{{2
    return win->popup_getpos()->get('visible')
enddef

def FollowChains(groups: list<string>): list<string> #{{{2
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
            ->split($'\s*${LINK}\s*')
            ->map((_, g: string) => g->IsCleared())
            ->index(true) >= 0
        if a_link_is_cleared
            continue
        endif
        chains += [chain]
    endfor
    return chains
enddef

def LinksTo(group: string): string #{{{2
    return group
        ->hlget()
        ->get(0, {})
        ->get('linksto', '')
enddef

def GroupUnderCursor(): string #{{{2
    if getline('.')->matchstr('\%.c.') !~ '\w'
        return ''
    endif
    return expand('<cword>')
enddef

def GetVariousGroups(): list<string> #{{{2
    var various_groups: list<string> = getcompletion('hl-', 'help')
        ->filter((_, helptag: string): bool => helptag =~ '^hl-\w\+$')
        ->map((_, helptag: string) => helptag->substitute('^hl-', '', ''))

    various_groups += range(1, 9)
        ->map((_, n: number) => $'User{n}')

    return various_groups
enddef

def GetColorSchemeGroups(): list<string> #{{{2
    var groups: list<string> = GetVariousGroups()
            ->filter((_, group: string): bool => group !~ '^User[1-9]$')
    groups += default_syntax_groups
    return groups
        ->sort()
        ->uniq()
enddef

def GetAttributesFromGroup(group: string): list<string> #{{{2
    var hl: dict<any> = group
        ->hlget()
        ->get(0, {})

    return hl
        ->keys()
        ->filter((_, attr: string): bool => attr != 'id' && attr != 'name')
        ->sort()
        ->map((_, attr: string) => $'{attr}={hl[attr]->GetAttributeValue()}')
enddef

def GetAttributeValue(attr: any): string #{{{2
    var type: string = attr->typename()
    if type == 'string'
        return attr
    elseif type == 'bool'
        return attr->string()
    elseif type =~ '^dict'
        return attr
            ->filter((_, v: bool): bool => v)
            ->keys()
            ->join(',')
    endif
    return ''
enddef

def Error(msg: string) #{{{2
    redraw
    echohl ErrorMsg
    echomsg msg
    echohl NONE
enddef

def Complete(kind: string, arglead: string): string #{{{2
    var relevant_inputs: list<string> = inputs
        ->get(kind, [])
        ->copy()

    # filter out attributes which are already present in the popup menu
    if kind == 'attribute'
        var ignorelist: list<string> = winid
            ->winbufnr()
            ->getbufline(1, '$')
            ->map((_, attr: string) => attr->matchstr('[^=]\+'))
        relevant_inputs
            ->filter((_, input: string): bool => ignorelist->index(input) == -1)
    endif

    return relevant_inputs
        ->filter((_, input: string): bool => input->stridx(arglead) == 0)
        ->join("\n")
enddef

def CompleteFile(arglead: string, ..._): string #{{{2
    return (
        expand('%:p:h')->readdir()
        + written_files
    )->sort()
     ->uniq()
     ->join("\n")
enddef

def CompleteColorScheme( #{{{2
    arglead: string,
    cmdline: string,
    pos: number
): string
    if cmdline =~ $'ColorScheme\s\+\S*\%{pos + 1}c'
        return 'save'
    endif
    return arglead->CompleteFile()
enddef
