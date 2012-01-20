" Vim ftplugin file
" Language:     Erlang
" Author:       Oscar Hellström <oscar@oscarh.net>
" Contributors: Ricardo Catalinas Jiménez <jimenezrick@gmail.com>
"               Eduardo Lopez (http://github.com/tapichu)
" License:      Vim license
" Version:      2011/11/21

if exists('b:did_ftplugin')
	finish
else
	let b:did_ftplugin = 1
endif

if exists('s:did_function_definitions')
	call s:SetErlangOptions()
	finish
else
	let s:did_function_definitions = 1
endif

if !exists('g:erlang_keywordprg')
	let g:erlang_keywordprg = 'erl -man'
endif

if !exists('g:erlang_folding')
	let g:erlang_folding = 0
endif

" Local settings
function s:SetErlangOptions()
	compiler erlang
	if version >= 700
		setlocal omnifunc=erlang_complete#Complete
	endif

	if g:erlang_folding
		setlocal foldmethod=expr
		setlocal foldexpr=GetErlangFold(v:lnum)
		setlocal foldtext=ErlangFoldText()
	endif

	setlocal comments=:%%%,:%%,:%
	setlocal commentstring=%%s

	setlocal formatoptions+=ro
	let &l:keywordprg = g:erlang_keywordprg
endfunction

" Define folding functions
if !exists('*GetErlangFold')
	" Folding params
	let s:erlang_fun_begin  = '^\a\w*(.*$'
	let s:erlang_fun_end    = '^[^%]*\.\s*\(%.*\)\?$'
	let s:erlang_blank_line = '^\s*\(%.*\)\?$'

	" Auxiliary fold functions
	function s:GetNextNonBlank(lnum)
		let lnum = nextnonblank(a:lnum + 1)
		let line = getline(lnum)
		while line =~ s:erlang_blank_line && 0 != lnum
			let lnum = nextnonblank(lnum + 1)
			let line = getline(lnum)
		endwhile
		return lnum
	endfunction

	function s:GetFunName(str)
		return matchstr(a:str, '^\a\w*(\@=')
	endfunction

	function s:GetFunArgs(str, lnum)
		let str = a:str
		let lnum = a:lnum
		while str !~ '->\s*\(%.*\)\?$'
			let lnum = s:GetNextNonBlank(lnum)
			if 0 == lnum " EOF
				return ''
			endif
			let str .= getline(lnum)
		endwhile
		return matchstr(str, 
			\ '\(^(\s*\)\@<=.*\(\s*)\(\s\+when\s\+.*\)\?\s\+->\s*\(%.*\)\?$\)\@=')
	endfunction

	function s:CountFunArgs(arguments)
		let pos = 0
		let ac = 0 " arg count
		let arguments = a:arguments
		
		" Change list / tuples into just one A(rgument)
		let erlang_tuple = '{\([A-Za-z_,|=\-\[\]]\|\s\)*}'
		let erlang_list  = '\[\([A-Za-z_,|=\-{}]\|\s\)*\]'

		" FIXME: Use searchpair?
		while arguments =~ erlang_tuple
			let arguments = substitute(arguments, erlang_tuple, 'A', 'g')
		endwhile
		" FIXME: Use searchpair?
		while arguments =~ erlang_list
			let arguments = substitute(arguments, erlang_list, 'A', 'g')
		endwhile
		
		let len = strlen(arguments)
		while pos < len && pos > -1
			let ac += 1
			let pos = matchend(arguments, ',\s*', pos)
		endwhile
		return ac
	endfunction

	" Main fold function
	function GetErlangFold(lnum)
		let lnum = a:lnum
		let line = getline(lnum)

		if line =~ s:erlang_fun_end
			return '<1'
		endif

		if line =~ s:erlang_fun_begin && foldlevel(lnum - 1) == 1
			return '1'
		endif

		if line =~ s:erlang_fun_begin
			return '>1'
		endif

		return '='
	endfunction

	" Erlang fold description (foldtext function)
	function ErlangFoldText()
		let foldlen = v:foldend - v:foldstart
		if 1 < foldlen
			let lines = 'lines'
		else
			let lines = 'line'
		endif
		let line = getline(v:foldstart)
		let name = s:GetFunName(line)
		let arguments = s:GetFunArgs(strpart(line, strlen(name)), v:foldstart)
		let argcount = s:CountFunArgs(arguments)
		let retval = '+' . v:folddashes . ' ' . name . '/' . argcount
		let retval .= ' (' . foldlen . ' ' . lines . ')'
		return retval
	endfunction
endif

call s:SetErlangOptions()
