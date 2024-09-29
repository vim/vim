vim9script

# Maintainer: Maxim Kim <habamax@gmail.com>
# Last Update: 2024-09-24
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
    var start_col = getpos("'[")[2]
    if len(cms_l) == 1 && lnum1 == lnum2 && first_col < start_col
        var line_start = getline(lnum1)[0 : max(0, start_col - 2)]
        var line_end = getline(lnum1)[start_col - 1 : -1]
        line_end = line_end =~# $'^\s*{cms_l[0]}' ?
                    \ substitute(line_end, $'^\s*\zs{cms_l[0]}', '', '') :
                    \ printf(substitute(cms, '%s\@!', '%%', 'g'), line_end)
        setline(lnum1, line_start .. line_end)
        return ''
    endif

    if len(cms_l) == 0 | return '' | endif
    if len(cms_l) == 1 | call add(cms_l, '') | endif
    var comment = 0
    var indent_min = indent(lnum1)
    var indent_start = matchstr(getline(lnum1), '^\s*')
    for lnum in range(lnum1, lnum2)
        if getline(lnum) =~ '^\s*$' | continue | endif
        if indent_min > indent(lnum)
            indent_min = indent(lnum)
            indent_start = matchstr(getline(lnum), '^\s*')
        endif
        if getline(lnum) !~ $'^\s*{cms_l[0]}.*{cms_l[1]}$'
            comment = 1
        endif
    endfor
    var lines = []
    var line = ''
    for lnum in range(lnum1, lnum2)
        if getline(lnum) =~ '^\s*$'
            line = getline(lnum)
        elseif comment
            if exists("g:comment_first_col") || exists("b:comment_first_col")
                # handle % with substitute
                line = printf(substitute(cms, '%s\@!', '%%', 'g'), getline(lnum))
            else
                # handle % with substitute
                line = printf(indent_start .. substitute(cms, '%s\@!', '%%', 'g'),
                        strpart(getline(lnum), strlen(indent_start)))
            endif
        else
            line = substitute(getline(lnum), $'^\s*\zs{cms_l[0]} \?\| \?{cms_l[1]}$', '', 'g')
        endif
        add(lines, line)
    endfor
    noautocmd keepjumps setline(lnum1, lines)
    return ''
enddef
