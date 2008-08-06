" Vim filetype plugin file
" Language:	html
" Maintainer:	Dan Sharp <dwsharp at hotmail dot com>
" Last Changed: 2007 Nov 20
" URL:		http://mywebpage.netscape.com/sharppeople/vim/ftplugin

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

" Make sure the continuation lines below do not cause problems in
" compatibility mode.
let s:save_cpo = &cpo
set cpo-=C

setlocal matchpairs+=<:>
setlocal commentstring=<!--%s-->
setlocal comments=s:<!--,m:\ \ \ \ ,e:-->

if exists("g:ft_html_autocomment") && (g:ft_html_autocomment == 1)
    setlocal formatoptions-=t formatoptions+=croql
endif


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
    let line = ""
    while i < 10 && i < line("$")
        let line = getline(i)
        if line =~ '<!DOCTYPE.*\<DTD '
            break
        endif
        let i += 1
    endwhile
    if line =~ '<!DOCTYPE.*\<DTD '  " doctype line found above
        if line =~ ' HTML 3\.2'
            let b:html_omni_flavor = 'html32'
        elseif line =~ ' XHTML 1\.1'
            let b:html_omni_flavor = 'xhtml11'
        else    " two-step detection with strict/frameset/transitional
            if line =~ ' XHTML 1\.0'
                let b:html_omni_flavor = 'xhtml10'
            elseif line =~ ' HTML 4\.01'
                let b:html_omni_flavor = 'html401'
            elseif line =~ ' HTML 4.0\>'
                let b:html_omni_flavor = 'html40'
            endif
            if line =~ '\<Transitional\>'
                let b:html_omni_flavor .= 't'
            elseif line =~ '\<Frameset\>'
                let b:html_omni_flavor .= 'f'
            else
                let b:html_omni_flavor .= 's'
            endif
        endif
    endif
endif

" HTML:  thanks to Johannes Zellner and Benji Fisher.
if exists("loaded_matchit")
    let b:match_ignorecase = 1
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
let b:undo_ftplugin = "setlocal commentstring< matchpairs< omnifunc< comments< formatoptions<" .
    \	" | unlet! b:match_ignorecase b:match_skip b:match_words b:browsefilter"

" Restore the saved compatibility options.
let &cpo = s:save_cpo
