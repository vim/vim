" Vim indent file
" Language:	PHP
" Author:	John Wellesz <John.wellesz (AT) teaser (DOT) fr>
" URL:		http://www.2072productions.com/vim/indent/php.vim
" Last Change:  2006 Apr 30
" Newsletter:   http://www.2072productions.com/?to=php-indent-for-vim-newsletter.php
" Version:	1.23
"
"  The change log and all the comments have been removed from this file.
"
"  For a complete change log and fully commented code, download the script on
"  2072productions.com at the URI provided above.
"
"  If you find a bug, please e-mail me at John.wellesz (AT) teaser (DOT) fr
"  with an example of code that breaks the algorithm.
"
"
"	Thanks a lot for using this script.
"
"
" NOTE: This script must be used with PHP syntax ON and with the php syntax
"	script by Lutz Eymers ( http://www.isp.de/data/php.vim ) that's the script bundled with Vim.
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


" Options: PHP_autoformatcomment = 0 to not enable autoformating of comment by
"		    default, if set to 0, this script will let the 'formatoptions' setting intact.
"
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

if exists("PHP_autoformatcomment")
    let b:PHP_autoformatcomment = PHP_autoformatcomment
else
    let b:PHP_autoformatcomment = 1
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
setlocal nolisp

setlocal indentexpr=GetPhpIndent()
setlocal indentkeys=0{,0},0),:,!^F,o,O,e,*<Return>,=?>,=<?,=*/



let s:searchpairflags = 'bWr'

if &fileformat == "unix" && exists("PHP_removeCRwhenUnix") && PHP_removeCRwhenUnix
    silent! %s/\r$//g
endif

if exists("*GetPhpIndent")
    finish " XXX
endif

let s:endline= '\s*\%(//.*\|#.*\|/\*.*\*/\s*\)\=$'
let s:PHP_startindenttag = '<?\%(.*?>\)\@!\|<script[^>]*>\%(.*<\/script>\)\@!'
" setlocal debug=msg " XXX


function! GetLastRealCodeLNum(startline) " {{{

    let lnum = a:startline

    if b:GetLastRealCodeLNum_ADD && b:GetLastRealCodeLNum_ADD == lnum + 1
	let lnum = b:GetLastRealCodeLNum_ADD
    endif

    let old_lnum = lnum

    while lnum > 1
	let lnum = prevnonblank(lnum)
	let lastline = getline(lnum)

	if b:InPHPcode_and_script && lastline =~ '?>\s*$'
	    let lnum = lnum - 1
	elseif lastline =~ '^\s*?>.*<?\%(php\)\=\s*$'
	    let lnum = lnum - 1
	elseif lastline =~ '^\s*\%(//\|#\|/\*.*\*/\s*$\)'
	    let lnum = lnum - 1
	elseif lastline =~ '\*/\s*$'
	    call cursor(lnum, 1)
	    if lastline !~ '^\*/'
		call search('\*/', 'W')
	    endif
	    let lnum = searchpair('/\*', '', '\*/', s:searchpairflags, 'Skippmatch2()')

	    let lastline = getline(lnum)
	    if lastline =~ '^\s*/\*'
		let lnum = lnum - 1
	    else
		break
	    endif


	elseif lastline =~? '\%(//\s*\|?>.*\)\@<!<?\%(php\)\=\s*$\|^\s*<script\>'

	    while lastline !~ '\(<?.*\)\@<!?>' && lnum > 1
		let lnum = lnum - 1
		let lastline = getline(lnum)
	    endwhile
	    if lastline =~ '^\s*?>'
		let lnum = lnum - 1
	    else
		break
	    endif


	elseif lastline =~? '^\a\w*;$' && lastline !~? s:notPhpHereDoc
	    let tofind=substitute( lastline, '\([^;]\+\);', '<<<\1$', '')
	    while getline(lnum) !~? tofind && lnum > 1
		let lnum = lnum - 1
	    endwhile
	else
	    break
	endif
    endwhile

    if lnum==1 && getline(lnum)!~ '<?'
	let lnum=0
    endif

    if b:InPHPcode_and_script && !b:InPHPcode
	let b:InPHPcode_and_script = 0
    endif
    return lnum
endfunction " }}}

function! Skippmatch2()

    let line = getline(".")

   if line =~ '\%(".*\)\@<=/\*\%(.*"\)\@=' || line =~ '\%(//.*\)\@<=/\*'
       return 1
   else
       return 0
   endif
endfun

function! Skippmatch()  " {{{
    let synname = synIDattr(synID(line("."), col("."), 0), "name")
    if synname == "Delimiter" || synname == "phpParent" || synname == "javaScriptBraces" || synname == "phpComment" && b:UserIsTypingComment
	return 0
    else
	return 1
    endif
endfun " }}}

function! FindOpenBracket(lnum) " {{{
    call cursor(a:lnum, 1)
    return searchpair('{', '', '}', 'bW', 'Skippmatch()')
endfun " }}}

function! FindTheIfOfAnElse (lnum, StopAfterFirstPrevElse) " {{{

    if getline(a:lnum) =~# '^\s*}\s*else\%(if\)\=\>'
	let beforeelse = a:lnum
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

endfunction " }}}

function! IslinePHP (lnum, tofind) " {{{
    let cline = getline(a:lnum)

    if a:tofind==""
	let tofind = "^\\s*[\"']*\\s*\\zs\\S"
    else
	let tofind = a:tofind
    endif

    let tofind = tofind . '\c'

    let coltotest = match (cline, tofind) + 1

    let synname = synIDattr(synID(a:lnum, coltotest, 0), "name")

    if synname =~ '^php' || synname=="Delimiter" || synname =~? '^javaScript'
	return synname
    else
	return ""
    endif
endfunction " }}}

let s:notPhpHereDoc = '\%(break\|return\|continue\|exit\);'
let s:blockstart = '\%(\%(\%(}\s*\)\=else\%(\s\+\)\=\)\=if\>\|else\>\|while\>\|switch\>\|for\%(each\)\=\>\|declare\>\|class\>\|interface\>\|abstract\>\|try\>\|catch\>\|[|&]\)'

let s:autorestoptions = 0
if ! s:autorestoptions
    au BufWinEnter,Syntax	*.php,*.php3,*.php4,*.php5	call ResetOptions()
    let s:autorestoptions = 1
endif

function! ResetOptions()
    if ! b:optionsset
	if b:PHP_autoformatcomment

	    setlocal comments=s1:/*,mb:*,ex:*/,://,:#

	    setlocal formatoptions-=t
	    setlocal formatoptions+=q
	    setlocal formatoptions+=r
	    setlocal formatoptions+=o
	    setlocal formatoptions+=w
	    setlocal formatoptions+=c
	    setlocal formatoptions+=b
	endif
	let b:optionsset = 1
    endif
endfunc

function! GetPhpIndent()

    let b:GetLastRealCodeLNum_ADD = 0

    let UserIsEditing=0
    if	b:PHP_oldchangetick != b:changedtick
	let b:PHP_oldchangetick = b:changedtick
	let UserIsEditing=1
    endif

    if b:PHP_default_indenting
	let b:PHP_default_indenting = g:PHP_default_indenting * &sw
    endif

    let cline = getline(v:lnum)

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

    elseif v:lnum > b:PHP_lastindented
	let real_PHP_lastindented = b:PHP_lastindented
	let b:PHP_lastindented = v:lnum
    endif


    if !b:InPHPcode_checked " {{{ One time check
	let b:InPHPcode_checked = 1

	let synname = ""
	if cline !~ '<?.*?>'
	    let synname = IslinePHP (prevnonblank(v:lnum), "")
	endif

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
	else
	    let b:InPHPcode = 0
	    let b:UserIsTypingComment = 0
	    let b:InPHPcode_tofind = '<?\%(.*?>\)\@!\|<script.*>'
	endif
    endif "!b:InPHPcode_checked }}}


    " Test if we are indenting PHP code {{{
    let lnum = prevnonblank(v:lnum - 1)
    let last_line = getline(lnum)

    if b:InPHPcode_tofind!=""
	if cline =~? b:InPHPcode_tofind
	    let	b:InPHPcode = 1
	    let b:InPHPcode_tofind = ""
	    let b:UserIsTypingComment = 0
	    if cline =~ '\*/'
		call cursor(v:lnum, 1)
		if cline !~ '^\*/'
		    call search('\*/', 'W')
		endif
		let lnum = searchpair('/\*', '', '\*/', s:searchpairflags, 'Skippmatch2()')

		let b:PHP_CurrentIndentLevel = b:PHP_default_indenting

		let b:PHP_LastIndentedWasComment = 0

		if cline =~ '^\s*\*/'
		    return indent(lnum) + 1
		else
		    return indent(lnum)
		endif

	    elseif cline =~? '<script\>'
		let b:InPHPcode_and_script = 1
		let b:GetLastRealCodeLNum_ADD = v:lnum
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

	elseif !UserIsEditing && cline =~ '^\s*/\*\%(.*\*/\)\@!' && getline(v:lnum + 1) !~ '^\s*\*'
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
	    return indent(real_PHP_lastindented)
	endif
	let b:PHP_LastIndentedWasComment = 1
    else
	let b:PHP_LastIndentedWasComment = 0
    endif " }}}

    " Indent multiline /* comments correctly {{{

    if b:PHP_InsideMultilineComment || b:UserIsTypingComment
	if cline =~ '^\s*\*\%(\/\)\@!'
	    if last_line =~ '^\s*/\*'
		return indent(lnum) + 1
	    else
		return indent(lnum)
	    endif
	else
	    let b:PHP_InsideMultilineComment = 0
	endif
    endif

    if !b:PHP_InsideMultilineComment && cline =~ '^\s*/\*' && cline !~ '\*/\s*$'
	if getline(v:lnum + 1) !~ '^\s*\*'
	    return -1
	endif
	let b:PHP_InsideMultilineComment = 1
    endif " }}}


    " Things always indented at col 1 (PHP delimiter: <?, ?>, Heredoc end) {{{
    if cline =~# '^\s*<?' && cline !~ '?>'
	return 0
    endif

    if  cline =~ '^\s*?>' && cline !~# '<?'
	return 0
    endif

    if cline =~? '^\s*\a\w*;$' && cline !~? s:notPhpHereDoc
	return 0
    endif " }}}

    let s:level = 0

    let lnum = GetLastRealCodeLNum(v:lnum - 1)

    let last_line = getline(lnum)
    let ind = indent(lnum)
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

    if cline =~ '^\s*\*/'
	call cursor(v:lnum, 1)
	if cline !~ '^\*/'
	    call search('\*/', 'W')
	endif
	let lnum = searchpair('/\*', '', '\*/', s:searchpairflags, 'Skippmatch2()')

	let b:PHP_CurrentIndentLevel = b:PHP_default_indenting

	if cline =~ '^\s*\*/'
	    return indent(lnum) + 1
	else
	    return indent(lnum)
	endif
    endif

    let defaultORcase = '^\s*\%(default\|case\).*:'

    if last_line =~ '[;}]'.endline && last_line !~# defaultORcase
	if ind==b:PHP_default_indenting
	    return b:PHP_default_indenting
	elseif b:PHP_indentinghuge && ind==b:PHP_CurrentIndentLevel && cline !~# '^\s*\%(else\|\%(case\|default\).*:\|[})];\=\)' && last_line !~# '^\s*\%(\%(}\s*\)\=else\)' && getline(GetLastRealCodeLNum(lnum - 1))=~';'.endline
	    return b:PHP_CurrentIndentLevel
	endif
    endif

    let LastLineClosed = 0

    let terminated = '\%(;\%(\s*?>\)\=\|<<<\a\w*\|}\)'.endline

    let unstated   = '\%(^\s*'.s:blockstart.'.*)\|\%(//.*\)\@<!\<e'.'lse\>\)'.endline

    if ind != b:PHP_default_indenting && cline =~# '^\s*else\%(if\)\=\>'
	let b:PHP_CurrentIndentLevel = b:PHP_default_indenting
	return indent(FindTheIfOfAnElse(v:lnum, 1))
    elseif cline =~ '^\s*{'
	let previous_line = last_line
	let last_line_num = lnum

	while last_line_num > 1

	    if previous_line =~ '^\s*\%(' . s:blockstart . '\|\%([a-zA-Z]\s*\)*function\)' && previous_line !~ '^\s*[|&]'

		let ind = indent(last_line_num)

		if  b:PHP_BracesAtCodeLevel
		    let ind = ind + &sw
		endif

		return ind
	    endif

	    let last_line_num = last_line_num - 1
	    let previous_line = getline(last_line_num)
	endwhile

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
		    continue
		endif


		let last_match = last_line_num

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

	if indent(last_match) != ind
	    let ind = indent(last_match)
	    let b:PHP_CurrentIndentLevel = b:PHP_default_indenting

	    if cline =~# defaultORcase
		let ind = ind - &sw
	    endif
	    return ind
	endif
    endif

    let plinnum = GetLastRealCodeLNum(lnum - 1)
    let pline = getline(plinnum)

    let last_line = substitute(last_line,"\\(//\\|#\\)\\(\\(\\([^\"']*\\([\"']\\)[^\"']*\\5\\)\\+[^\"']*$\\)\\|\\([^\"']*$\\)\\)",'','')


    if ind == b:PHP_default_indenting
	if last_line =~ terminated
	    let LastLineClosed = 1
	endif
    endif

    if !LastLineClosed

	if last_line =~# '[{(]'.endline || last_line =~? '\h\w*\s*(.*,$' && pline !~ '[,(]'.endline

	    if !b:PHP_BracesAtCodeLevel || last_line !~# '^\s*{'
		let ind = ind + &sw
	    endif

	    if b:PHP_BracesAtCodeLevel || cline !~# defaultORcase
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

" vim: set ts=8 sw=4 sts=4:
