vim9script

# Interface {{{1
export def Find(editcmd: string) #{{{2
    var curline: string = getline('.')

    if curline =~ '^\s*\%(:\s*\)\=packadd!\=\s'
        HandlePackaddLine(editcmd, curline)
        return
    endif

    if curline =~ '^\s*\%(:\s*\)\=import\s'
        HandleImportLine(editcmd, curline)
        return
    endif

    try
        execute 'normal! ' .. editcmd
    catch
        Error(v:exception)
    endtry
enddef
#}}}1
# Core {{{1
def HandlePackaddLine(editcmd: string, curline: string) #{{{2
    var pat: string = '^\s*packadd!\=\s\+\zs\S\+$'
    var plugin: string = curline
        ->matchstr(pat)
        ->substitute('^vim-\|\.vim$', '', 'g')

    if plugin == ''
        try
            execute 'normal! ' .. editcmd .. 'zv'
        catch
            Error(v:exception)
            return
        endtry
    else
        var split: string = editcmd[0] == 'g' ? 'edit' : editcmd[1] == 'g' ? 'tabedit' : 'split'
        # In the  past, we passed  `runtime` to `getcompletion()`,  instead of
        # `cmdline`.  But the  output was tricky to use,  because it contained
        # paths relative to inconsistent root directories.
        var files: list<string> = getcompletion($'edit **/plugin/{plugin}.vim', 'cmdline')
            ->filter((_, path: string): bool => filereadable(path))
            ->map((_, fname: string) => fname->fnamemodify(':p'))
        if empty(files)
            echo 'Could not find any plugin file for ' .. string(plugin)
            return
        endif
        files->Open(split)
    endif
enddef

def HandleImportLine(editcmd: string, curline: string) #{{{2
    var fname: string
    var import_cmd: string = '^\s*import\s\+\%(autoload\s\+\)\='
    var import_alias: string = '\%(\s\+as\s\+\w\+\)\=$'
    var import_string: string = import_cmd .. '\([''"]\)\zs.*\ze\1' .. import_alias
    var import_expr: string = import_cmd .. '\zs.*\ze' .. import_alias
    # the script is referred to by its name in a quoted string
    if curline =~ import_string
        fname = curline->matchstr(import_string)
    # the script is referred to by an expression
    elseif curline =~ import_expr
        try
            sandbox fname = curline
                ->matchstr(import_expr)
                ->eval()
        catch
            Error(v:exception)
            return
        endtry
    endif

    var filepath: string
    if fname->isabsolutepath()
        filepath = fname
    elseif fname[0] == '.'
        filepath = (expand('%:h') .. '/' .. fname)->simplify()
    else
        var subdir: string = curline =~ '^\s*import\s\+autoload\>' ? 'autoload' : 'import'
        # Matching patterns in `'wildignore'` can be slow.
        # Let's set `{nosuf}` to `true` to avoid `globpath()` to be slow.
        filepath = globpath(&runtimepath, subdir .. '/' .. fname, true, true)
            ->get(0, '')
    endif

    if !filepath->filereadable()
        printf('E447: Can''t find file "%s" in path', fname)
            ->Error()
        return
    endif

    var how_to_split: string = {
        gF: 'edit',
        "\<C-W>F": 'split',
        "\<C-W>gF": 'tab split',
    }[editcmd]
    execute how_to_split .. ' ' .. filepath
enddef

def Open(what: any, how: string) #{{{2
    var fname: string
    if what->typename() == 'list<string>'
        if what->empty()
            return
        endif
        fname = what[0]
    else
        if what->typename() != 'string'
            return
        endif
        fname = what
    endif

    execute $'{how} {fname}'
    cursor(1, 1)

    # If there are several files to open, put them into an arglist.
    if what->typename() == 'list<string>'
            && what->len() > 1
        var arglist: list<string> = what
            ->copy()
            ->map((_, f: string) => f->fnameescape())
        execute $'arglocal {arglist->join()}'
    endif
enddef
#}}}1
# Util {{{1
def Error(msg: string) #{{{2
    echohl ErrorMsg
    echomsg msg
    echohl NONE
enddef
