vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2025-03-30
#
# Toggle comments
# Usage:
#   Add following mappings to vimrc:
#       import autoload 'dist/comment.vim'
#       nnoremap <silent> <expr> gc comment.Toggle()
#       xnoremap <silent> <expr> gc comment.Toggle()
#       nnoremap <silent> <expr> gcc comment.Toggle() .. '_'
#       nnoremap <silent> <expr> gC  comment.Toggle() .. '$'
export def Toggle(...args: list<string>): string
    if len(args) == 0
        &opfunc = matchstr(expand('<stack>'), '[^. ]*\ze[')
        return 'g@'
    endif
    if empty(&cms) || !&ma | return '' | endif
    var cms = substitute(substitute(&cms, '\S\zs%s\s*', ' %s', ''), '%s\ze\S', '%s ', '')
    var [lnum1, lnum2] = [line("'["), line("']")]
    var cms_l = split(escape(cms, '*.'), '\s*%s\s*')

    var first_col = indent(lnum1)
    var start_col = getpos("'[")[2] - 1
    if len(cms_l) == 1 && lnum1 == lnum2 && first_col < start_col
        var line_start = getline(lnum1)[0 : start_col - 1]
        var line_end = getline(lnum1)[start_col : -1]
        line_end = line_end =~ $'\c^\s*{cms_l[0]}' ?
                    \ substitute(line_end, $'\c^\s*\zs{cms_l[0]}\s\ze\s*', line_end =~ '^\s' ? ' ' : '', '') :
                    \ printf(substitute(cms, '%s\@!', '%%', ''), line_end)
        setline(lnum1, line_start .. line_end)
        return ''
    endif

    if len(cms_l) == 0 | return '' | endif
    if len(cms_l) == 1 | call add(cms_l, '') | endif
    var comment = false
    var indent_spaces = false
    var indent_tabs = false
    var indent_min = indent(lnum1)
    var indent_start = matchstr(getline(lnum1), '^\s*')
    for lnum in range(lnum1, lnum2)
        if getline(lnum) =~ '^\s*$' | continue | endif
        var indent_str = matchstr(getline(lnum), '^\s*')
        if indent_min > indent(lnum)
            indent_min = indent(lnum)
            indent_start = indent_str
        endif
        indent_spaces = indent_spaces || (stridx(indent_str, ' ') != -1)
        indent_tabs = indent_tabs || (stridx(indent_str, "\t") != -1)
        if getline(lnum) !~ $'\c^\s*{cms_l[0]}.*{cms_l[1]}$'
            comment = true
        endif
    endfor
    var mixed_indent = indent_spaces && indent_tabs
    var lines = []
    var line = ''
    for lnum in range(lnum1, lnum2)
        if getline(lnum) =~ '^\s*$'
            line = getline(lnum)
        elseif comment
            if exists("g:comment_first_col") || exists("b:comment_first_col")
                line = printf(substitute(cms, '%s\@!', '%%', 'g'), getline(lnum))
            else
                # consider different whitespace indenting
                var indent_current = mixed_indent ? matchstr(getline(lnum), '^\s*') : indent_start
                line = printf(indent_current .. substitute(cms, '%s\@!', '%%', 'g'),
                    strpart(getline(lnum), strlen(indent_current)))
            endif
        else
            line = substitute(getline(lnum), $'\c^\s*\zs{cms_l[0]} \?\| \?{cms_l[1]}$', '', 'g')
        endif
        add(lines, line)
    endfor
    noautocmd keepjumps setline(lnum1, lines)
    return ''
enddef


# Comment text object
# Usage:
#     import autoload 'dist/comment.vim'
#     onoremap <silent>ic <scriptcmd>comment.ObjComment(v:true)<CR>
#     onoremap <silent>ac <scriptcmd>comment.ObjComment(v:false)<CR>
#     xnoremap <silent>ic <esc><scriptcmd>comment.ObjComment(v:true)<CR>
#     xnoremap <silent>ac <esc><scriptcmd>comment.ObjComment(v:false)<CR>
export def ObjComment(inner: bool)
    def IsComment(): bool
        var stx = map(synstack(line('.'), col('.')), 'synIDattr(v:val, "name")')->join()
        return stx =~? 'Comment'
    enddef

    # requires syntax support
    if !exists("g:syntax_on")
      return
    endif

    var pos_init = getcurpos()

    # If not in comment, search next one,
    if !IsComment()
        if search('\v\k+', 'W', line(".") + 100, 100, () => !IsComment()) <= 0
            return
        endif
    endif

    # Search for the beginning of the comment block
    if IsComment()
        if search('\v%(\S+)|%(^\s*$)', 'bW', 0, 200, IsComment) > 0
            search('\v%(\S)|%(^\s*$)', 'W', 0, 200, () => !IsComment())
        else
            cursor(1, 1)
            search('\v\S+', 'cW', 0, 200)
        endif
    endif

    var pos_start = getcurpos()

    if !inner
        var col = pos_start[2]
        var prefix = getline(pos_start[1])[ : col - 2]
        while col > 0 && prefix[col - 2] =~ '\s'
            col -= 1
        endwhile
        pos_start[2] = col
    endif

    # Search for the comment end.
    if pos_init[1] > pos_start[1]
        cursor(pos_init[1], pos_init[2])
    endif
    if search('\v%(\S+)|%(^\s*$)', 'W', 0, 200, IsComment) > 0
        search('\S', 'beW', 0, 200, () => !IsComment())
    else
        if search('\%$', 'W', 0, 200) > 0
            search('\ze\S', 'beW', line('.'), 200, () => !IsComment())
        endif
    endif

    var pos_end = getcurpos()

    if !inner
        var spaces = matchstr(getline(pos_end[1]), '\%>.c\s*')
        pos_end[2] += spaces->len()
        if getline(pos_end[1])[pos_end[2] : ] =~ '^\s*$'
            && (pos_start[2] <= 1 || getline(pos_start[1])[ : pos_start[2]] =~ '^\s*$')
            if search('\v\s*\_$(\s*\n)+', 'eW', 0, 200) > 0
                pos_end = getcurpos()
            endif
        endif
    endif

    if (pos_end[2] == (getline(pos_end[1])->len() ?? 1)) && pos_start[2] <= 1
        cursor(pos_end[1], 1)
        normal! V
        cursor(pos_start[1], 1)
    else
        cursor(pos_end[1], pos_end[2])
        normal! v
        if &selection == 'exclusive'
            normal! lo
        endif
        cursor(pos_start[1], pos_start[2])
    endif
enddef
