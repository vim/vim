" Vim indent file
" Language:	    Makefile
" Maintainer:	    Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-26

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetMakeIndent()
setlocal indentkeys=!^F,o,O
setlocal nosmartindent

if exists("*GetMakeIndent")
  finish
endif

let s:rule_rx = '^[^ \t#:][^#:]*:\{1,2}\%([^=:]\|$\)'
let s:continuation_rx = '\\$'
let s:assignment_rx = '^\s*\h\w*\s*+\==\s*\zs.*\\$'

" TODO: Deal with comments, string, and all kinds of other crap, e.g., defines.
" TODO: Unwrap the whole logic of this function into something that requires a
" lot less 'return's.
function GetMakeIndent()
  let lnum = v:lnum - 1
  if lnum == 0
    return 0
  endif

  " Figure out if the previous line is part of a rule or not.  If it is, then
  " we more or less just indent by a 'tabstop', the previous' lines indent, or
  " remove all indent if the current line is itself a rule.  Also, if the line
  " in question is part of a continuation-line set constituting the rule line
  " itself, we indent by either a 'shiftwidth', if the line is the first in the
  " continuation, or use the indent of the previous line, if not.
  while lnum > 0
    let line = getline(lnum)
    if line[0] != "\t"
      " We found a non-shell-command line, i.e., one that doesn't have a
      " leading tab.
      if line =~ s:rule_rx
	" The line looks like a rule line, so we must therefore either be inside a
	" rule or we are a continuation line to that rule line.
	if line =~ s:continuation_rx
	  " Ah, the rule line was continued, so look up the last continuation
	  " line that's above the current line.
	  while line =~ s:continuation_rx && lnum < v:lnum
	    let lnum += 1
	    let line = getline(lnum)
	  endwhile
	  let lnum -= 1
	  let line = getline(lnum)
	endif

	" If the line that we've found is right above the current line, deal
	" with it specifically.
	if lnum == v:lnum - 1
	  " If it was continued, indent the current line by a shiftwidth, as it
	  " is the first to follow it.  Otherwise, depending on if the current
	  " line is a rule line, i.e, a rule line following another rule line,
	  " then indent to the left margin.  Otherwise, the current line is the
	  " first shell-command line in the rule, so indent by a 'tabstop'
	  if line =~ s:continuation_rx
	    return &sw
	  else
	    return getline(v:lnum) =~ s:rule_rx ? 0 : &ts
	  endif
	else
	  " If the previous line was a continuation line, then unless it was
	  " itself a part of a continuation line, add a 'shiftwidth''s worth of
	  " indent.  Otherwise, just use the indent of the previous line.
	  " Otherwise, if the previous line wasn't a continuation line, check
	  " if the one above it was.  If it was then indent to whatever level
	  " the 'owning' line had.  Otherwise, indent to the previous line's
	  " level.
	  let lnum = v:lnum - 1
	  let line = getline(lnum)
	  if line =~ s:continuation_rx
	    let pnum = v:lnum - 2
	    let pine = getline(pnum)
	    if pine =~ s:continuation_rx
	      return indent(lnum)
	    else
	      return indent(lnum) + &sw
	    endif
	  else
	    let lnum = v:lnum - 2
	    let line = getline(lnum)
	    if line =~ s:continuation_rx
	      while lnum > 0
		if line !~ s:continuation_rx
		  let lnum += 1
		  let line = getline(lnum)
		  break
		endif
		let lnum -= 1
		let line = getline(lnum)
	      endwhile
	      " We've found the owning line.  Indent to it's level.
	      return indent(lnum)
	    else
	      return indent(v:lnum - 1)
	    endif
	  endif
	endif
      endif

      " The line wasn't a rule line, so the current line is part of a series
      " of tab-indented lines that don't belong to any rule.
      break
    endif
    let lnum -= 1
  endwhile

  " If the line before the one we are currently indenting ended with a
  " continuation, then try to figure out what 'owns' that line and indent
  " appropriately.
  let lnum = v:lnum - 1
  let line = getline(lnum)
  if line =~ s:continuation_rx
    let indent = indent(lnum)
    if line =~ s:assignment_rx
      " The previous line is a continuation line that begins a variable-
      " assignment expression, so set the indent to just beyond the whitespace
      " following the assignment operator ('=').
      call cursor(lnum, 1)
      if search(s:assignment_rx, 'W') != 0
	let indent = virtcol('.') - 1
      endif
    endif

    " The previous line didn't constitute an assignment, so just indent to
    " whatever level it had.
    return indent
  endif

  " If the line above the line above the current line ended was continued,
  " then the line above the current line was part of a continued line.  Find
  " the 'owning' line and indent to its level.
  let lnum = v:lnum - 2
  let line = getline(lnum)
  if line =~ s:continuation_rx
    while lnum > 0
      if line !~ s:continuation_rx
	let lnum += 1
	let line = getline(lnum)
	break
      endif
      let lnum -= 1
      let line = getline(lnum)
    endwhile
    " We've found the owning line.  Indent to it's level.
    return indent(lnum)
  endif

  " If nothing else caught on, then check if this line is a rule line.  If it
  " is, indent it to the left margin.  Otherwise, simply use the indent of the
  " previous line.
  let line = getline(v:lnum)
  if line =~ s:rule_rx
    return 0
  else
    return indent(v:lnum - 1)
  endif
endfunction
