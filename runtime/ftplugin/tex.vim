" LaTeX filetype plugin
" Language:     LaTeX (ft=tex)
" Maintainer:   Benji Fisher, Ph.D. <benji@member.AMS.org>
" Version:	1.2
" Last Change:	Tue 11 May 2004 04:49:20 PM EDT
"  URL:		http://www.vim.org/script.php?script_id=411

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let s:save_cpo = &cpo
set cpo&vim

" This may be used to set b:tex_flavor.  A more complete version can be found
" in foo.vim (see http://www.vim.org/script.php?script_id=72).
if !exists("*s:GetModelines")
  fun! s:GetModelines(pat, ...)
    " Long but simple:  set start line and finish line.
    let EOF = line("$")
    if a:0 > 1
      let start = a:1 | let finish = a:2
    elseif a:0 == 1
      if a:1 > 0
	let finish = a:1
      else
	let start = EOF + a:1 + 1
      endif
    endif
    if !exists("start") || start < 1
      let start = 1
    endif
    if !exists("finish") || finish > EOF
      let finish = EOF
    endif
    let n = 0
    silent! execute start .",". finish
	  \ 'g/' . escape(a:pat, "/") . "/let n=line('.')"
    if n
      execute "normal!\<C-O>"
    endif
    return n . ":"
  endfun
endif " !exists("*GetModelines")

" Define the buffer-local variable b:tex_flavor to "tex" (for plain) or
" "latex".
" 1. Check the first line of the file for "%&<format>".
" 2. Check the first 1000 lines for "\begin{document}".
" 3. Check for a global variable g:tex_flavor, can be set in user's vimrc.
" 4. Default to "latex".
" 5. Strip "pdf" and change "plain" to "tex".
if getline(1) =~ '^%&\s*\k\+'
	let b:tex_flavor = matchstr(getline(1), '%&\s*\zs\k\+')
elseif s:GetModelines('\\begin\s*{\s*document\s*}', 1000) != "0:"
	let b:tex_flavor = "latex"
elseif exists("g:tex_flavor")
	let b:tex_flavor = g:tex_flavor
else
	let b:tex_flavor = "latex"
endif
let b:tex_flavor = substitute(b:tex_flavor, 'pdf', '', '')
if b:tex_flavor == "plain"
	let b:tex_flavor = "tex"
endif

" Set 'comments' to format dashed lists in comments
setlocal com=sO:%\ -,mO:%\ \ ,eO:%%,:%

" Set 'commentstring' to recognize the % comment character:
" (Thanks to Ajit Thakkar.)
setlocal cms=%%s

" Allow "[d" to be used to find a macro definition:
" Recognize plain TeX \def as well as LaTeX \newcommand and \renewcommand .
" I may as well add the AMS-LaTeX DeclareMathOperator as well.
let &l:define='\\\([egx]\|char\|mathchar\|count\|dimen\|muskip\|skip\|toks\)\='
	\ .	'def\|\\font\|\\\(future\)\=let'
	\ . '\|\\new\(count\|dimen\|skip\|muskip\|box\|toks\|read\|write'
	\ .	'\|fam\|insert\)'
	\ . '\|\\\(re\)\=new\(boolean\|command\|counter\|environment\|font'
	\ . '\|if\|length\|savebox\|theorem\(style\)\=\)\s*\*\=\s*{\='
	\ . '\|DeclareMathOperator\s*{\=\s*'

" Tell Vim how to recognize LaTeX \include{foo} and plain \input bar :
setlocal include=\\\\input\\\\|\\\\include{
setlocal suffixesadd=.tex
" On some file systems, "{" and "}" are inluded in 'isfname'.  In case the
" TeX file has \include{fname} (LaTeX only), strip everything except "fname".
let &l:includeexpr = "substitute(v:fname, '^.\\{-}{\\|}.*', '', 'g')"
" fun! TexIncludeExpr()
"   let fname = substitute(v:fname, '}.*', '', '')
"   return fname
" endfun

" The following lines enable the macros/matchit.vim plugin for
" extended matching with the % key.
" TODO:  Customize this based on b:tex_flavor .
if exists("loaded_matchit")
  let b:match_ignorecase = 0
    \ | let b:match_skip = 'r:\\\@<!\%(\\\\\)*%'
    \ | let b:match_words = '(:),\[:],{:},\\(:\\),\\\[:\\],' .
    \ '\\begin\s*\({\a\+\*\=}\):\\end\s*\1'
endif " exists("loaded_matchit")

let &cpo = s:save_cpo

" vim:sts=2:sw=2:
