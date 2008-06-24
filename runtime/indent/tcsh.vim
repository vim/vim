" Vim indent file
" Language:		C-shell (tcsh)
" Maintainer:		Gautam Iyer <gautam@math.uchicago.edu>
" Last Modified:	Sat 16 Jun 2007 04:27:45 PM PDT

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
    finish
endif

let b:did_indent = 1

setlocal indentexpr=TcshGetIndent()
setlocal indentkeys+=e,0=end,0=endsw indentkeys-=0{,0},0),:,0#

" Only define the function once.
if exists("*TcshGetIndent")
    finish
endif

set cpoptions-=C

function TcshGetIndent()
    " Find a non-blank line above the current line.
    let lnum = prevnonblank(v:lnum - 1)

    " Hit the start of the file, use zero indent.
    if lnum == 0
	return 0
    endif

    " Add indent if previous line begins with while or foreach
    " OR line ends with case <str>:, default:, else, then or \
    let ind = indent(lnum)
    let line = getline(lnum)
    if line =~ '\v^\s*%(while|foreach)>|^\s*%(case\s.*:|default:|else)\s*$|%(<then|\\)$'
	let ind = ind + &sw
    endif

    if line =~ '\v^\s*breaksw>'
	let ind = ind - &sw
    endif

    " Subtract indent if current line has on end, endif, case commands
    let line = getline(v:lnum)
    if line =~ '\v^\s*%(else|end|endif)\s*$'
	let ind = ind - &sw
    endif

    return ind
endfunction
