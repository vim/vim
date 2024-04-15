function! astro#IdentifyScope(start, end) abort
    let pos_start = searchpairpos(a:start, '', a:end, 'bnW')
    let pos_end = searchpairpos(a:start, '', a:end, 'nW')

    return pos_start != [0, 0]
                \ && pos_end != [0, 0]
                \ && pos_start[0] != getpos('.')[1]
endfunction

function! astro#AstroComments() abort
    if astro#IdentifyScope('^---\n\s*\S', '^---\n\n')
                \ || astro#IdentifyScope('^\s*<script', '^\s*<\/script>')
        " ECMAScript comments
        setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://
        setlocal commentstring=//%s

    elseif astro#IdentifyScope('^\s*<style', '^\s*<\/style>')
        " CSS comments
        setlocal comments=s1:/*,mb:*,ex:*/
        setlocal commentstring=/*%s*/

    else
        " HTML comments
        setlocal comments=s:<!--,m:\ \ \ \ ,e:-->
        setlocal commentstring=<!--%s-->
    endif
endfunction

" https://code.visualstudio.com/docs/languages/jsconfig
function! astro#CollectPathsFromConfig() abort
    let config_json = findfile('tsconfig.json', '.;')

    if empty(config_json)
        let config_json = findfile('jsconfig.json', '.;')

        if empty(config_json)
            return
        endif
    endif

    let paths_from_config = config_json
                \ ->readfile()
                \ ->filter({ _, val -> val =~ '^\s*[\[\]{}"0-9]' })
                \ ->join()
                \ ->json_decode()
                \ ->get('compilerOptions', {})
                \ ->get('paths', {})

    if !empty(paths_from_config)
        let b:astro_paths = paths_from_config
                    \ ->map({key, val -> [
                    \     key->glob2regpat(),
                    \     val[0]->substitute('\/\*$', '', '')
                    \   ]})
                    \ ->values()
    endif

    let b:undo_ftplugin ..= " | unlet! b:astro_paths"
endfunction

function! astro#AstroInclude(filename) abort
    let decorated_filename = a:filename
                \ ->substitute("^", "@", "")

    let found_path = b:
                \ ->get("astro_paths", [])
                \ ->indexof({ key, val -> decorated_filename =~ val[0]})

    if found_path != -1
        let alias = b:astro_paths[found_path][0]
        let path  = b:astro_paths[found_path][1]
                    \ ->substitute('\(\/\)*$', '/', '')

        return decorated_filename
                    \ ->substitute(alias, path, '')
    endif

    return a:filename
endfunction
