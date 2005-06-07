" Vim indent file
" Language:    Pascal
" Maintainer:  Neil Carter <n.carter@swansea.ac.uk>
" Created:     2004 Jul 13
" Last Change: 2005 Jun 07
" TODO: Reduce indentation on line after a statement that flowed across
" two lines (e.g. parameter list closed on second line). Also, increase
" indent of a becomes-statement that flows onto second line.

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

setlocal indentexpr=GetPascalIndent(v:lnum)
" Appending an & to an option sets it to its default value.
setlocal indentkeys&
setlocal indentkeys+=~end;,=~const,=~type,=~var,=~begin,=~repeat,=~until,=~for
setlocal indentkeys+=~program,=~function,=~procedure,=~object,=~private
setlocal indentkeys+=~record,=~if,=~else,=~case

if exists("*GetPascalIndent")
	finish
endif


function s:GetPrevLineNum( line_num )
	
	" Skip over comments and conditional directives
	let SKIP_LINES = '^\s*\((\*\)\|\(\*\ \)\|\(\*)\)\|\({\$\)'
	
	let nline = a:line_num
	while nline > 0
		let nline = prevnonblank(nline-1)
		if getline(nline) !~? SKIP_LINES
			break
		endif
	endwhile
	
"	call input( "nline = ".nline )
	
	return nline

endfunction


function! GetPascalIndent( line_num )
	if a:line_num == 0
		return 0
	endif

	" If in the middle of a three-part comment
	if getline( a:line_num ) =~ '^\s*\*\ '
		return indent( a:line_num )
	endif
	
	" We have to subtract one to start on the line before the current
	" one. Otherwise, prevnonblank() returns the current line!
	let prev_line_num = s:GetPrevLineNum( a:line_num )
	let prev_line = getline( prev_line_num )
	let indnt = indent( prev_line_num )

	let this_line = getline( a:line_num )

	" At the start of a block, we have to indent the newly-created line
	" based on the previous line.
	" =~ means matches a regular expression
	" a question mark after =~ means ignore case (# means match case)
	" const, type, var should always appear at the start of a line, but
	" begin can appear anywhere in the line.
	" if one of the following keywords appear in the previous line with
	" nothing before it but optional whitespace, and nothing after it.
	" Has to be end of line at end to show this is not a routine
	" parameter list. Otherwise, you'd end up with cascading vars.
	
	" These words appear alone on a line (apart from whitespace).
	if prev_line =~ '^\s*\(const\|var\|begin\|repeat\|private\)$'
		" Place an & before an option to obtain its value.
		let indnt = indnt + &shiftwidth
	endif

	" Words preceded by optional whitespace and followed by anything.
	if prev_line =~ '^\s*\(for\|if\|else\|case\)'
		" Place an & before an option to obtain its value.
		let indnt = indnt + &shiftwidth
		" if this is a multistatement block then we need to align the
		" begin with the previous line.
		if this_line =~ '^\s*begin'
			let indnt = indnt - &shiftwidth
		endif
	endif
	" These words may have text before them on the line (hence the .*).
	if prev_line =~ '^.*\s*\<\(object\|record\)\>$'
		let indnt = indnt + &shiftwidth
	endif
	" If we have opened a bracket and the contents spills over one line,
	" then indent one level beyond the bracket's first line. RE = an
	" opening bracket followed by any amount of anything other than a
	" closing bracket and then the end-of-line. If we didn't include the
	" end of line, this RE would match even closed brackets, since it
	" would match everything up to the closing bracket.
	" This test isn't clever enough to handle brackets inside strings or
	" comments.
	if prev_line =~ '([^*][^)]*$'
		let indnt = indnt + &shiftwidth
	endif
	
	" If we just closed a bracket that started on a previous line, then
	" unindent.
	if prev_line =~ '^[^(]*[^*])'
		let indnt = indnt - &shiftwidth
	endif

	" At the end of a block, we have to unindent both the current line
	" (the 'end;' for instance) and the newly-created line.
	if this_line =~ '^\s*\(end;\|until\|else\)'
		let indnt = indnt - &shiftwidth
	endif

	" Keywords that always appear at the start of a line.
	" Problem is that function and procedure keywords should be indented
	" if within a class declaration.
	if this_line =~ '^\s*\<type\|uses\|$IFDEF\|$ENDIF\|procedure\|function\>'
		let indnt = 0
	endif
	if prev_line =~ '^\s*\<type\|uses\>'
		let indnt = &shiftwidth
	endif
	
	" Put conditional compile directives on first column.
	if this_line =~ '^\s*{\$'
		let indnt = 0
	endif
	
	return indnt
endfunction

" TODO: end; should align with the previous (begin/record/object/else).
" "else begin" is the only case where begin does not appear at the start
" of the line.

" TODO: Don't align with {$IFDEF}

"Example from vb.vim
" regular expression match, case insensitive
"if previous_line =~? 
" start of line, zero or more whitespace
"'^\s*
" start of word
"\<
" 
"\(
"	begin\|
"	\%(
"		\%(
"			private\|public\|friend
"		\)
"		\s\+
"	\)
"	zero or more of the previous atom
"	\=
"	\%(
"		function\|sub\|property
"	\)
"	\|select\|case\|default\|if
"\>
"	.\{-}\<then\>\s*$\|else\|elseif\|do\|for\|while\|enum\|with
"\)
" end of word
"\>'
"	let ind = ind + &sw
"endif
