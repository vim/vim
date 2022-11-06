vim9script


# To be able to reformat with `gq` add following to `.vim/ftplugin/json.vim`:
#    import autoload 'json.vim'
#    setl formatexpr=json.FormatExpr()
#    xnoremap <buffer> gq <scriptcmd>json.Format(line('v'), line('.'))<CR>
export def FormatExpr(): number
    Format(v:lnum, v:lnum + v:count - 1)
    return 0
enddef


# import autoload 'json.vim'
# command -range=% JSONFormat json.Format(<line1>, <line2>)
export def Format(line1: number, line2: number)
    var indent_base = matchstr(getline(line1), '^\s*')
    var indent = &expandtab ? repeat(' ', &shiftwidth) : "\t"

    var json_src = getline(line1, line2)->join()
    var json_fmt = FormatStr(json_src, {use_tabs: !&et, indent: &sw, indent_base: indent_base})->split("\n")

    exe $":{line1},{line2}d"
    if line('$') == 1
        setline(line1, json_fmt[0])
        append(line1, json_fmt[1 : ])
    else
        append(line1 - 1, json_fmt)
    endif
enddef


# import autoload 'json.vim'
# echo json.FormatStr('{"hello": "world"}', {use_tabs: false, indent: 2, indent_base: 0})
# {
#   "hello": "world"
# }
export def FormatStr(json_str: string, params: dict<any>): string
    var json = []
    var indent_lvl = 0
    var indent_base = get(params, "indent_base", "")
    var indent = get(params, "use_tabs", false) ? "\t" : repeat(' ', get(params, "indent", 2))
    var json_line = indent_base
    var state = ""
    for char in json_str
        if state == ""
            if char =~ '[{\[]'
                json_line ..= char
                json->add(json_line)
                indent_lvl += 1
                json_line = indent_base .. repeat(indent, indent_lvl)
            elseif char =~ '[}\]]'
                if json_line !~ '^\s*$'
                    json->add(json_line)
                    indent_lvl -= 1
                    if indent_lvl < 0
                        json_line = strpart(indent_base, -indent_lvl * len(indent))
                    else
                        json_line = indent_base .. repeat(indent, indent_lvl)
                    endif
                endif
                json_line ..= char
            elseif char == ':'
                json_line ..= char .. ' '
            elseif char == '"'
                json_line ..= char
                state = 'ATTR'
            elseif char == ','
                json_line ..= char
                json->add(json_line)
                json_line = indent_base .. repeat(indent, indent_lvl)
            elseif char !~ '\s'
                json_line ..= char
            endif
        elseif state == "ATTR"
            json_line ..= char
            if char == '\'
                state = "ESCAPE"
            elseif char == '"'
                state = ""
            endif
        elseif state == "ESCAPE"
            state = "ATTR"
            json_line ..= char
        else
            json_line ..= char
        endif
    endfor
    if json_line !~ '^\s*$'
        json->add(json_line)
    endif
    return json->join("\n")
enddef
