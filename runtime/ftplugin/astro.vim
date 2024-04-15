" Vim filetype plugin file
" Language:     Astro
" Maintainer:   Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change:  2022 Dec 5

if exists("b:did_ftplugin")
    finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo-=C

let b:undo_ftplugin = "setlocal"
            \ .. " formatoptions<"
            \ .. " path<"
            \ .. " suffixesadd<"
            \ .. " matchpairs<"
            \ .. " comments<"
            \ .. " commentstring<"
            \ .. " iskeyword<"
            \ .. " define<"
            \ .. " include<"
            \ .. " includeexpr<"

" Create self-resetting autocommand group
augroup Astro
    autocmd! * <buffer>
augroup END

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
setlocal formatoptions-=t
setlocal formatoptions+=croql

" Remove irrelevant part of 'path'.
setlocal path-=/usr/include

" Seed 'path' with default directories for :find, gf, etc.
setlocal path+=src/**
setlocal path+=public/**

" Help Vim find extension-less filenames
let &l:suffixesadd =
            \ ".astro"
            \ .. ",.js,.jsx,.es,.es6,.cjs,.mjs,.jsm"
            \ .. ",.json"
            \ .. ",.scss,.sass,.css"
            \ .. ",.svelte"
            \ .. ",.ts,.tsx,.d.ts"
            \ .. ",.vue"

" From $VIMRUNTIME/ftplugin/html.vim
setlocal matchpairs+=<:>

" Matchit configuration
if exists("loaded_matchit")
    let b:match_ignorecase = 0

    " From $VIMRUNTIME/ftplugin/javascript.vim
    let b:match_words =
                \ '\<do\>:\<while\>,'
                \ .. '<\@<=\([^ \t>/]\+\)\%(\s\+[^>]*\%([^/]>\|$\)\|>\|$\):<\@<=/\1>,'
                \ .. '<\@<=\%([^ \t>/]\+\)\%(\s\+[^/>]*\|$\):/>'

    " From $VIMRUNTIME/ftplugin/html.vim
    let b:match_words ..=
                \ '<!--:-->,'
                \ .. '<:>,'
                \ .. '<\@<=[ou]l\>[^>]*\%(>\|$\):<\@<=li\>:<\@<=/[ou]l>,'
                \ .. '<\@<=dl\>[^>]*\%(>\|$\):<\@<=d[td]\>:<\@<=/dl>,'
                \ .. '<\@<=\([^/!][^ \t>]*\)[^>]*\%(>\|$\):<\@<=/\1>'

    let b:undo_ftplugin ..= " | unlet! b:match_ignorecase b:match_words"
endif

" Change what constitutes a word, mainly useful for CSS/SASS
setlocal iskeyword+=-
setlocal iskeyword+=$
setlocal iskeyword+=%

" Define paths/aliases for module resolution
call astro#CollectPathsFromConfig()

" Find ESM imports
setlocal include=^\\s*\\(import\\\|import\\s\\+[^\/]\\+from\\)\\s\\+['\"]

" Process aliases if file can't be found
setlocal includeexpr=astro#AstroInclude(v:fname)

" Set 'define' to a comprehensive value
" From $VIMRUNTIME/ftplugin/javascript.vim and
" $VIMRUNTIME/ftplugin/sass.vim
let &l:define =
            \ '\(^\s*(*async\s\+function\|(*function\)'
            \ .. '\|^\s*\(\*\|static\|async\|get\|set\|\i\+\.\)'
            \ .. '\|^\s*\(\ze\i\+\)\(([^)]*).*{$\|\s*[:=,]\)'


" Set &comments and &commentstring according to current scope
autocmd Astro CursorMoved <buffer> call astro#AstroComments()

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: textwidth=78 tabstop=8 shiftwidth=4 softtabstop=4 expandtab b:undo_ftplugin
