vim9script

# Language:     Vim9 script
# Contributers: @lacygoill
#               Shane-XB-Qian
#               Andrew Radev
# Last Change:  2025 Oct 17
#
# Vim script to handle jumping to the targets of several types of Vim commands
# (:import, :packadd, :runtime, :colorscheme), and to autoloaded functions of
# the style <path>#<function_name>.
#
# see runtime/ftplugin/vim.vim

# Interface {{{1
export def Find(editcmd: string) #{{{2
    var curline: string = getline('.')

    if curline =~ '^\s*\%(:\s*\)\=\%(sil\%[ent]!\=\s\+\)\=packadd!\=\s'
        HandlePackaddLine(editcmd, curline)
        return
    endif

    if curline =~ '^\s*\%(:\s*\)\=\%(sil\%[ent]!\=\s\+\)\=ru\%[ntime]!\='
        HandleRuntimeLine(editcmd, curline, expand('<cfile>'))
        return
    endif

    if curline =~ '^\s*\%(:\s*\)\=\%(sil\%[ent]!\=\s\+\)\=colo\%[rscheme]\s'
        HandleColoLine(editcmd, curline)
        return
    endif

    if curline =~ '^\s*\%(:\s*\)\=import\s'
        HandleImportLine(editcmd, curline)
        return
    endif

    var curfunc = FindCurfunc()

    if stridx(curfunc, '#') >= 0
        var parts = split(curfunc, '#')
        var path = $"autoload/{join(parts[0 : -2], '/')}.vim"
        var resolved_path = globpath(&runtimepath, path)

        if resolved_path != ''
            var function_pattern: string = $'^\s*\%(:\s*\)\=fun\%[ction]!\=\s\+\zs{curfunc}('
            resolved_path->Open(editcmd, function_pattern)
        endif
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
    var pat: string = '\s*\%(:\s*\)\=packadd!\=\s\+\zs\S\+\>\ze'
    var plugin: string = curline
        ->matchstr(pat)
        ->substitute('^vim-\|\.vim$', '', 'g')

    if plugin == ''
        Fallback(editcmd)
    else
        var files: list<string> = getcompletion($'plugin/{plugin}', 'runtime')
            ->map((_, fname: string) => fname->findfile(&rtp)->fnamemodify(':p'))
            ->filter((_, path: string): bool => filereadable(path))
        if empty(files)
            echo 'Could not find any plugin file for ' .. string(plugin)
            return
        endif
        files->Open(editcmd)
    endif
enddef

def HandleRuntimeLine(editcmd: string, curline: string, cfile: string) #{{{2
    var fname: string
    var where_pat: string = '\%(START\|OPT\|PACK\|ALL\)'

    if cfile == 'runtime' || cfile =~# $'^{where_pat}$'
        # then the cursor was not on one of the filenames, jump to the first file:
        var fname_pat: string = $'\s*\%(:\s*\)\=ru\%[ntime]\%(!\s*\|\s\+\)\%({where_pat}\s\+\)\=\zs\S\+\>\ze'
        fname = curline->matchstr(fname_pat)
    else
        fname = cfile
    endif

    if fname == ''
        Fallback(editcmd)
    else
        var file: string = fname
            ->findfile(&rtp)
            ->fnamemodify(':p')
        if file == '' || !filereadable(file)
            echo 'Could not be found in the runtimepath: ' .. string(fname)
            return
        endif
        file->Open(editcmd)
    endif
enddef

def HandleColoLine(editcmd: string, curline: string) #{{{2
    var pat: string = '\s*\%(:\s*\)\=colo\%[rscheme]\s\+\zs\S\+\>\ze'
    var colo: string = curline->matchstr(pat)

    if colo == ''
        Fallback(editcmd)
    else
        var files: list<string> = getcompletion($'colors/{colo}', 'runtime')
            ->map((_, fname: string) => fname->findfile(&rtp)->fnamemodify(':p'))
            ->filter((_, path: string): bool => filereadable(path))
        if empty(files)
            echo 'Could not find any colorscheme file for ' .. string(colo)
            return
        endif
        files->Open(editcmd)
    endif
enddef

def HandleImportLine(editcmd: string, curline: string) #{{{2
    var fname: string
    var import_cmd: string = '^\s*\%(:\s*\)\=import\s\+\%(autoload\s\+\)\='
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

def Open(target: any, editcmd: string, search_pattern: string = '') #{{{2
    var split: string = editcmd[0] == 'g' ? 'edit' : editcmd[1] == 'g' ? 'tabedit' : 'split'
    var fname: string
    var cmd: string

    if target->typename() == 'list<string>'
        if target->empty()
            return
        endif
        fname = target[0]
    else
        if target->typename() != 'string'
            return
        endif
        fname = target
    endif

    if search_pattern != ''
        var escaped_pattern = escape(search_pattern, '\#'' ')
        cmd = $'+silent\ call\ search(''{escaped_pattern}'')'
    endif

    execute $'{split} {cmd} {fname}'

    # If there are several files to open, put them into an arglist.
    if target->typename() == 'list<string>'
            && target->len() > 1
        var arglist: list<string> = target
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

def Fallback(editcmd: string) #{{{2
    try
        execute 'normal! ' .. editcmd .. 'zv'
    catch
        Error(v:exception)
    endtry
enddef

def FindCurfunc(): string #{{{2
    var curfunc = ''
    var saved_iskeyword = &iskeyword

    try
        set iskeyword+=#
        curfunc = expand('<cword>')
    finally
        &iskeyword = saved_iskeyword
    endtry

    return curfunc
enddef

# vim: sw=4 et
