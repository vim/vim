" Vim filetype plugin file
" Language:	Javascript
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:  2020 Jun 21
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/ftplugin/javascript.vim
" Contributor:  Romain Lafourcade <romainlafourcade@gmail.com>

if exists("b:did_ftplugin")
    finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo-=C

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
setlocal formatoptions-=t formatoptions+=croql

" Set completion with CTRL-X CTRL-O to autoloaded function.
if exists('&ofu')
    setlocal omnifunc=javascriptcomplete#CompleteJS
endif

" Set 'comments' to format dashed lists in comments.
setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/*,mb:*,ex:*/,://

setlocal commentstring=//%s

" Change the :browse e filter to primarily show JavaScript-related files.
if has("gui_win32")
    let  b:browsefilter="Javascript Files (*.js, *.jsx, *.es, *.es6, *.cjs, *.mjs, *.jsm, *.vue, *.json)\t*.js;*.jsx;*.es;*.es6;*.cjs;*.mjs;*.jsm;*.vue;*.json\n" .
                \ "All Files (*.*)\t*.*\n"
endif

" The following suffixes should be implied when resolving filenames
setlocal suffixesadd+=.js,.jsx,.es,.es6,.cjs,.mjs,.jsm,.vue,.json

" The following suffixes should have low priority
"   .snap    jest snapshot
setlocal suffixes+=.snap

" Prepend node_modules/.bin to $PATH if applicable
" Allows calling executables installed locally via npm/yarn
let s:bin_dir = finddir('node_modules/.bin', '.;')->fnamemodify(':p')
if len(s:bin_dir) && $PATH !~ s:bin_dir
    let $PATH = s:bin_dir .. ':' .. $PATH
endif
unlet s:bin_dir

" Set 'path' to a minimal, non-greedy, value
" User is expected to augment it with contextually-relevant paths
setlocal path-=node_modules
setlocal path-=/usr/include
setlocal path-=**

" Matchit configuration
let b:match_words = '\<function\>:\<return\>,'
            \ .. '\<do\>:\<while\>,'
            \ .. '\<switch\>:\<case\>:\<default\>,'
            \ .. '\<if\>:\<else\>,'
            \ .. '\<try\>:\<catch\>:\<finally\>,'
            \ .. '<\(\w\+\):</\1>'

" Use eslint for :make if applicable
if executable('eslint')
    compiler eslint
endif

" Use standard for :make if applicable
if executable('standard')
    compiler standard
endif

" Set 'define' to a comprehensive value
let &define = '\('
            \ .. '\(^\s*(*async\s\{-}function\|(*function\)'
            \ .. '\|^\s*\(\*\|static\|async\|get\|set\|\i\{-}\.\)'
            \ .. '\|^\s*\(\ze\i\{-}\)\(([^)]*).*{$\|\s*[:=,]\)'
            \ .. '\|^\s*\(export\s\{-}\|export\s\{-}default\s\{-}\)*\(var\|let\|const\|function\)'
            \ .. '\|\(\<as\>\)'
            \ .. '\)'

let b:undo_ftplugin += "setl fo< ofu< com< cms< sua< su< def< pa< mp< efm<"

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: textwidth=78 tabstop=8 shiftwidth=4 softtabstop=4 expandtab
