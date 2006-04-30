" Vim indent file generic utility functions
" Language:    * (various)
" Maintainer:  Dave Silvia <dsilvia@mchsi.com>
" Date:        6/30/2004

" SUMMARY:  To use GenericIndent, indent/<your_filename>.vim would have the
"           following general format:
"
"      if exists("b:did_indent") | finish | endif
"      let b:did_indent = 1
"      runtime indent/GenericIndent.vim
"      let b:indentStmts=''
"      let b:dedentStmts=''
"      let b:allStmts=''
"      setlocal indentexpr=GenericIndent()
"      setlocal indentkeys=<your_keys>
"      call GenericIndentStmts(<your_stmts>)
"      call GenericDedentStmts(<your_stmts>)
"      call GenericAllStmts()
"
" END SUMMARY:

" NOTE:  b:indentStmts, b:dedentStmts, and b:allStmts need to be initialized
"        to '' before callin the functions because 'indent.vim' explicitly
"        'unlet's b:did_indent.  This means that the lists will compound if
"        you change back and forth between buffers.  This is true as of
"        version 6.3, 6/23/2004.
"
" NOTE:  By default, GenericIndent is case sensitive.
"        let b:case_insensitive=1 if you want to ignore case, e.g. DOS batch files

" The function 'GenericIndent' is data driven and handles most all cases of
" indent checking if you first set up the data.  To use this function follow
" the example below (taken from the file indent/MuPAD_source.vim)
"
" Before you start, source this file in indent/<your_script>.vim to have it
" define functions for your use.
"
"runtime indent/GenericIndent.vim
"
" The data is in 5 sets:
"
" First, set the data set 'indentexpr' to GenericIndent().
"
"setlocal indentexpr=GenericIndent()
"
" Second, set the data set 'indentkeys' to the keywords/expressions that need
" to be checked for 'indenting' _as_ they typed.
"
"setlocal indentkeys==end_proc,=else,=then,=elif,=end_if,=end_case,=until,=end_repeat,=end_domain,=end_for,=end_while,=end,o,O
"
" NOTE: 'o,O' at the end of the previous line says you wish to be called
" whenever a newline is placed in the buffer.  This allows the previous line
" to be checked for indentation parameters.
"
" Third, set the data set 'b:indentStmts' to the keywords/expressions that, when
" they are on a line  _when_  you  _press_  the  _<Enter>_  key,
" you wish to have the next line indented.
"
"call GenericIndentStmts('begin,if,then,else,elif,case,repeat,until,domain,do')
"
" Fourth, set the data set 'b:dedentStmts' to the keywords/expressions that, when
" they are on a line you are currently typing, you wish to have that line
" 'dedented' (having already been indented because of the previous line's
" indentation).
"
"call GenericDedentStmts('end_proc,then,else,elif,end_if,end_case,until,end_repeat,end_domain,end_for,end_while,end')
"
" Fifth, set the data set 'b:allStmts' to the concatenation of the third and
" fourth data sets, used for checking when more than one keyword/expression
" is on a line.
"
"call GenericAllStmts()
"
" NOTE:  GenericIndentStmts uses two variables: 'b:indentStmtOpen' and
" 'b:indentStmtClose' which default to '\<' and '\>' respectively.  You can
" set (let) these to any value you wish before calling GenericIndentStmts with
" your list.  Similarly, GenericDedentStmts uses 'b:dedentStmtOpen' and
" 'b:dedentStmtClose'.
"
" NOTE:  Patterns may be used in the lists passed to Generic[In|De]dentStmts
" since each element in the list is copied verbatim.
"
" Optionally, you can set the DEBUGGING flag within your script to have the
" debugging messages output.  See below for description.  This can also be set
" (let) from the command line within your editing buffer.
"
"let b:DEBUGGING=1
"
" See:
"      :h runtime
"      :set runtimepath ?
" to familiarize yourself with how this works and where you should have this
" file and your file(s) installed.
"
" For help with setting 'indentkeys' see:
"      :h indentkeys
" Also, for some good examples see 'indent/sh.vim' and 'indent/vim.vim' as
" well as files for other languages you may be familiar with.
"
"
" Alternatively, if you'd rather specify yourself, you can enter
" 'b:indentStmts', 'b:dedentStmts', and 'b:allStmts' 'literally':
"
"let b:indentStmts='\<begin\>\|\<if\>\|\<then\>\|\<else\>\|\<elif\>\|\<case\>\|\<repeat\>\|\<until\>\|\<domain\>\|\<do\>'
"let b:dedentStmts='\<end_proc\>\|\<else\>\|\<elif\>\|\<end_if\>\|\<end_case\>\|\<until\>\|\<end_repeat\>\|\<end_domain\>\|\<end_for\>\|\<end_while\>\|\<end\>'
"let b:allStmts=b:indentStmts.'\|'.b:dedentStmts
"
" This is only useful if you have particularly different parameters for
" matching each statement.

" RECAP:  From indent/MuPAD_source.vim
"
"if exists("b:did_indent") | finish | endif
"
"let b:did_indent = 1
"
"runtime indent/GenericIndent.vim
"
"setlocal indentexpr=GenericIndent()
"setlocal indentkeys==end_proc,=then,=else,=elif,=end_if,=end_case,=until,=end_repeat,=end_domain,=end_for,=end_while,=end,o,O
"call GenericIndentStmts('begin,if,then,else,elif,case,repeat,until,domain,do')
"call GenericDedentStmts('end_proc,then,else,elif,end_if,end_case,until,end_repeat,end_domain,end_for,end_while,end')
"call GenericAllStmts()
"
" END RECAP:

let s:hit=0
let s:lastVlnum=0
let s:myScriptName=expand("<sfile>:t")

if exists("*GenericIndent")
	finish
endif

function GenericAllStmts()
	let b:allStmts=b:indentStmts.'\|'.b:dedentStmts
	call DebugGenericIndent(expand("<sfile>").": "."b:indentStmts: ".b:indentStmts.", b:dedentStmts: ".b:dedentStmts.", b:allStmts: ".b:allStmts)
endfunction

function GenericIndentStmts(stmts)
	let Stmts=a:stmts
	let Comma=match(Stmts,',')
	if Comma == -1 || Comma == strlen(Stmts)-1
		echoerr "Must supply a comma separated list of at least 2 entries."
		echoerr "Supplied list: <".Stmts.">"
		return
	endif

	if !exists("b:indentStmtOpen")
		let b:indentStmtOpen='\<'
	endif
	if !exists("b:indentStmtClose")
		let b:indentStmtClose='\>'
	endif
	if !exists("b:indentStmts")
		let b:indentStmts=''
	endif
	if b:indentStmts != ''
		let b:indentStmts=b:indentStmts.'\|'
	endif
	call DebugGenericIndent(expand("<sfile>").": "."b:indentStmtOpen: ".b:indentStmtOpen.", b:indentStmtClose: ".b:indentStmtClose.", b:indentStmts: ".b:indentStmts.", Stmts: ".Stmts)
	let stmtEntryBegin=0
	let stmtEntryEnd=Comma
	let stmtEntry=strpart(Stmts,stmtEntryBegin,stmtEntryEnd-stmtEntryBegin)
	let Stmts=strpart(Stmts,Comma+1)
	let Comma=match(Stmts,',')
	let b:indentStmts=b:indentStmts.b:indentStmtOpen.stmtEntry.b:indentStmtClose
	while Comma != -1
		let stmtEntryEnd=Comma
		let stmtEntry=strpart(Stmts,stmtEntryBegin,stmtEntryEnd-stmtEntryBegin)
		let Stmts=strpart(Stmts,Comma+1)
		let Comma=match(Stmts,',')
		let b:indentStmts=b:indentStmts.'\|'.b:indentStmtOpen.stmtEntry.b:indentStmtClose
	endwhile
	let stmtEntry=Stmts
	let b:indentStmts=b:indentStmts.'\|'.b:indentStmtOpen.stmtEntry.b:indentStmtClose
endfunction

function GenericDedentStmts(stmts)
	let Stmts=a:stmts
	let Comma=match(Stmts,',')
	if Comma == -1 || Comma == strlen(Stmts)-1
		echoerr "Must supply a comma separated list of at least 2 entries."
		echoerr "Supplied list: <".Stmts.">"
		return
	endif

	if !exists("b:dedentStmtOpen")
		let b:dedentStmtOpen='\<'
	endif
	if !exists("b:dedentStmtClose")
		let b:dedentStmtClose='\>'
	endif
	if !exists("b:dedentStmts")
		let b:dedentStmts=''
	endif
	if b:dedentStmts != ''
		let b:dedentStmts=b:dedentStmts.'\|'
	endif
	call DebugGenericIndent(expand("<sfile>").": "."b:dedentStmtOpen: ".b:dedentStmtOpen.", b:dedentStmtClose: ".b:dedentStmtClose.", b:dedentStmts: ".b:dedentStmts.", Stmts: ".Stmts)
	let stmtEntryBegin=0
	let stmtEntryEnd=Comma
	let stmtEntry=strpart(Stmts,stmtEntryBegin,stmtEntryEnd-stmtEntryBegin)
	let Stmts=strpart(Stmts,Comma+1)
	let Comma=match(Stmts,',')
	let b:dedentStmts=b:dedentStmts.b:dedentStmtOpen.stmtEntry.b:dedentStmtClose
	while Comma != -1
		let stmtEntryEnd=Comma
		let stmtEntry=strpart(Stmts,stmtEntryBegin,stmtEntryEnd-stmtEntryBegin)
		let Stmts=strpart(Stmts,Comma+1)
		let Comma=match(Stmts,',')
		let b:dedentStmts=b:dedentStmts.'\|'.b:dedentStmtOpen.stmtEntry.b:dedentStmtClose
	endwhile
	let stmtEntry=Stmts
	let b:dedentStmts=b:dedentStmts.'\|'.b:dedentStmtOpen.stmtEntry.b:dedentStmtClose
endfunction

" Debugging function.  Displays messages in the command area which can be
" reviewed using ':messages'.  To turn it on use ':let b:DEBUGGING=1'.  Once
" on, turn off by using ':let b:DEBUGGING=0.  If you don't want it at all and
" feel it's slowing down your editing (you must have an _awfully_ slow
" machine!;-> ), you can just comment out the calls to it from 'GenericIndent'
" below.  No need to remove the function or the calls, tho', as you never can
" tell when they might come in handy!;-)
function DebugGenericIndent(msg)
  if exists("b:DEBUGGING") && b:DEBUGGING
		echomsg '['.s:hit.']'.s:myScriptName."::".a:msg
	endif
endfunction

function GenericIndent()
	" save ignore case option.  Have to set noignorecase for the match
	" functions to do their job the way we want them to!
	" NOTE: if you add a return to this function be sure you do
	"           if IgnoreCase | set ignorecase | endif
	"       before returning.  You can just cut and paste from here.
	let IgnoreCase=&ignorecase
	" let b:case_insensitive=1 if you want to ignore case, e.g. DOS batch files
	if !exists("b:case_insensitive")
		set noignorecase
	endif
	" this is used to let DebugGenericIndent display which invocation of the
	" function goes with which messages.
	let s:hit=s:hit+1
  let lnum=v:lnum
	let cline=getline(lnum)
	let lnum=prevnonblank(lnum)
	if lnum==0 | if IgnoreCase | set ignorecase | endif | return 0 | endif
	let pline=getline(lnum)
  let ndnt=indent(lnum)
	if !exists("b:allStmts")
		call GenericAllStmts()
	endif

	call DebugGenericIndent(expand("<sfile>").": "."cline=<".cline.">, pline=<".pline.">, lnum=".lnum.", v:lnum=".v:lnum.", ndnt=".ndnt)
	if lnum==v:lnum
		" current line, only check dedent
		"
		" just dedented this line, don't need to do it again.
		" another dedentStmts was added or an end%[_*] was completed.
		if s:lastVlnum==v:lnum
 			if IgnoreCase | set ignorecase | endif
			return ndnt
		endif
		let s:lastVlnum=v:lnum
		call DebugGenericIndent(expand("<sfile>").": "."Checking dedent")
		let srcStr=cline
		let dedentKeyBegin=match(srcStr,b:dedentStmts)
		if dedentKeyBegin != -1
			let dedentKeyEnd=matchend(srcStr,b:dedentStmts)
			let dedentKeyStr=strpart(srcStr,dedentKeyBegin,dedentKeyEnd-dedentKeyBegin)
			"only dedent if it's the beginning of the line
			if match(srcStr,'^\s*\<'.dedentKeyStr.'\>') != -1
				call DebugGenericIndent(expand("<sfile>").": "."It's the beginning of the line, dedent")
				let ndnt=ndnt-&shiftwidth
			endif
		endif
		call DebugGenericIndent(expand("<sfile>").": "."dedent - returning ndnt=".ndnt)
	else
		" previous line, only check indent
		call DebugGenericIndent(expand("<sfile>").": "."Checking indent")
		let srcStr=pline
		let indentKeyBegin=match(srcStr,b:indentStmts)
		if indentKeyBegin != -1
			" only indent if it's the last indentStmts in the line
			let allKeyBegin=match(srcStr,b:allStmts)
			let allKeyEnd=matchend(srcStr,b:allStmts)
			let allKeyStr=strpart(srcStr,allKeyBegin,allKeyEnd-allKeyBegin)
			let srcStr=strpart(srcStr,allKeyEnd)
			let allKeyBegin=match(srcStr,b:allStmts)
			if allKeyBegin != -1
				" not the end of the line, check what is and only indent if
				" it's an indentStmts
				call DebugGenericIndent(expand("<sfile>").": "."Multiple words in line, checking if last is indent")
				while allKeyBegin != -1
					let allKeyEnd=matchend(srcStr,b:allStmts)
					let allKeyStr=strpart(srcStr,allKeyBegin,allKeyEnd-allKeyBegin)
					let srcStr=strpart(srcStr,allKeyEnd)
					let allKeyBegin=match(srcStr,b:allStmts)
				endwhile
				if match(b:indentStmts,allKeyStr) != -1
					call DebugGenericIndent(expand("<sfile>").": "."Last word in line is indent")
					let ndnt=ndnt+&shiftwidth
				endif
			else
				" it's the last indentStmts in the line, go ahead and indent
				let ndnt=ndnt+&shiftwidth
			endif
		endif
		call DebugGenericIndent(expand("<sfile>").": "."indent - returning ndnt=".ndnt)
	endif
	if IgnoreCase | set ignorecase | endif
	return ndnt
endfunction


" TODO:  I'm open!
"
" BUGS:  You tell me!  Probably.  I just haven't found one yet or haven't been
"        told about one.
"
