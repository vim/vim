" Vim filetype plugin file
" Language:	Java
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Change:  2004 May 16
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

" Go ahead and set this to get decent indenting even if the indent files
" aren't being used.  For people who really don't want any indentation,
" let them turn it off.
if !exists("g:ftplugin_java_no_indent")
    setlocal cindent

    "---------------------
    " Correctly indent anonymous classes
    " From Johannes Zellner <johannes@zellner.org>
    setlocal cinoptions+=j1
    "---------------------
endif

" For filename completion, prefer the .java extension over the .class
" extension.
set suffixes+=.class

" Enable gf on import statements.  Convert . in the package
" name to / and append .java to the name, then search the path.
setlocal includeexpr=substitute(v:fname,'\\.','/','g')
setlocal suffixesadd=.java
if exists("g:ftplugin_java_source_path")
    let &l:path=g:ftplugin_java_source_path . ',' . &l:path
endif

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
setlocal formatoptions-=t formatoptions+=croql

" Set 'comments' to format dashed lists in comments. Behaves just like C.
setlocal comments& comments^=sO:*\ -,mO:*\ \ ,exO:*/

setlocal commentstring=//%s

" Change the :browse e filter to primarily show Java-related files.
if has("gui_win32")
    let  b:browsefilter="Java Files (*.java)\t*.java\n" .
		\	"Properties Files (*.prop*)\t*.prop*\n" .
		\	"Manifest Files (*.mf)\t*.mf\n" .
		\	"All Files (*.*)\t*.*\n"
endif

" Undo the stuff we changed.
let b:undo_ftplugin = "setlocal cindent< cinoptions< suffixes< suffixesadd<" .
		\     " formatoptions< comments< commentstring< path< includeexpr<" .
		\     " | unlet! b:browsefilter"

" Restore the saved compatibility options.
let &cpo = s:save_cpo
