" SQL filetype plugin file
" Language:    SQL (Common for Oracle, Microsoft SQL Server, Sybase)
" Version:     0.08
" Maintainer:  David Fishburn <fishburn at ianywhere dot com>
" Last Change: Mon Feb 21 2005 7:27:36 AM
" Download:    http://vim.sourceforge.net/script.php?script_id=454

" This file should only contain values that are common to all SQL languages
" Oracle, Microsoft SQL Server, Sybase ASA/ASE, MySQL, and so on
" If additional features are required create:
"        vimfiles/after/ftplugin/sql.vim (Windows)
"        .vim/after/ftplugin/sql.vim     (Unix)
" to override and add any of your own settings.

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

let s:save_cpo = &cpo
set cpo=

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Some standard expressions for use with the matchit strings
let s:notend = '\%(\<end\s\+\)\@<!'
let s:when_no_matched_or_others = '\%(\<when\>\%(\s\+\%(\%(\<not\>\s\+\)\?<matched\>\)\|\<others\>\)\@!\)'
let s:or_replace = '\%(or\s\+replace\s\+\)\?'

" Define patterns for the matchit macro
if !exists("b:match_words")
    " SQL is generally case insensitive
    let b:match_ignorecase = 1

    " Handle the following:
    " if
    " elseif | elsif
    " else [if]
    " end if
    "
    " [while condition] loop
    "     leave
    "     break
    "     continue
    "     exit
    " end loop
    "
    " for
    "     leave
    "     break
    "     continue
    "     exit
    " end loop
    "
    " do
    "     statements
    " doend
    "
    " case
    " when 
    " when
    " default
    " end case
    "
    " merge
    " when not matched
    " when matched
    "
    " EXCEPTION
    " WHEN column_not_found THEN
    " WHEN OTHERS THEN
    "
    " create[ or replace] procedure|function|event

    let b:match_words =
		\ '\<begin\>:\<end\>\W*$,'.
		\
                \ s:notend . '\<if\>:'.
                \ '\<elsif\>\|\<elseif\>\|\<else\>:'.
                \ '\<end\s\+if\>,'.
                \
                \ '\<do\>\|'.
                \ '\<while\>\|'.
                \ '\%(' . s:notend . '\<loop\>\)\|'.
                \ '\%(' . s:notend . '\<for\>\):'.
                \ '\<exit\>\|\<leave\>\|\<break\>\|\<continue\>:'.
                \ '\%(\<end\s\+\%(for\|loop\>\)\)\|\<doend\>,'.
                \
                \ '\%('. s:notend . '\<case\>\):'.
                \ '\%('.s:when_no_matched_or_others.'\):'.
                \ '\%(\<when\s\+others\>\|\<end\s\+case\>\),' .
                \
                \ '\<merge\>:' .
                \ '\<when\s\+not\s\+matched\>:' .
                \ '\<when\s\+matched\>,' .
                \
                \ '\%(\<create\s\+' . s:or_replace . '\)\?'.
                \ '\%(function\|procedure\|event\):'.
                \ '\<returns\?\>'
                " \ '\<begin\>\|\<returns\?\>:'.
                " \ '\<end\>\(;\)\?\s*$'
                " \ '\<exception\>:'.s:when_no_matched_or_others.
                " \ ':\<when\s\+others\>,'.
		"
                " \ '\%(\<exception\>\|\%('. s:notend . '\<case\>\)\):'.
                " \ '\%(\<default\>\|'.s:when_no_matched_or_others.'\):'.
                " \ '\%(\%(\<when\s\+others\>\)\|\<end\s\+case\>\),' .
endif

" Define how to find the macro definition of a variable using the various
" [d, [D, [_CTRL_D and so on features
" Match these values ignoring case
" ie  DECLARE varname INTEGER
let &l:define = '\c\(DECLARE\|IN\|OUT\|INOUT\)\s*'


" Mappings to move to the next BEGIN ... END block
" \W - no characters or digits
nmap <buffer> <silent> ]] :call search('\c^\s*begin\>', 'W' )<CR>
nmap <buffer> <silent> [[ :call search('\c^\s*begin\>', 'bW' )<CR>
nmap <buffer> <silent> ][ :call search('\c^\s*end\W*$', 'W' )<CR>
nmap <buffer> <silent> [] :call search('\c^\s*end\W*$', 'bW' )<CR>
vmap <buffer> <silent> ]] /\c^\s*begin\><CR>
vmap <buffer> <silent> [[ ?\c^\s*begin<CR>
vmap <buffer> <silent> ][ /\c^\s*end\W*$<CR>
vmap <buffer> <silent> [] ?\c^\s*end\W*$<CR>


" Predefined SQL objects what are used by the below mappings using
" the ]} style maps.
" This global variable allows the users to override it's value
" from within their vimrc.
if !exists('g:ftplugin_sql_objects')
    let g:ftplugin_sql_objects = 'function,procedure,event,' .
                \ '\(existing\\|global\s\+temporary\s\+\)\?table,trigger' .
                \ ',schema,service,publication,database,datatype,domain' .
                \ ',index,subscription,synchronization,view,variable'
endif

let s:ftplugin_sql_objects = 
            \ '\c^\s*' .
            \ '\(create\s\+\(or\s\+replace\s\+\)\?\)\?' .
            \ '\<\(' .
            \ substitute(g:ftplugin_sql_objects, ',', '\\\\|', 'g') .
            \ '\)\>' 

" Mappings to move to the next CREATE ... block
" map <buffer> <silent> ]} :call search(g:ftplugin_sql_objects, 'W' )<CR>
" nmap <buffer> <silent> [{ :call search('\c^\s*\(create\s\+\(or\s\+replace\s\+\)\?\)\?\<\(function\\|procedure\\|event\\|table\\|trigger\\|schema\)\>', 'bW' )<CR>
" exec 'nmap <buffer> <silent> ]} /'.s:ftplugin_sql_objects.'<CR>'
exec "nmap <buffer> <silent> ]} :call search('".s:ftplugin_sql_objects."', 'W')<CR>"
exec "nmap <buffer> <silent> [{ :call search('".s:ftplugin_sql_objects."', 'bW')<CR>"
" Could not figure out how to use a :call search() string in visual mode
" without it ending visual mode
exec 'vmap <buffer> <silent> ]} /'.s:ftplugin_sql_objects.'<CR>'
exec 'vmap <buffer> <silent> [{ ?'.s:ftplugin_sql_objects.'<CR>'
" vmap <buffer> <silent> ]} /\c^\s*\(create\s\+\(or\s\+replace\s\+\)\?\)\?\<\(function\\|procedure\\|event\\|table\\|trigger\\|schema\)\><CR>
" vmap <buffer> <silent> [{ ?\c^\s*\(create\s\+\(or\s\+replace\s\+\)\?\)\?\<\(function\\|procedure\\|event\\|table\\|trigger\\|schema\)\><CR>

" Mappings to move to the next COMMENT
"
" Had to double the \ for the \| separator since this has a special
" meaning on maps
let b:comment_leader = '\(--\\|\/\/\\|\*\\|\/\*\\|\*\/\)'
" Find the start of the next comment
let b:comment_start  = '^\(\s*'.b:comment_leader.'.*\n\)\@<!'.
            \ '\(\s*'.b:comment_leader.'\)'
" Find the end of the previous comment
let b:comment_end = '\(^\s*'.b:comment_leader.'.*\n\)'.
            \ '\(^\s*'.b:comment_leader.'\)\@!'
" Skip over the comment
let b:comment_jump_over  = "call search('".
            \ '^\(\s*'.b:comment_leader.'.*\n\)\@<!'.
            \ "', 'W')"
let b:comment_skip_back  = "call search('".
            \ '^\(\s*'.b:comment_leader.'.*\n\)\@<!'.
            \ "', 'bW')"
" Move to the start and end of comments
exec 'nnoremap <silent><buffer> ]" /'.b:comment_start.'<CR>'
exec 'nnoremap <silent><buffer> [" /'.b:comment_end.'<CR>'
exec 'vnoremap <silent><buffer> ]" /'.b:comment_start.'<CR>'
exec 'vnoremap <silent><buffer> [" /'.b:comment_end.'<CR>'

" Comments can be of the form:
"   /*
"    *
"    */
" or
"   // 
" or
"   --
setlocal comments=s1:/*,mb:*,ex:*/,:--,://

let &cpo = s:save_cpo

" vim:sw=4:ff=unix:

