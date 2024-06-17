vim9script

# Vim indent file
# Language:	VisualBasic (ft=vb) / Basic (ft=basic) / SaxBasic (ft=vb)
# Author:	Johannes Zellner <johannes@zellner.org>
# Maintainer:	Michael Soyka (mssr953@gmail.com)
# Contributors: Doug Kearns (dougkearns@gmail.com)
# Last Change:	Fri, 18 Jun 2004 07:22:42 CEST
#		Small update 2010 Jul 28 by Maxim Kim
#		2022/12/15: add support for multiline statements.
#		2022/12/21: move VbGetIndent from global to script-local scope
#		2022/12/26: recognize "Type" keyword
#		2023/07/13: correct/extend line continuation pattern (Doug Kearns)
#		2023/07/14: add more keywords; various optimizations (Doug Kearns)
#		2023/07/20: convert to Vim9 script
#		2023/07/23: improve detection of preproc directives (Doug Kearns)

if exists("b:did_indent")
    finish
endif
b:did_indent = v:true

setlocal autoindent
setlocal indentexpr=VbGetIndent()
setlocal indentkeys&
setlocal indentkeys+==~else,=~elseif,=~end,=~wend,=~case,=~next,=~select,=~loop

b:undo_indent = "setlocal autoindent< indentexpr< indentkeys<"

# Only define the function once.
if exists("*VbGetIndent")
    finish
endif

# These regular expressions identify statement labels and preprocessor
# directives.
#
const RE_LABEL: string = '^\s*\k\+:\s*$'
const RE_PREPROC: string =
    '^\s*#\%(const\|if\|elseif\|else\|end\|region\|enable\|disable\)\>'

# Microsoft documentation states that line continuation is indicated by a
# two-character sequence at end-of-line: a space character followed by an
# underscore.  Nonetheless, it has been reported that additional
# whitespace after the underscore is also allowed.  We will support both.
# However, VB 16.0 also permits a comment after the underscore which,
# for simplicity, we do not support.
#
const RE_LINE_CONTINUATION: string = '\s_\s*$'

# The following regular expressions are used to increase the indent
# after statements that open a new scope.
#
const RE_INCR_INDENT_1: string =
    '^\s*\%(begin\|select\|case\|default\|if\|else\|elseif\|do\|for\|while\|with\)\>'
const RE_INCR_INDENT_2: string =
    '^\s*\%(\%(private\|public\|friend\)\s\+\)\=\%(static\s\+\)\=\%(function\|sub\|property\)\>'
const RE_INCR_INDENT_3: string =
    '^\s*\%(\%(private\|public\)\s\+\)\=\%(enum\|type\)\>'

def VbGetIndent(): number
    var this_lnum: number = v:lnum
    var this_line: string = getline(this_lnum)
    var this_indent: number = 0

    # labels and preprocessor statements get zero indent immediately
    if (this_line =~? RE_LABEL) || (this_line =~? RE_PREPROC)
	return this_indent
    endif

    # Get the current value of 'shiftwidth'
    const SHIFTWIDTH: number = shiftwidth()

    # Find a non-blank line above the current line.
    # Skip over labels and preprocessor directives.
    var lnum: number = this_lnum
    var previous_line: string
    while lnum > 0
	lnum = prevnonblank(lnum - 1)
	previous_line = getline(lnum)
	if (previous_line !~? RE_LABEL) || (previous_line !~? RE_PREPROC)
	    break
	endif
    endwhile

    # Hit the start of the file, use zero indent.
    if lnum == 0
	return this_indent
    endif

    # Variable "previous_line" now contains the text in buffer line "lnum".

    # Multi-line statements have the underscore character at end-of-line:
    #
    #    object.method(arguments, _
    #                  arguments, _
    #                  arguments)
    #
    # and require extra logic to determine the correct indentation.
    #
    # Case 1: Line "lnum" is the first line of a multiline statement.
    #         Line "lnum" will have a trailing underscore character
    #         but the preceding non-blank line does not.
    #         Line "this_lnum" will be indented relative to "lnum".
    #
    # Case 2: Line "lnum" is the last line of a multiline statement.
    #         Line "lnum" will not have a trailing underscore character
    #         but the preceding non-blank line will.
    #         Line "this_lnum" will have the same indentation as the starting
    #         line of the multiline statement.
    #
    # Case 3: Line "lnum" is neither the first nor last line.
    #         Lines "lnum" and "lnum-1" will have a trailing underscore
    #         character.
    #         Line "this_lnum" will have the same indentation as the preceding
    #         line.
    #
    # No matter which case it is, the starting line of the statement must be
    # found.  It will be assumed that multiline statements cannot have
    # intermingled comments, statement labels, preprocessor directives or
    # blank lines.
    #
    var lnum_is_continued: bool = (previous_line =~? RE_LINE_CONTINUATION)
    var before_lnum: number
    var before_previous_line: string
    if lnum > 1
	before_lnum = prevnonblank(lnum - 1)
	before_previous_line = getline(before_lnum)
    else
	before_lnum = 0
	before_previous_line = ""
    endif

    if before_previous_line !~? RE_LINE_CONTINUATION
	# Variable "previous_line" contains the start of a statement.
	#
	this_indent = indent(lnum)
	if lnum_is_continued
	    this_indent += SHIFTWIDTH
	endif
    elseif ! lnum_is_continued
	# Line "lnum" contains the last line of a multiline statement.
        # Need to find where this multiline statement begins
	#
	while before_lnum > 0
	    before_lnum -= 1
	    if getline(before_lnum) !~? RE_LINE_CONTINUATION
		before_lnum += 1
		break
	    endif
	endwhile
	if before_lnum == 0
	    before_lnum = 1
	endif
	previous_line = getline(before_lnum)
	this_indent = indent(before_lnum)
    else
	# Line "lnum" is not the first or last line of a multiline statement.
	#
	this_indent = indent(lnum)
    endif

    # Increment indent
    if (previous_line =~? RE_INCR_INDENT_1) ||
       (previous_line =~? RE_INCR_INDENT_2) ||
       (previous_line =~? RE_INCR_INDENT_3)
	this_indent += SHIFTWIDTH
    endif

    # Decrement indent
    if this_line =~? '^\s*end\s\+select\>'
	if previous_line !~? '^\s*select\>'
	    this_indent -= 2 * SHIFTWIDTH
	else
	    # this case is for an empty 'select' -- 'end select'
	    # (w/o any case statements) like:
	    #
	    # select case readwrite
	    # end select
	    this_indent -= SHIFTWIDTH
	endif
    elseif this_line =~? '^\s*\%(end\|else\|elseif\|until\|loop\|next\|wend\)\>'
	this_indent -= SHIFTWIDTH
    elseif this_line =~? '^\s*\%(case\|default\)\>'
	if previous_line !~? '^\s*select\>'
	    this_indent -= SHIFTWIDTH
	endif
    endif

    return this_indent
enddef

# vim:sw=4
