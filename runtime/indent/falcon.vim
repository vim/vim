" Vim indent file
" Language: Falcon
" Maintainer: Steven Oliver <oliver.steven@gmail.com>
" Website: https://steveno@github.com/steveno/falconpl-vim.git
" Credits: Thanks to the ruby.vim authors, I borrow a lot!
" Previous Maintainer: Brent A. Fulgham <bfulgham@debian.org>
" -----------------------------------------------------------
" GetLatestVimScripts: 2752 1 :AutoInstall: falcon.vim

"======================================
"       SETUP
"======================================

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal nosmartindent

" Setup indent function and when to use it
setlocal indentexpr=FalconGetIndent()
setlocal indentkeys=0{,0},0),0],!^F,o,O,e
setlocal indentkeys+==~case,=~catch,=~default,=~elif,=~else,=~end,=~\"

" Define the appropriate indent function but only once
if exists("*FalconGetIndent")
    finish
endif

let s:cpo_save = &cpo
set cpo&vim

"======================================
"       VARIABLES
"======================================

" Regex of syntax group names that are strings AND comments
let s:syng_strcom = '\<falcon\%(String\|StringEscape\|Comment\)\>'

" Regex of syntax group names that are strings
let s:syng_string = '\<falcon\%(String\|StringEscape\)\>'

" Keywords to indent on
let s:falcon_indent_keywords = '^\s*\(case\|catch\|class\|enum\|default\|elif\|else' .
    \ '\|for\|function\|if.*"[^"]*:.*"\|if \(\(:\)\@!.\)*$\|loop\|object\|select' .
    \ '\|switch\|try\|while\|\w*\s*=\s*\w*([$\)'

" Keywords to deindent on
let s:falcon_deindent_keywords = '^\s*\(case\|catch\|default\|elif\|else\|end\)'

"======================================
"       FUNCTIONS
"======================================

" Check if the character at lnum:col is inside a string
function s:IsInStringOrComment(lnum, col)
    return synIDattr(synID(a:lnum, a:col, 1), 'name') =~ s:syng_strcom
endfunction

"======================================
"       INDENT ROUTINE
"======================================

function FalconGetIndent()
    " Get the line to be indented
    let cline = getline(v:lnum)

    " Don't reindent comments on first column
    if cline =~ '^\/\/'
        return 0
    endif

    " Find the previous non-blank line
    let lnum = prevnonblank(v:lnum - 1)

    " Use zero indent at the top of the file
    if lnum == 0
        return 0
    endif

    let prevline=getline(lnum)
    let ind = indent(lnum)
    let chg = 0

    " If we are in a multi-line string or line-comment, don't do anything
    if s:IsInStringOrComment(v:lnum, matchend(cline, '^\s*') + 1 )
        return indent('.')
    endif

    " If the start of the line equals a double quote, then indent to the
    " previous lines first double quote
    if cline =~? '^\s*"'
        let chg = chg + &sw
    endif

    " If previous line started with a double quote and this one
    " doesn't, unindent
    if prevline =~? '^\s*"' && cline =~? '^\s*'
        let chg = chg - &sw
    endif

    " Indent if proper keyword
    if prevline =~? s:falcon_indent_keywords
        let chg = &sw
    " If previous line opened a parenthesis, and did not close it, indent
    elseif prevline =~ '^.*(\s*[^)]*\((.*)\)*[^)]*$'
        " Make sure this isn't just a function split between two lines
        if prevline =~ ',\s*$'
            return indent(prevnonblank(v:lnum - 1)) + &sw
        else
            return match(prevline, '(.*\((.*)\|[^)]\)*.*$') + 1
        endif
    elseif prevline =~ '^[^(]*)\s*$'
        " This line closes a parenthesis. Finds opening.
        let curr_line = prevnonblank(lnum - 1)
        while curr_line >= 0
            let str = getline(curr_line)
            if str !~ '^.*(\s*[^)]*\((.*)\)*[^)]*$'
                let curr_line = prevnonblank(curr_line - 1)
            else
                break
            endif
        endwhile
        if curr_line < 0
            return -1
        endif
        let ind = indent(curr_line)
    endif

    " If previous line ends in a semi-colon reset indent to previous
    " lines setting
    if prevline =~? ';\s*$' && prevnonblank(prevline) =~? ',\s*$'
        return chg = chg - (2 * &sw)
    endif

    " If previous line ended in a comma, indent again
    if prevline =~? ',\s*$'
        let chg = chg + &sw
    endif

    " If previous line ended in a =>, indent again
    if prevline =~? '=>\s*$'
        let chg = chg + &sw
    endif

    " Deindent on proper keywords
    if cline =~? s:falcon_deindent_keywords
        let chg = chg - &sw
    endif

    return ind + chg
endfunction

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: set sw=4 sts=4 et tw=80 :
