" Vim indent file
" Language:	PHP
" Author:	John Wellesz <John.wellesz (AT) teaser (DOT) fr>
" URL:		http://www.2072productions.com/vim/indent/php.vim
" Last Change: 2005 Aug 15
" Version: 1.17
"
" For a complete change log and lots of comments in the code, download the script on
" 2072productions.com at the URI provided above.
" 
"
" 
"  If you find a bug, please e-mail me at John.wellesz (AT) teaser (DOT) fr
"  with an example of code that break the algorithm.
"
"
"	Thanks a lot for using this script.
"
"
" NOTE: This script must be used with PHP syntax ON and with the php syntax
"		script by Lutz Eymers (http://www.isp.de/data/php.vim ) that's the script bundled with Gvim.
"
"
"	In the case you have syntax errors in your script such as end of HereDoc
"	tags not at col 1 you'll have to indent your file 2 times (This script 
"	will automatically put HereDoc end tags at col 1).
" 
"
" NOTE: If you are editing file in Unix file format and that (by accident)
" there are '\r' before new lines, this script won't be able to proceed
" correctly and will make many mistakes because it won't be able to match
" '\s*$' correctly.
" So you have to remove those useless characters first with a command like:
"
" :%s /\r$//g
"
" or simply 'let' the option PHP_removeCRwhenUnix to 1 and the script will
" silently remove them when VIM load this script (at each bufread).

" Options: PHP_default_indenting = # of sw (default is 0), # of sw will be
"		   added to the indent of each line of PHP code.
"
" Options: PHP_removeCRwhenUnix = 1 to make the script automatically remove CR
"		   at end of lines (by default this option is unset), NOTE that you
"		   MUST remove CR when the fileformat is UNIX else the indentation
"		   won't be correct...
"
" Options: PHP_BracesAtCodeLevel = 1 to indent the '{' and '}' at the same
"		   level than the code they contain.
"		   Exemple:
"			Instead of:
"				if ($foo)
"				{
"					foo();
"				}
"
"			You will write:
"				if ($foo)
"					{
"					foo();
"					}
"
"			NOTE: The script will be a bit slower if you use this option because
"			some optimizations won't be available.


if exists("b:did_indent")
	finish
endif
let b:did_indent = 1

"	This script set the option php_sync_method of PHP syntax script to 0
"	(fromstart indenting method) in order to have an accurate syntax.
"	If you are using very big PHP files (which is a bad idea) you will
"	experience slowings down while editing, if your code contains only PHP
"	code you can comment the line below.

let php_sync_method = 0


if exists("PHP_default_indenting")
	let b:PHP_default_indenting = PHP_default_indenting * &sw
else
	let b:PHP_default_indenting = 0
endif

if exists("PHP_BracesAtCodeLevel")
	let b:PHP_BracesAtCodeLevel = PHP_BracesAtCodeLevel
else
	let b:PHP_BracesAtCodeLevel = 0
endif


let b:PHP_lastindented = 0
let b:PHP_indentbeforelast = 0
let b:PHP_indentinghuge = 0
let b:PHP_CurrentIndentLevel = b:PHP_default_indenting
let b:PHP_LastIndentedWasComment = 0
let b:PHP_InsideMultilineComment = 0
let b:InPHPcode = 0
let b:InPHPcode_checked = 0
let b:InPHPcode_and_script = 0
let b:InPHPcode_tofind = ""
let b:PHP_oldchangetick = b:changedtick
let b:UserIsTypingComment = 0
let b:optionsset = 0

setlocal nosmartindent
setlocal noautoindent 
setlocal nocindent
setlocal nolisp " autoindent must be on, so this line is also useless...

setlocal indentexpr=GetPhpIndent()
setlocal indentkeys=0{,0},0),:,!^F,o,O,e,*<Return>,=?>,=<?,=*/


if version <= 603 && &encoding == 'utf-8'
	let s:searchpairflags = 'bW'
else
	let s:searchpairflags = 'bWr'
endif

if &fileformat == "unix" && exists("PHP_removeCRwhenUnix") && PHP_removeCRwhenUnix
	silent! %s/\r$//g
endif

if exists("*GetPhpIndent")
	finish " XXX
endif

let s:endline= '\s*\%(//.*\|#.*\|/\*.*\*/\s*\)\=$'
let s:PHP_startindenttag = '<?\%(.*?>\)\@!\|<script[^>]*>\%(.*<\/script>\)\@!'
"setlocal debug=msg " XXX


function! GetLastRealCodeLNum(startline) " {{{
	"Inspired from the function SkipJavaBlanksAndComments by Toby Allsopp for indent/java.vim 
	let lnum = a:startline
	let old_lnum = lnum

	while lnum > 1
		let lnum = prevnonblank(lnum)
		let lastline = getline(lnum)

		if b:InPHPcode_and_script && lastline =~ '?>\s*$'
			let lnum = lnum - 1
		elseif lastline =~ '^\s*?>.*<?\%(php\)\=\s*$'
			let lnum = lnum - 1
		elseif lastline =~ '^\s*\%(//\|#\|/\*.*\*/\s*$\)' " if line is under comment
			let lnum = lnum - 1
		elseif lastline =~ '\*/\s*$' " skip multiline comments
			call cursor(lnum, 1)
			call search('\*/\zs', 'W') " positition the cursor after the first */
			let lnum = searchpair('/\*', '', '\*/\zs', s:searchpairflags) " find the most outside /*

			let lastline = getline(lnum)
			if lastline =~ '^\s*/\*' " if line contains nothing but comment
				let lnum = lnum - 1 " do the job again on the line before (a comment can hide another...)
			else
				break
			endif

			
		elseif lastline =~? '\%(//\s*\|?>.*\)\@<!<?\%(php\)\=\s*$\|^\s*<script\>' " skip non php code

			while lastline !~ '\(<?.*\)\@<!?>' && lnum > 1
				let lnum = lnum - 1
				let lastline = getline(lnum)
			endwhile
			if lastline =~ '^\s*?>' " if line contains nothing but end tag 
				let lnum = lnum - 1
			else
				break " else there is something important before the ?>
			endif


		elseif lastline =~? '^\a\w*;$' && lastline !~? s:notPhpHereDoc " match the end of a heredoc
			let tofind=substitute( lastline, '\([^;]\+\);', '<<<\1$', '')
			while getline(lnum) !~? tofind && lnum > 1
				let lnum = lnum - 1
			endwhile
		else
			break " if none of these were true then we are done
		endif
	endwhile

	if lnum==1 && getline(lnum)!~ '<?'
		let lnum=0
	endif
	
	if b:InPHPcode_and_script && !b:InPHPcode
		let b:InPHPcode_and_script = 0
	endif
	return lnum
endfunction
" }}}

function! Skippmatch()  " {{{
	let synname = synIDattr(synID(line("."), col("."), 0), "name")
	if synname == "Delimiter" || synname == "phpParent" || synname == "javaScriptBraces" || synname == "phpComment" && b:UserIsTypingComment
		return 0
	else
		return 1
	endif
endfun
" }}}

function! FindOpenBracket(lnum) " {{{
	call cursor(a:lnum, 1) " set the cursor to the start of the lnum line
	return searchpair('{', '', '}', 'bW', 'Skippmatch()')
endfun
" }}}

function! FindTheIfOfAnElse (lnum, StopAfterFirstPrevElse) " {{{
" A very clever recoursive function created by me (John Wellesz) that find the "if" corresponding to an
" "else". This function can easily be adapted for other languages :)
	
	if getline(a:lnum) =~# '^\s*}\s*else\%(if\)\=\>'
		let beforeelse = a:lnum " we do this so we can find the opened bracket to speed up the process
	else
		let beforeelse = GetLastRealCodeLNum(a:lnum - 1)
	endif

	if !s:level
		let s:iftoskip = 0
	endif

	if getline(beforeelse) =~# '^\s*\%(}\s*\)\=else\%(\s*if\)\@!\>'
		let s:iftoskip = s:iftoskip + 1
	endif
	
	if getline(beforeelse) =~ '^\s*}'
		let beforeelse = FindOpenBracket(beforeelse)

		if getline(beforeelse) =~ '^\s*{'
			let beforeelse = GetLastRealCodeLNum(beforeelse - 1)
		endif
	endif


	if !s:iftoskip && a:StopAfterFirstPrevElse && getline(beforeelse) =~# '^\s*\%([}]\s*\)\=else\%(if\)\=\>'
		return beforeelse
	endif

	if getline(beforeelse) !~# '^\s*if\>' && beforeelse>1 || s:iftoskip && beforeelse>1
		
		if  s:iftoskip && getline(beforeelse) =~# '^\s*if\>'
			let s:iftoskip = s:iftoskip - 1
		endif

		let s:level =  s:level + 1
		let beforeelse = FindTheIfOfAnElse(beforeelse, a:StopAfterFirstPrevElse)
	endif

	return beforeelse

endfunction
" }}}

function! IslinePHP (lnum, tofind) " {{{
	let cline = getline(a:lnum)

	if a:tofind==""
		let tofind = "^\\s*[\"']*\s*\\zs\\S" " This correct the issue where lines beginning by a 
		" single or double quote were not indented in some cases.
	else
		let tofind = a:tofind
	endif

	let tofind = tofind . '\c' " ignorecase

	let coltotest = match (cline, tofind) + 1 "find the first non blank char in the current line
	
	let synname = synIDattr(synID(a:lnum, coltotest, 0), "name") " ask to syntax what is its name

	if synname =~ '^php' || synname=="Delimiter" || synname =~? '^javaScript'
		return synname
	else
		return ""
	endif
endfunction
" }}}

let s:notPhpHereDoc = '\%(break\|return\|continue\|exit\);'
let s:blockstart = '\%(\%(\%(}\s*\)\=else\%(\s\+\)\=\)\=if\>\|while\>\|switch\>\|for\%(each\)\=\>\|declare\>\|[|&]\)'

let s:autorestoptions = 0
if ! s:autorestoptions
	au BufWinEnter,Syntax	*.php,*.php3,*.php4,*.php5	call ResetOptions()
	let s:autorestoptions = 1
endif

function! ResetOptions()
	if ! b:optionsset
		setlocal formatoptions=qroc
		let b:optionsset = 1
	endif
endfunc

function! GetPhpIndent()
	"##############################################
	"########### MAIN INDENT FUNCTION #############
	"##############################################

	let UserIsEditing=0
	if 	b:PHP_oldchangetick != b:changedtick
		let b:PHP_oldchangetick = b:changedtick
		let UserIsEditing=1
	endif

	if b:PHP_default_indenting
		let b:PHP_default_indenting = g:PHP_default_indenting * &sw
	endif

	let cline = getline(v:lnum) " current line

	if !b:PHP_indentinghuge && b:PHP_lastindented > b:PHP_indentbeforelast 
		if b:PHP_indentbeforelast
			let b:PHP_indentinghuge = 1
			echom 'Large indenting detected, speed optimizations engaged'
		endif
		let b:PHP_indentbeforelast = b:PHP_lastindented
	endif

	if b:InPHPcode_checked && prevnonblank(v:lnum - 1) != b:PHP_lastindented
		if b:PHP_indentinghuge
			echom 'Large indenting deactivated'
			let b:PHP_indentinghuge = 0
			let b:PHP_CurrentIndentLevel = b:PHP_default_indenting
		endif
		let b:PHP_lastindented = v:lnum
		let b:PHP_LastIndentedWasComment=0
		let b:PHP_InsideMultilineComment=0
		let b:PHP_indentbeforelast = 0
		
		let b:InPHPcode = 0
		let b:InPHPcode_checked = 0
		let b:InPHPcode_and_script = 0
		let b:InPHPcode_tofind = ""

	elseif v:lnum > b:PHP_lastindented " we are indenting line in > order (we can rely on the line before)
		let real_PHP_lastindented = b:PHP_lastindented
		let b:PHP_lastindented = v:lnum
	endif


	if !b:InPHPcode_checked " {{{ One time check
		let b:InPHPcode_checked = 1

		let synname = IslinePHP (prevnonblank(v:lnum), "") " the line could be blank (if the user presses 'return')

		if synname!=""
			if synname != "phpHereDoc"
				let b:InPHPcode = 1
				let b:InPHPcode_tofind = ""

				if synname == "phpComment"
					let b:UserIsTypingComment = 1
				else
					let b:UserIsTypingComment = 0
				endif

				if synname =~? '^javaScript'
					let b:InPHPcode_and_script = 1
				endif

			else
				let b:InPHPcode = 0
				let b:UserIsTypingComment = 0

				let lnum = v:lnum - 1
				while getline(lnum) !~? '<<<\a\w*$' && lnum > 1
					let lnum = lnum - 1
				endwhile

				let b:InPHPcode_tofind = substitute( getline(lnum), '^.*<<<\(\a\w*\)\c', '^\\s*\1;$', '')
			endif
		else " IslinePHP returned "" => we are not in PHP or Javascript
			let b:InPHPcode = 0
			let b:UserIsTypingComment = 0
			" Then we have to find a php start tag...
			let b:InPHPcode_tofind = '<?\%(.*?>\)\@!\|<script.*>'
		endif
	endif "!b:InPHPcode_checked }}}


	let lnum = prevnonblank(v:lnum - 1)
	let last_line = getline(lnum)

	if b:InPHPcode_tofind!=""
		if cline =~? b:InPHPcode_tofind
			let	b:InPHPcode = 1
			let b:InPHPcode_tofind = ""
			let b:UserIsTypingComment = 0
			if cline =~ '\*/' " End comment tags must be indented like start comment tags
				call cursor(v:lnum, 1)
				call search('\*/\zs', 'W')
				let lnum = searchpair('/\*', '', '\*/\zs', s:searchpairflags) " find the most outside /*

				let b:PHP_CurrentIndentLevel = b:PHP_default_indenting
				let b:PHP_LastIndentedWasComment = 0 " prevent a problem if multiline /**/ comment are surounded by
													 " other types of comments
				
				if cline =~ '^\s*\*/'
					return indent(lnum) + 1
				else
					return indent(lnum)
				endif

			elseif cline =~? '<script\>' " a more accurate test is useless since there isn't any other possibility
				let b:InPHPcode_and_script = 1
			endif
		endif
	endif


	if b:InPHPcode

		if !b:InPHPcode_and_script && last_line =~ '\%(<?.*\)\@<!?>\%(.*<?\)\@!' && IslinePHP(lnum, '?>')=="Delimiter"
			if cline !~? s:PHP_startindenttag
				let b:InPHPcode = 0
				let b:InPHPcode_tofind = s:PHP_startindenttag
			elseif cline =~? '<script\>'
				let b:InPHPcode_and_script = 1
			endif

		elseif last_line =~? '<<<\a\w*$' 
			let b:InPHPcode = 0
			let b:InPHPcode_tofind = substitute( last_line, '^.*<<<\(\a\w*\)\c', '^\\s*\1;$', '')

		elseif !UserIsEditing && cline =~ '^\s*/\*\%(.*\*/\)\@!' && getline(v:lnum + 1) !~ '^\s*\*' " XXX indent comments
			let b:InPHPcode = 0
			let b:InPHPcode_tofind = '\*/'

		elseif cline =~? '^\s*</script>'
			let b:InPHPcode = 0
			let b:InPHPcode_tofind = s:PHP_startindenttag
		endif
	endif " }}}

	if !b:InPHPcode && !b:InPHPcode_and_script
		return -1
	endif


	" Indent successive // or # comment the same way the first is {{{
	if cline =~ '^\s*\%(//\|#\|/\*.*\*/\s*$\)'
		if b:PHP_LastIndentedWasComment == 1
			return indent(real_PHP_lastindented) " line replaced in 1.02
		endif
		let b:PHP_LastIndentedWasComment = 1
	else
		let b:PHP_LastIndentedWasComment = 0
	endif
	" }}}
	
	" Indent multiline /* comments correctly {{{
	

	if b:PHP_InsideMultilineComment || b:UserIsTypingComment
		if cline =~ '^\s*\*\%(\/\)\@!'   " if cline == '*'
			if last_line =~ '^\s*/\*' " if last_line == '/*'
				return indent(lnum) + 1
			else
				return indent(lnum)
			endif
		else
			let b:PHP_InsideMultilineComment = 0
		endif
	endif
	
	if !b:PHP_InsideMultilineComment && cline =~ '^\s*/\*' " if cline == '/*'
		let b:PHP_InsideMultilineComment = 1
		return -1
	endif
	" }}}

	if cline =~# '^\s*<?' && cline !~ '?>' " Added the ^\s* part in version 1.03
		return 0
	endif

	if  cline =~ '^\s*?>' && cline !~# '<?'  
		return 0
	endif

	if cline =~? '^\s*\a\w*;$' && cline !~? s:notPhpHereDoc
		return 0
	endif
	" }}}

	let s:level = 0

	let lnum = GetLastRealCodeLNum(v:lnum - 1)
	let last_line = getline(lnum)    " last line
	let ind = indent(lnum) " by default
	let endline= s:endline

	if ind==0 && b:PHP_default_indenting
		let ind = b:PHP_default_indenting
	endif

	if lnum == 0
		return b:PHP_default_indenting
	endif


	if cline =~ '^\s*}\%(}}\)\@!'
		let ind = indent(FindOpenBracket(v:lnum))
		let b:PHP_CurrentIndentLevel = b:PHP_default_indenting
		return ind
	endif

	if cline =~ '^\s*\*/' " End comment tags must be indented like start comment tags
		call cursor(v:lnum, 1)
		call search('\*/\zs', 'W')
		let lnum = searchpair('/\*', '', '\*/\zs', s:searchpairflags) " find the most outside /*

		let b:PHP_CurrentIndentLevel = b:PHP_default_indenting

		if cline =~ '^\s*\*/'
			return indent(lnum) + 1
		else
			return indent(lnum)
		endif
	endif

	let defaultORcase = '^\s*\%(default\|case\).*:'

	if last_line =~ '[;}]'.endline && last_line !~# defaultORcase 
		if ind==b:PHP_default_indenting " if no indentation for the previous line
			return b:PHP_default_indenting
		elseif b:PHP_indentinghuge && ind==b:PHP_CurrentIndentLevel && cline !~# '^\s*\%(else\|\%(case\|default\).*:\|[})];\=\)' && last_line !~# '^\s*\%(\%(}\s*\)\=else\)' && getline(GetLastRealCodeLNum(lnum - 1))=~';'.endline
			return b:PHP_CurrentIndentLevel
		endif
	endif

	let LastLineClosed = 0 " used to prevent redundant tests in the last part of the script

	let terminated = '\%(;\%(\s*?>\)\=\|<<<\a\w*\|}\)'.endline

	let unstated   = '\%(^\s*'.s:blockstart.'.*)\|\%(//.*\)\@<!\<e'.'lse\>\)'.endline

	if ind != b:PHP_default_indenting && cline =~# '^\s*else\%(if\)\=\>'
		let b:PHP_CurrentIndentLevel = b:PHP_default_indenting " prevent optimized to work at next call
		return indent(FindTheIfOfAnElse(v:lnum, 1))
	elseif last_line =~# unstated && cline !~ '^\s*{\|^\s*);\='.endline
		let ind = ind + &sw
		return ind


	elseif ind != b:PHP_default_indenting && last_line =~ terminated
		let previous_line = last_line
		let last_line_num = lnum
		let LastLineClosed = 1


		while 1
			if previous_line =~ '^\s*}'
				let last_line_num = FindOpenBracket(last_line_num)

				if getline(last_line_num) =~ '^\s*{'
					let last_line_num = GetLastRealCodeLNum(last_line_num - 1)
				endif

				let previous_line = getline(last_line_num)

				continue
			else
				if getline(last_line_num) =~# '^\s*else\%(if\)\=\>'
					let last_line_num = FindTheIfOfAnElse(last_line_num, 0)
					continue " re-run the loop (we could find a '}' again)
				endif


				let last_match = last_line_num " remember the 'topest' line we found so far

				let one_ahead_indent = indent(last_line_num)
				let last_line_num = GetLastRealCodeLNum(last_line_num - 1)
				let two_ahead_indent = indent(last_line_num)
				let after_previous_line = previous_line
				let previous_line = getline(last_line_num)


				if previous_line =~# defaultORcase.'\|{'.endline
					break
				endif

				if after_previous_line=~# '^\s*'.s:blockstart.'.*)'.endline && previous_line =~# '[;}]'.endline
					break
				endif

				if one_ahead_indent == two_ahead_indent || last_line_num < 1 
					if previous_line =~# '[;}]'.endline || last_line_num < 1
						break
					endif
				endif
			endif
		endwhile

		if indent(last_match) != ind " if nothing was done lets the old script continue
			let ind = indent(last_match) " let's use the indent of the last line matched by the alhorithm above
			let b:PHP_CurrentIndentLevel = b:PHP_default_indenting " line added in version 1.02 to prevent optimized mode
			" from acting in some special cases

			if cline =~# defaultORcase
				let ind = ind - &sw
			endif
			return ind
		endif
	endif

	let plinnum = GetLastRealCodeLNum(lnum - 1)
	let pline = getline(plinnum) " previous to last line

	let last_line = substitute(last_line,"\\(//\\|#\\)\\(\\(\\([^\"']*\\([\"']\\)[^\"']*\\5\\)\\+[^\"']*$\\)\\|\\([^\"']*$\\)\\)",'','')


	if ind == b:PHP_default_indenting
		if last_line =~ terminated
			let LastLineClosed = 1
		endif
	endif
	
	if !LastLineClosed " the last line isn't a .*; or a }$ line
		if last_line =~# '[{(]'.endline || last_line =~? '\h\w*\s*(.*,$' && pline !~ '[,(]'.endline

			if !b:PHP_BracesAtCodeLevel || last_line !~# '^\s*{' " XXX mod {
				let ind = ind + &sw
			endif

			if b:PHP_BracesAtCodeLevel || cline !~# defaultORcase " XXX mod (2) {
				" case and default are not indented inside blocks
				let b:PHP_CurrentIndentLevel = ind
				return ind
			endif

		elseif last_line =~ '\S\+\s*),'.endline
			call cursor(lnum, 1)
			call search('),'.endline, 'W')
			let openedparent = searchpair('(', '', ')', 'bW', 'Skippmatch()')
			if openedparent != lnum
				let ind = indent(openedparent)
			endif
			
		elseif cline !~ '^\s*{' && pline =~ '\%(;\%(\s*?>\)\=\|<<<\a\w*\|{\|^\s*'.s:blockstart.'\s*(.*)\)'.endline.'\|^\s*}\|'.defaultORcase
			
			let ind = ind + &sw

		endif
		if  b:PHP_BracesAtCodeLevel && cline =~# '^\s*{' " XXX mod {
			let ind = ind + &sw
		endif

	elseif last_line =~# defaultORcase
		let ind = ind + &sw
	endif

	if cline =~  '^\s*);\='
		let ind = ind - &sw
	elseif cline =~# defaultORcase
		let ind = ind - &sw
	
	endif

	let b:PHP_CurrentIndentLevel = ind
	return ind
endfunction

" vim: set ts=4 sw=4:
" vim: set ff=unix:
