" Vim filetype plugin file
" Language:	html
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Changed: 2006 Apr 26
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

setlocal commentstring=<!--%s-->

if exists('&omnifunc')
" Distinguish between HTML versions
" To use with other HTML versions add another
" elseif condition to match proper DOCTYPE
setlocal omnifunc=htmlcomplete#CompleteTags

if &filetype == 'xhtml'
	let b:html_omni_flavor = 'xhtml10s'
else
	let b:html_omni_flavor = 'html401t'
endif
let i = 1
while i < 10 && i < line("$")
	let line = getline(i)
	if line =~ '<!DOCTYPE.*\<DTD HTML 3\.2'
		let b:html_omni_flavor = 'html32'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.0 Transitional'
		let b:html_omni_flavor = 'html40t'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.0 Frameset'
		let b:html_omni_flavor = 'html40f'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.0'
		let b:html_omni_flavor = 'html40s'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.01 Transitional'
		let b:html_omni_flavor = 'html401t'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.01 Frameset'
		let b:html_omni_flavor = 'html401f'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD HTML 4\.01'
		let b:html_omni_flavor = 'html401s'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD XHTML 1\.0 Transitional'
		let b:html_omni_flavor = 'xhtml10t'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD XHTML 1\.0 Frameset'
		let b:html_omni_flavor = 'xhtml10f'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD XHTML 1\.0 Strict'
		let b:html_omni_flavor = 'xhtml10s'
		break
	elseif line =~ '<!DOCTYPE.*\<DTD XHTML 1\.1'
		let b:html_omni_flavor = 'xhtml11'
		break
	endif
	let i += 1
endwhile
endif

" HTML:  thanks to Johannes Zellner and Benji Fisher.
if exists("loaded_matchit")
    let b:match_ignorecase = 1
    let b:match_skip = 's:Comment'
    let b:match_words = '<:>,' .
    \ '<\@<=[ou]l\>[^>]*\%(>\|$\):<\@<=li\>:<\@<=/[ou]l>,' .
    \ '<\@<=dl\>[^>]*\%(>\|$\):<\@<=d[td]\>:<\@<=/dl>,' .
    \ '<\@<=\([^/][^ \t>]*\)[^>]*\%(>\|$\):<\@<=/\1>'
endif

" Change the :browse e filter to primarily show HTML-related files.
if has("gui_win32")
    let  b:browsefilter="HTML Files (*.html,*.htm)\t*.htm;*.html\n" .
		\	"JavaScript Files (*.js)\t*.js\n" .
		\	"Cascading StyleSheets (*.css)\t*.css\n" .
		\	"All Files (*.*)\t*.*\n"
endif

" Undo the stuff we changed.
let b:undo_ftplugin = "setlocal commentstring<"
    \	" | unlet! b:match_ignorecase b:match_skip b:match_words b:browsefilter"

" Restore the saved compatibility options.
let &cpo = s:save_cpo
