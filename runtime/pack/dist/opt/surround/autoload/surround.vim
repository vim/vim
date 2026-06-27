vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2026-06-27

var base_pairs = {
    'b': ('(', ')'), '(': ('( ', ' )'), ')': ("\n(", ')'),
    'B': ('{', '}'), '{': ('{ ', ' }'), '}': ("\n{", '}'),
    '[': ('[ ', ' ]'), ']': ("\n[", ']'),
    '<': ('< ', ' >'), '>': ("\n<", '>'),
    '"': ("\n\"", '"'), "'": ("\n'", "'"), "`": ("\n`", "`"),
    '*': ("\n*", '*'), '_': ("\n_", '_'), '/': ("\n/", '/'),
}

extend(base_pairs, get(g:, "surround_pairs", {}))

def Pairs(): dict<any>
    return extendnew(base_pairs, get(b:, "surround_pairs", {}))
enddef

# Surround/Remove surround with.
var s_with: string = ''
# Change surround with.
var c_with: string = ''
# If block selection is done with $
var visual_dollar: bool = false
# Filetypes with indent script to fix indent after surround
var filetypes = []

# To prevent asking for surround char in every repetition of dot command, e.g.
# ysiw( followed by . should surround with ( as well, not ask for char again.
var dotrepeat = false

# save view before Adding surround, to restore if Add operation is canceled with
# <ESC>
var cancel_view = {}

export def Add(): string
    if !&l:modifiable
        echohl ErrorMsg
        echomsg "E21: Cannot make changes, 'modifiable' is off"
        echohl NONE
        return ''
    endif
    dotrepeat = false
    cancel_view = winsaveview()
    visual_dollar = getcursorcharpos()[-1] == v:maxcol
    &opfunc = (mode) => AddSurround(mode)
    return 'g@'
enddef

export def Remove(): string
    if !&l:modifiable
        echohl ErrorMsg
        echomsg "E21: Cannot make changes, 'modifiable' is off"
        echohl NONE
        return ''
    endif
    var char = getcharstr(-1, {cursor: 'keep'})
    if char == "\<Esc>" || char == "\<CR>"
        return ''
    endif
    s_with = char
    &opfunc = (_) => RemoveSurround()
    return 'g@l'
enddef

export def Change(): string
    if !&l:modifiable
        echohl ErrorMsg
        echomsg "E21: Cannot make changes, 'modifiable' is off"
        echohl NONE
        return ''
    endif
    var char = getcharstr(-1, {cursor: 'keep'})
    if char == "\<Esc>" || char == "\<CR>"
        return ''
    endif
    s_with = char

    char = getcharstr(-1, {cursor: 'keep'})
    if char == "\<Esc>" || char == "\<CR>"
        return ''
    endif
    if char == "t"
        var tag  = input("Tag: ")
        if empty(trim(tag))
            return ''
        else
            c_with = '<' .. trim(trim(tag), '<>') .. '>'
        endif
    else
        c_with = char
    endif
    &opfunc = (_) => ChangeSurround()
    return 'g@l'
enddef

def ShouldIndent(): bool
    if empty(filetypes)
        filetypes = globpath(&rtp, 'indent/*.vim', 0, 1)
            ->mapnew((_, v) => fnamemodify(v, ':t:r'))
    endif
    return filetypes->index(&filetype) != -1
enddef

def AddSurround(mode: string, pos_start: list<number> = getcharpos("'["), pos_end: list<number> = getcharpos("']"))
    var save_lazyredraw = &lazyredraw
    var save_virtualedit = &l:virtualedit
    var save_indentkeys = &l:indentkeys
    var save_autoindent = &l:autoindent
    var save_comments = &l:comments
    set lazyredraw
    setlocal virtualedit=block
    setlocal indentkeys=
    setlocal autoindent
    setlocal comments=
    defer () => {
        &lazyredraw = save_lazyredraw
        &l:virtualedit = save_virtualedit
        &l:indentkeys = save_indentkeys
        &l:autoindent = save_autoindent
        &l:comments = save_comments
    }()

    if !dotrepeat
        dotrepeat = true
        var char = getcharstr(-1, {cursor: 'keep'})
        if char == "\<Esc>" || char == "\<CR>"
            winrestview(cancel_view)
            return
        endif
        if char == "t"
            var tag  = input("Tag: ")
            if empty(trim(tag))
                return
            else
                s_with = '<' .. trim(trim(tag), '<>') .. '>'
            endif
        else
            s_with = char
        endif
    endif

    var pairs = Pairs()

    var start = pos_start
    var end = pos_end

    # Handle case with v$S(. when selection is started in the middle of the
    # line. Start/end positions are incorrect in dot-repeating.
    var end_len = strcharlen(getline(end[1]))
    if visual_dollar && start[1] == end[1] && start[2] == 1 && end[2] != end_len
        start = deepcopy(end)
        end[2] = end_len
    endif

    var s_left = ''
    var s_right = ''
    if s_with =~ '^<.*>$'
        s_left = s_with
        s_right = '</' .. s_with[1 : -2]->split()[0] .. '>'
    else
        var pair = get(pairs, s_with, ())
        if empty(pair) && s_with !~ '[[:punct:][:blank:]]'
            return
        endif
        s_left = empty(pair) ? s_with : pair[0]
        s_right = empty(pair) ? s_with : pair[1]
    endif

    var s_tab = s_with == "\<tab>" ? "\<C-v>" : ''

    # For a single line surround
    # - ( [ { < surround with newlines
    # - ``` and """ surrounds with newlines
    # - <tag> surrounds with newlines
    # - others surround line without newlines
    var s_mode = mode
    if mode == 'line' && start[1] == end[1] && s_left[-1] !~ '[> ]' && s_left[-1] != "\n"
        s_mode = 'char'
        noautocmd normal! _
        start = getcursorcharpos()
        noautocmd normal! g_
        end = getcursorcharpos()
    elseif mode == 'line' && s_left[0] == "\n"
        s_mode = 'char'
        noautocmd normal! _
        start = getcursorcharpos()
        setcharpos('.', end)
        noautocmd normal! g_
        end = getcharpos('.')
    elseif mode == 'line'
        s_left = trim(s_left)
        s_right = trim(s_right)
    endif

    s_left = trim(s_left, "\n")

    if s_mode == 'char'
        setlocal virtualedit=all
        setcharpos('.', start)
        if col('.') == col('$')
                || getline('.') =~ '^\s*$'
                || getline('.')[col('.') - 1] =~ '\s'
            s_left = trim(s_left)
        endif
        exe $"noautocmd normal! i{s_tab}{s_left}"
        setlocal virtualedit=none
        if start[1] == end[1]
            end[2] += strchars(s_left)
        endif
        start[2] += strchars(s_left)
        setcharpos('.', end)
        if getline('.') =~ '^\s*$' || getline('.')[col('.') - 1] =~ '\s'
            s_right = trim(s_right)
        endif
        if empty(getline(end[1]))
            setline(end[1], s_right)
        else
            exe $"noautocmd normal! a{s_tab}{s_right}"
        endif
        setcharpos('.', start)
    elseif s_mode == 'line'
        exe $":noautocmd :{start[1]}normal! O{s_left}"
        exe $":noautocmd :{end[1]}normal! jo{s_right}"
        if (s_left =~ '[([{]' || s_right =~ '</.\{-}>')
            && ShouldIndent()
            exe $":{start[1]}"
            exe $":silent noautocmd normal! {end[1] - start[1] + 2}=="
        endif
        exe $":{start[1] + 1}"
        exe ":noautocmd normal! _"
    elseif s_mode == "block"
        if visual_dollar
            for nr in range(start[1], end[1])
                if strchars(getline(nr)) >= start[2]
                    setcursorcharpos(nr, start[2])
                    var squeeze = ""
                    if getline(nr)[ : start[2] - 1] =~ '^\s*$'
                        squeeze = "_"
                    endif
                    exe $"noautocmd normal! {squeeze}\<C-v>$"
                    exe $"noautocmd normal! I{s_tab}{s_left}"
                    exe "noautocmd normal! \<C-v>$"
                    exe $"noautocmd normal! A{s_tab}{s_right}"

                endif
            endfor
            setcursorcharpos(start[1 :])
            exe "noautocmd normal! \<C-v>"
            setcursorcharpos(end[1 :])
            exe "noautocmd normal! $\<ESC>"
            start[2] += strchars(s_left)
            setcursorcharpos(start[1 :])
        else
            # better undo -- first change should be in the block begining
            # Add letter X and then delete it.
            noautocmd normal! iX
            noautocmd normal! x

            noautocmd normal! gv
            var v_pos = getregionpos(getpos("v"), getpos('.'), {mode: visualmode()})
            var v_start = v_pos[0][0]
            # var v_end = v_pos[-1][0]
            if v_start[1 : ] != start[1 : ]
                exe "noautocmd normal! \<ESC>"
                setlocal virtualedit=all
                # setcursorcharpos(...) can't navigate to an empty location
                MoveCursor(end[1], end[2] + end[3])
                if strchars(getline(end[1])) < end[2] + end[3]
                    exe "noautocmd normal! i\<space>"
                endif
                exe "noautocmd normal! \<C-v>"
                MoveCursor(start[1], start[2] + start[3])
            endif
            exe $"noautocmd normal! A{s_tab}{s_right}"
            noautocmd normal! gv
            exe $"noautocmd normal! I{s_tab}{s_left}"

            setcursorcharpos(end[1], end[2] + end[3] + strchars(s_left))
            exe "noautocmd normal! \<C-v>"
            setcursorcharpos(start[1], start[2] + start[3] + strchars(s_left))
            exe "noautocmd normal! \<ESC>"
        endif
    endif
enddef

def RemoveSurround(delete_empty_lines: bool = true): list<list<number>>
    var save_clipboard = &clipboard
    var save_virtualedit = &l:virtualedit
    set clipboard=
    setlocal virtualedit=none
    defer () => {
        &clipboard = save_clipboard
        &l:virtualedit = save_virtualedit
    }()
    var view = winsaveview()
    var cursor = getcursorcharpos()
    var s_left = ""
    var s_right = ""
    var start = []
    var end = []
    var pairs = Pairs()
    if s_with == 's'
        var pos_list = []

        # # get deduplicated chars from pairs
        # # while this works, I would leave the explicit list of chairs for
        # # better control and performance
        # # TODO: remove "duplicates" as ' and ''' or " and """
        # var pair_chars = pairs->items()
        #     ->mapnew((_, v) => [v[0], trim(v[1][0]) .. trim(v[1][1])])
        #     ->sort((v1, v2) => v1[1] == v2[1] ? 0 : v1[1] > v2[1] ? 1 : -1)
        #     ->uniq((v1, v2) => v1[1] == v2[1] ? 0 : v1[1] > v2[1] ? 1 : -1)
        #     ->mapnew((_, v) => v[0])

        var pair_chars = '({["`''*_|/'

        for char in pair_chars
            var pair = get(pairs, char, ())
            s_left = empty(pair) ? char : trim(pair[0])
            s_right = empty(pair) ? char : trim(pair[1])
            [start, end] = ProbePair(s_left, s_right)
            if !empty(start) && !empty(end)
                add(pos_list, [start, end, s_left, s_right])
            endif
        endfor
        if empty(pos_list)
            return []
        endif
        [start, end, s_left, s_right] = pos_list->sort((v1, v2) => {
            if v1[0][1] == v2[0][1]
                return v1[0][2] == v2[0][2] ? 0 : v1[0][2] > v2[0][2] ? 1 : -1
            elseif v1[0][1] > v2[0][1]
                return 1
            else
                return -1
            endif
        })[-1]
    elseif s_with == 't'
        [start, end, s_left, s_right] = ProbeTag()
    else
        var pair = get(pairs, s_with, ())
        s_left = empty(pair) ? s_with : trim(pair[0])
        s_right = empty(pair) ? s_with : trim(pair[1])
        [start, end] = ProbePair(s_left, s_right)
    endif

    if empty(start) || empty(end)
        winrestview(view)
        return []
    endif

    setcharpos('.', start)
    var indent_lines = end[0] - start[0]

    if start[1] == cursor[1] && end[1] == cursor[1]
        end[2] -= strchars(s_left)
    endif
    if delete_empty_lines && getline('.') =~ $'\V\^\s\*{escape(s_left, '\')}\$'
        noautocmd normal! "_dd
        end[1] -= 1
    else
        exe $'noautocmd normal! {strcharlen(s_left)}"_x'
    endif
    setcharpos('.', end)
    if delete_empty_lines && getline('.') =~ $'\V\^\s\*{escape(s_right, '\')}\$'
        noautocmd normal! "_dd
        end[1] -= 1
    else
        var move_left = charcol('.') < charcol('$') - strcharlen(s_right)
        exe $'noautocmd normal! {strcharlen(s_right)}"_x'
        if move_left
            noautocmd normal! h
        endif
        end[2] = charcol('.')
    endif
    if delete_empty_lines && indent_lines >= 1
            && (s_left =~ '[([{]' || s_with == 't')
            && ShouldIndent()
        exe $":{start[0] - 1}"
        exe $":silent noautocmd normal! {end[1] - start[1] + 2}=="
    endif
    winrestview(view)
    setcharpos('.', start)

    return [start, end]
enddef

def ChangeSurround()
    if s_with == c_with
        return
    endif
    var with = s_with
    var pos = RemoveSurround(false)
    if !empty(pos)
        var [start, end] = pos
        if getline(start[1]) =~ '^\s*$'
            defer () => {
                noautocmd normal! 2_
            }()
        endif
        s_with = c_with
        AddSurround('char', start, end)
        s_with = with
    endif
enddef

def SkipEscaped(): bool
    var line = getline(line('.'))[ : col('.') - 2]
    var escaped = matchstr(line, '\\*$')
    return fmod(len(escaped), 2) > 0
enddef

def ProbePair(s_left: string, s_right: string): list<list<number>>
    var view = winsaveview()
    var unnamed = getreg("")
    defer () => {
        setreg("", unnamed)
        winrestview(view)
    }()

    if trim(s_left) != trim(s_right)
        noautocmd normal! yl
        var char = getreg("")
        var flags = 'bW'
        if stridx(s_right, char) != -1
            if search('\V' .. escape(s_right, '\'), 'cbW', line('.')) == 0
                flags ..= 'c'
            endif
        else
            flags ..= 'c'
        endif
        if searchpair('\V' .. escape(s_left, '\'), '', '\V' .. escape(s_right, '\'), flags, () => SkipEscaped()) <= 0
            return [[], []]
        endif
        var start = getcursorcharpos()
        if searchpair('\V' .. escape(s_left, '\'), '', '\V' .. escape(s_right, '\'), 'W', () => SkipEscaped()) <= 0
            return [[], []]
        endif
        var end = getcursorcharpos()
        return [start, end]
    else
        if search('\V' .. escape(s_left, '\'), 'bW', line('.'), 200, () => SkipEscaped()) <= 0
            if search('\V' .. escape(s_left, '\'), 'cbW', line('.'), 200, () => SkipEscaped()) <= 0
                return [[], []]
            endif
        endif
        var start = getcursorcharpos()
        if search('\V' .. escape(s_right, '\'), 'W', line('.'), 200, () => SkipEscaped()) <= 0
            return [[], []]
        endif
        var end = getcursorcharpos()

        if start != [0, 0] && end != [0, 0] && start != end
            return [start, end]
        else
            return [[], []]
        endif
    endif
enddef

def ProbeTag(): tuple<list<number>, list<number>, string, string>
    var view = winsaveview()
    var unnamed = getreg("")
    defer () => {
        winrestview(view)
        setreg("", unnamed)
    }()

    var s_left = ''
    var s_right = ''
    var tagregion = []
    try
        noautocmd normal! yat
        var start = getcharpos("'[")
        var end = getcharpos("']")

        var line = getline(end[1])[ : end[2] - 1]
        s_right = matchstr(line, '</\S\{-}>$')
        line = getline(start[1])[start[2] - 1 :]
        s_left = matchstr(line, '^<[^[:punct:][:space:]].\{-}>')

        if !empty(s_left) && !empty(s_right)
            end[2] -= (strcharlen(s_right) - 1)
            return (start, end, s_left, s_right)
        endif
    catch
    finally
        exe "noautocmd normal! \<esc>"
    endtry
    return ([], [], '', '')
enddef

def MoveCursor(lnum: number, col: number)
    exe $":{lnum}"
    noautocmd normal! 0
    if col > 1
        exe $"noautocmd normal! {col - 1}l"
    endif
enddef
