" Vim filetype plugin file
" Language:     Abaqus finite element input file (www.abaqus.com)
" Maintainer:   Carl Osterwisch <osterwischc@asme.org>
" Last Change:  2004 Jul 06

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin") | finish | endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Save the compatibility options and temporarily switch to vim defaults
let s:cpo_save = &cpoptions
set cpoptions&vim

" Folding
if version >= 600
    " Fold all lines that do not begin with *
    setlocal foldexpr=getline(v:lnum)[0]!=\"\*\"
    setlocal foldmethod=expr
endif

" Set the format of the include file specification for Abaqus
" Used in :check gf ^wf [i and other commands
setlocal include=\\<\\cINPUT\\s*=

" Remove characters up to the first = when evaluating filenames
setlocal includeexpr=substitute(v:fname,'.\\{-}=','','')

" Remove comma from valid filename characters since it is used to
" separate keyword parameters
setlocal isfname-=,

" Define format of comment lines (see 'formatoptions' for uses)
setlocal comments=:**
setlocal commentstring=**%s

" Definitions start with a * and assign a NAME, NSET, or ELSET
" Used in [d ^wd and other commands
setlocal define=^\\*\\a.*\\c\\(NAME\\\|NSET\\\|ELSET\\)\\s*=

" Abaqus keywords and identifiers may include a - character
setlocal iskeyword+=-

" Set the file browse filter (currently only supported under Win32 gui)
if has("gui_win32") && !exists("b:browsefilter")
    let b:browsefilter = "Abaqus Input Files (*.inp *.inc)\t*.inp;*.inc\n" .
    \ "Abaqus Results (*.dat)\t*.dat\n" .
    \ "Abaqus Messages (*.pre *.msg *.sta)\t*.pre;*.msg;*.sta\n" .
    \ "All Files (*.*)\t*.*\n"
endif

" Define keys used to move [count] sections backward or forward.
" TODO: Make this do something intelligent in visual mode.
nnoremap <silent> <buffer> [[ :call <SID>Abaqus_Jump('?^\*\a?')<CR>
nnoremap <silent> <buffer> ]] :call <SID>Abaqus_Jump('/^\*\a/')<CR>
function! <SID>Abaqus_Jump(motion) range
    let s:count = v:count1
    mark '
    while s:count > 0
        silent! execute a:motion
        let s:count = s:count - 1
    endwhile
endfunction

" Define key to toggle commenting of the current line or range
noremap <silent> <buffer> <m-c> :call <SID>Abaqus_ToggleComment()<CR>j
function! <SID>Abaqus_ToggleComment() range
    if strpart(getline(a:firstline), 0, 2) == "**"
        " Un-comment all lines in range
        silent execute a:firstline . ',' . a:lastline . 's/^\*\*//'
    else
        " Comment all lines in range
        silent execute a:firstline . ',' . a:lastline . 's/^/**/'
    endif
endfunction

" Restore saved compatibility options
let &cpoptions = s:cpo_save
