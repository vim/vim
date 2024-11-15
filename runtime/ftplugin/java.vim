" Vim filetype plugin file
" Language:		Java
" Maintainer:		Aliaksei Budavei <0x000c70 AT gmail DOT com>
" Former Maintainer:	Dan Sharp
" Repository:		https://github.com/zzzyxwvut/java-vim.git
" Last Change:		2024 Nov 15
"			2024 Jan 14 by Vim Project (browsefilter)
"			2024 May 23 by Riley Bruins <ribru17@gmail.com> ('commentstring')

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

if (exists("g:java_ignore_javadoc") || exists("g:java_ignore_markdown")) &&
	\ exists("*javaformat#RemoveCommonMarkdownWhitespace")
    delfunction javaformat#RemoveCommonMarkdownWhitespace
    unlet! g:loaded_javaformat
endif

if exists("b:did_ftplugin")
    let &cpo = s:save_cpo
    unlet s:save_cpo
    finish
endif

let b:did_ftplugin = 1

" For filename completion, prefer the .java extension over the .class
" extension.
set suffixes+=.class

" Enable gf on import statements.  Convert . in the package
" name to / and append .java to the name, then search the path.
setlocal includeexpr=substitute(v:fname,'\\.','/','g')
setlocal suffixesadd=.java

" Clean up in case this file is sourced again.
unlet! s:zip_func_upgradable

"""" STRIVE TO REMAIN COMPATIBLE FOR AT LEAST VIM 7.0.

" Documented in ":help ft-java-plugin".
if exists("g:ftplugin_java_source_path") &&
		\ type(g:ftplugin_java_source_path) == type("")
    if filereadable(g:ftplugin_java_source_path)
	if exists("#zip") &&
		    \ g:ftplugin_java_source_path =~# '.\.\%(jar\|zip\)$'
	    if !exists("s:zip_files")
		let s:zip_files = {}
	    endif

	    let s:zip_files[bufnr('%')] = g:ftplugin_java_source_path
	    let s:zip_files[0] = g:ftplugin_java_source_path
	    let s:zip_func_upgradable = 1

	    function! JavaFileTypeZipFile() abort
		let @/ = substitute(v:fname, '\.', '\\/', 'g') . '.java'
		return get(s:zip_files, bufnr('%'), s:zip_files[0])
	    endfunction

	    " E120 for "inex=s:JavaFileTypeZipFile()" before v8.2.3900.
	    setlocal includeexpr=JavaFileTypeZipFile()
	    setlocal suffixesadd<
	endif
    else
	let &l:path = g:ftplugin_java_source_path . ',' . &l:path
    endif
endif

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
setlocal formatoptions-=t formatoptions+=croql

" Set 'comments' to format Markdown Javadoc comments and dashed lists
" in other multi-line comments (it behaves just like C).
setlocal comments& comments^=:///,sO:*\ -,mO:*\ \ ,exO:*/

setlocal commentstring=//\ %s

" Change the :browse e filter to primarily show Java-related files.
if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
    let  b:browsefilter="Java Files (*.java)\t*.java\n" .
		\	"Properties Files (*.prop*)\t*.prop*\n" .
		\	"Manifest Files (*.mf)\t*.mf\n"
    if has("win32")
	let b:browsefilter .= "All Files (*.*)\t*\n"
    else
	let b:browsefilter .= "All Files (*)\t*\n"
    endif
endif

if exists('g:spotbugs_properties')
    let s:request = 0

    if has_key(g:spotbugs_properties, 'PreCompilerAction')
	let s:request = or(s:request, 1)
    endif

    if has_key(g:spotbugs_properties, 'PostCompilerAction')
	let s:request = or(s:request, 2)
    endif

    if s:request
	let s:actions = [{
		\ 'group': 'java_spotbugs',
		\ 'pattern': '<buffer>',
		\ 'event': 'Syntax',
		\ 'once': v:true,
		\ }, {
		\ 'group': 'java_spotbugs',
		\ 'pattern': '<buffer>',
		\ 'event': 'BufWritePost',
		\ }]

	for s:idx in range(len(s:actions))
	    if s:request == 3
		let s:actions[s:idx].cmd =
			\ 'call g:spotbugs_properties.PreCompilerAction() | ' .
			\ 'compiler spotbugs | ' .
			\ 'call g:spotbugs_properties.PostCompilerAction()'
	    elseif s:request == 2
		let s:actions[s:idx].cmd =
			\ 'compiler spotbugs | ' .
			\ 'call g:spotbugs_properties.PostCompilerAction()'
	    elseif s:request == 1
		let s:actions[s:idx].cmd =
			\ 'call g:spotbugs_properties.PreCompilerAction() | ' .
			\ 'compiler spotbugs'
	    else
		let s:actions[s:idx].cmd = ''
	    endif
	endfor

	let s:actions[0]['replace'] = v:true
	call autocmd_add(copy(s:actions))
	unlet s:idx s:actions
    endif

    unlet s:request
endif

" Undo the stuff we changed.
let b:undo_ftplugin = "setlocal suffixes< suffixesadd<" .
		\     " formatoptions< comments< commentstring< path< includeexpr<" .
		\     " | unlet! b:browsefilter" .
		\     " | silent! autocmd! java_spotbugs" .
		\     " | silent! augroup! java_spotbugs"

" See ":help vim9-mix".
if !has("vim9script")
    let &cpo = s:save_cpo
    unlet s:save_cpo
    finish
endif

if exists("s:zip_func_upgradable")
    delfunction! JavaFileTypeZipFile

    def! s:JavaFileTypeZipFile(): string
	@/ = substitute(v:fname, '\.', '\\/', 'g') .. '.java'
	return get(zip_files, bufnr('%'), zip_files[0])
    enddef

    setlocal includeexpr=s:JavaFileTypeZipFile()
    setlocal suffixesadd<
endif

" Restore the saved compatibility options.
let &cpo = s:save_cpo
unlet s:save_cpo
" vim: fdm=syntax sw=4 ts=8 noet sta
