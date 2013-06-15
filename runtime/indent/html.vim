" Vim indent script for HTML
" General: "{{{
" File:		html.vim (Vimscript #2075)
" Author:	Andy Wokula <anwoku@yahoo.de>
" Last Change:	2013 Jun 12
" Rev Days:     13
" Version:	0.9
" Vim Version:	Vim7
" Description:
"   Improved version of the distributed html indent script, faster on a
"   range of lines.
"
" Credits:
"	indent/html.vim (2006 Jun 05) from J. Zellner
"	indent/css.vim (2006 Dec 20) from N. Weibull
"
" History:
" 2012 Oct 21	(v0.9) added support for shiftwidth()
" 2011 Sep 09	(v0.8) added HTML5 tags (thx to J. Zuckerman)
" 2008 Apr 28	(v0.6) revised customization
" 2008 Mar 09	(v0.5) fixed 'indk' issue (thx to C.J. Robinson)
" }}}

" Init Folklore, check user settings (2nd time ++) "{{{
if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal indentexpr=HtmlIndent()
setlocal indentkeys=o,O,<Return>,<>>,{,},!^F

let b:indent = {"lnum": -1}
let b:undo_indent = "set inde< indk<| unlet b:indent"

" Load Once:
if exists("*HtmlIndent")
    call HtmlIndent_CheckUserSettings()
    finish
endif

" Patch 7.3.694
if exists('*shiftwidth')
    let s:ShiftWidth = function('shiftwidth')
else
    func! s:ShiftWidth()
	return &shiftwidth
    endfunc
endif

let s:cpo_save = &cpo
set cpo-=C
"}}}

func! HtmlIndent_CheckUserSettings() "{{{
    if exists("g:html_indent_inctags")
	call s:AddITags(split(g:html_indent_inctags, ","))
    endif
    if exists("g:html_indent_autotags")
	call s:RemoveITags(split(g:html_indent_autotags, ","))
    endif

    let indone = {"zero": 0
		\,"auto": "indent(prevnonblank(v:lnum-1))"
		\,"inc": "b:indent.blocktagind + s:ShiftWidth()"}
    if exists("g:html_indent_script1")
	let s:js1indent = get(indone, g:html_indent_script1, indone.zero)
    endif
    if exists("g:html_indent_style1")
	let s:css1indent = get(indone, g:html_indent_style1, indone.zero)
    endif
endfunc "}}}

" Init Script Vars  "{{{
let s:usestate = 1
let s:css1indent = 0
let s:js1indent = 0
" not to be changed:
let s:endtags = [0,0,0,0,0,0,0,0]   " some places unused
let s:newstate = {}
let s:countonly = 0
 "}}}
func! s:AddITags(taglist) "{{{
    for itag in a:taglist
	let s:indent_tags[itag] = 1
	let s:indent_tags['/'.itag] = -1
    endfor
endfunc "}}}
func! s:AddBlockTag(tag, id, ...) "{{{
    if !(a:id >= 2 && a:id < 2+len(s:endtags))
	return
    endif
    let s:indent_tags[a:tag] = a:id
    if a:0 == 0
	let s:indent_tags['/'.a:tag] = -a:id
	let s:endtags[a:id-2] = "</".a:tag.">"
    else
	let s:indent_tags[a:1] = -a:id
	let s:endtags[a:id-2] = a:1
    endif
endfunc "}}}
func! s:RemoveITags(taglist) "{{{
    " remove itags (protect blocktags from being removed)
    for itag in a:taglist
	if !has_key(s:indent_tags, itag) || s:indent_tags[itag] != 1
	    continue
	endif
	unlet s:indent_tags[itag]
	if itag =~ '^\w\+$'
	    unlet s:indent_tags["/".itag]
	endif
    endfor
endfunc "}}}
" Add Indent Tags: {{{
if !exists("s:indent_tags")
    let s:indent_tags = {}
endif

" old tags:
call s:AddITags(['a', 'abbr', 'acronym', 'address', 'b', 'bdo', 'big',
    \ 'blockquote', 'button', 'caption', 'center', 'cite', 'code', 'colgroup',
    \ 'del', 'dfn', 'dir', 'div', 'dl', 'em', 'fieldset', 'font', 'form',
    \ 'frameset', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'i', 'iframe', 'ins', 'kbd',
    \ 'label', 'legend', 'map', 'menu', 'noframes', 'noscript', 'object', 'ol',
    \ 'optgroup', 'q', 's', 'samp', 'select', 'small', 'span', 'strong', 'sub',
    \ 'sup', 'table', 'textarea', 'title', 'tt', 'u', 'ul', 'var', 'th', 'td',
    \ 'tr', 'tfoot', 'thead'])

" tags added 2011 Sep 09 (especially HTML5 tags):
call s:AddITags(['area', 'article', 'aside', 'audio', 'bdi', 'canvas',
    \ 'command', 'datalist', 'details', 'embed', 'figure', 'footer',
    \ 'header', 'group', 'keygen', 'mark', 'math', 'meter', 'nav', 'output',
    \ 'progress', 'ruby', 'section', 'svg', 'texture', 'time', 'video',
    \ 'wbr', 'text'])

"}}}
" Add Block Tags: contain alien content "{{{
call s:AddBlockTag('pre', 2)
call s:AddBlockTag('script', 3)
call s:AddBlockTag('style', 4)
call s:AddBlockTag('<!--', 5, '-->')
"}}}

func! s:CountITags(...) "{{{

    " relative indent steps for current line [unit &sw]:
    let s:curind = 0
    " relative indent steps for next line [unit &sw]:
    let s:nextrel = 0

    if a:0==0
	let s:block = s:newstate.block
	let tmpline = substitute(s:curline, '<\zs\/\=\w\+\>\|<!--\|-->', '\=s:CheckTag(submatch(0))', 'g')
	if s:block == 3
	    let s:newstate.scripttype = s:GetScriptType(matchstr(tmpline, '\C.*<SCRIPT\>\zs[^>]*'))
	endif
	let s:newstate.block = s:block
    else
	let s:block = 0		" assume starting outside of a block
	let s:countonly = 1	" don't change state
	let tmpline = substitute(s:altline, '<\zs\/\=\w\+\>\|<!--\|-->', '\=s:CheckTag(submatch(0))', 'g')
	let s:countonly = 0
    endif
endfunc "}}}
func! s:CheckTag(itag) "{{{
    " "tag" or "/tag" or "<!--" or "-->"
    let ind = get(s:indent_tags, a:itag)
    if ind == -1
	" closing tag
	if s:block != 0
	    " ignore itag within a block
	    return "foo"
	endif
	if s:nextrel == 0
	    let s:curind -= 1
	else
	    let s:nextrel -= 1
	endif
	" if s:curind >= 1
	"     let s:curind -= 1
	" else
	"     let s:nextrel -= 1
	" endif
    elseif ind == 1
	" opening tag
	if s:block != 0
	    return "foo"
	endif
	let s:nextrel += 1
    elseif ind != 0
	" block-tag (opening or closing)
	return s:Blocktag(a:itag, ind)
    endif
    " else ind==0 (other tag found): keep indent
    return "foo"   " no matter
endfunc "}}}
func! s:Blocktag(blocktag, ind) "{{{
    if a:ind > 0
	" a block starts here
	if s:block != 0
	    " already in a block (nesting) - ignore
	    " especially ignore comments after other blocktags
	    return "foo"
	endif
	let s:block = a:ind		" block type
	if s:countonly
	    return "foo"
	endif
	let s:newstate.blocklnr = v:lnum
	" save allover indent for the endtag
	let s:newstate.blocktagind = b:indent.baseindent + (s:nextrel + s:curind) * s:ShiftWidth()
	if a:ind == 3
	    return "SCRIPT"    " all except this must be lowercase
	    " line is to be checked again for the type attribute
	endif
    else
	let s:block = 0
	" we get here if starting and closing block-tag on same line
    endif
    return "foo"
endfunc "}}}
func! s:GetScriptType(str) "{{{
    if a:str == "" || a:str =~ "java"
	return "javascript"
    else
	return ""
    endif
endfunc "}}}

func! s:FreshState(lnum) "{{{
    " Look back in the file (lines 1 to a:lnum-1) to calc a state for line
    " a:lnum.  A state is to know ALL relevant details about the lines
    " 1..a:lnum-1, initial calculating (here!) can be slow, but updating is
    " fast (incremental).
    " State:
    "	lnum		last indented line == prevnonblank(a:lnum - 1)
    "	block = 0	a:lnum located within special tag: 0:none, 2:<pre>,
    "			3:<script>, 4:<style>, 5:<!--
    "	baseindent	use this indent for line a:lnum as a start - kind of
    "			autoindent (if block==0)
    "	scripttype = ''	type attribute of a script tag (if block==3)
    "	blocktagind	indent for current opening (get) and closing (set)
    "			blocktag (if block!=0)
    "	blocklnr	lnum of starting blocktag (if block!=0)
    "	inattr		line {lnum} starts with attributes of a tag
    let state = {}
    let state.lnum = prevnonblank(a:lnum - 1)
    let state.scripttype = ""
    let state.blocktagind = -1
    let state.block = 0
    let state.baseindent = 0
    let state.blocklnr = 0
    let state.inattr = 0

    if state.lnum == 0
	return state
    endif

    " Heuristic:
    " remember startline state.lnum
    " look back for <pre, </pre, <script, </script, <style, </style tags
    " remember stopline
    " if opening tag found,
    "	assume a:lnum within block
    " else
    "	look back in result range (stopline, startline) for comment
    "	    \ delimiters (<!--, -->)
    "	if comment opener found,
    "	    assume a:lnum within comment
    "	else
    "	    assume usual html for a:lnum
    "	    if a:lnum-1 has a closing comment
    "		look back to get indent of comment opener
    " FI

    " look back for blocktag
    call cursor(a:lnum, 1)
    let [stopline, stopcol] = searchpos('\c<\zs\/\=\%(pre\>\|script\>\|style\>\)', "bW")
    " fugly ... why isn't there searchstr()
    let tagline = tolower(getline(stopline))
    let blocktag = matchstr(tagline, '\/\=\%(pre\>\|script\>\|style\>\)', stopcol-1)
    if stopline > 0 && blocktag[0] != "/"
	" opening tag found, assume a:lnum within block
	let state.block = s:indent_tags[blocktag]
	if state.block == 3
	    let state.scripttype = s:GetScriptType(matchstr(tagline, '\>[^>]*', stopcol))
	endif
	let state.blocklnr = stopline
	" check preceding tags in the line:
	let s:altline = tagline[: stopcol-2]
	call s:CountITags(1)
	let state.blocktagind = indent(stopline) + (s:curind + s:nextrel) * s:ShiftWidth()
	return state
    elseif stopline == state.lnum
	" handle special case: previous line (= state.lnum) contains a
	" closing blocktag which is preceded by line-noise;
	" blocktag == "/..."
	let swendtag = match(tagline, '^\s*</') >= 0
	if !swendtag
	    let [bline, bcol] = searchpos('<'.blocktag[1:].'\>', "bW")
	    let s:altline = tolower(getline(bline)[: bcol-2])
	    call s:CountITags(1)
	    let state.baseindent = indent(bline) + (s:nextrel+s:curline) * s:ShiftWidth()
	    return state
	endif
    endif

    " else look back for comment
    call cursor(a:lnum, 1)
    let [comline, comcol, found] = searchpos('\(<!--\)\|-->', 'bpW', stopline)
    if found == 2
	" comment opener found, assume a:lnum within comment
	let state.block = 5
	let state.blocklnr = comline
	" check preceding tags in the line:
	let s:altline = tolower(getline(comline)[: comcol-2])
	call s:CountITags(1)
	let state.blocktagind = indent(comline) + (s:curind + s:nextrel) * s:ShiftWidth()
	return state
    endif

    " else within usual html
    let s:altline = tolower(getline(state.lnum))
    " check a:lnum-1 for closing comment (we need indent from the opening line)
    let comcol = stridx(s:altline, '-->')
    if comcol >= 0
	call cursor(state.lnum, comcol+1)
	let [comline, comcol] = searchpos('<!--', 'bW')
	if comline == state.lnum
	    let s:altline = s:altline[: comcol-2]
	else
	    let s:altline = tolower(getline(comline)[: comcol-2])
	endif
	call s:CountITags(1)
	let state.baseindent = indent(comline) + (s:nextrel+s:curline) * s:ShiftWidth()
	return state
	" TODO check tags that follow "-->"
    endif

    " else no comments
    call s:CountITags(1)
    let state.baseindent = indent(state.lnum) + s:nextrel * s:ShiftWidth()
    " line starts with end tag
    let swendtag = match(s:altline, '^\s*</') >= 0
    if !swendtag
	let state.baseindent += s:curind * s:ShiftWidth()
    endif
    return state
endfunc "}}}

func! s:Alien2() "{{{
    " <pre> block
    return -1
endfunc "}}}
func! s:Alien3() "{{{
    " <script> javascript
    if prevnonblank(v:lnum-1) == b:indent.blocklnr
	" indent for the first line after <script>
	return eval(s:js1indent)
    endif
    if b:indent.scripttype == "javascript"
	return cindent(v:lnum)
    else
	return -1
    endif
endfunc "}}}
func! s:Alien4() "{{{
    " <style>
    if prevnonblank(v:lnum-1) == b:indent.blocklnr
	" indent for first content line
	return eval(s:css1indent)
    endif
    return s:CSSIndent()
endfunc

func! s:CSSIndent() "{{{
    " adopted $VIMRUNTIME/indent/css.vim
    if getline(v:lnum) =~ '^\s*[*}]'
	return cindent(v:lnum)
    endif
    let minline = b:indent.blocklnr
    let pnum = s:css_prevnoncomment(v:lnum - 1, minline)
    if pnum <= minline
	" < is to catch errors
	" indent for first content line after comments
	return eval(s:css1indent)
    endif
    let ind = indent(pnum) + s:css_countbraces(pnum, 1) * s:ShiftWidth()
    let pline = getline(pnum)
    if pline =~ '}\s*$'
	let ind -= (s:css_countbraces(pnum, 0) - (pline =~ '^\s*}')) * s:ShiftWidth()
    endif
    return ind
endfunc "}}}
func! s:css_prevnoncomment(lnum, stopline) "{{{
    " caller starts from a line a:lnum-1 that is not a comment
    let lnum = prevnonblank(a:lnum)
    let ccol = match(getline(lnum), '\*/')
    if ccol < 0
	return lnum
    endif
    call cursor(lnum, ccol+1)
    let lnum = search('/\*', 'bW', a:stopline)
    if indent(".") == virtcol(".")-1
	return prevnonblank(lnum-1)
    else
	return lnum
    endif
endfunc "}}}
func! s:css_countbraces(lnum, count_open) "{{{
    let brs = substitute(getline(a:lnum),'[''"].\{-}[''"]\|/\*.\{-}\*/\|/\*.*$\|[^{}]','','g')
    let n_open = 0
    let n_close = 0
    for brace in split(brs, '\zs')
	if brace == "{"
	    let n_open += 1
	elseif brace == "}"
	    if n_open > 0
		let n_open -= 1
	    else
		let n_close += 1
	    endif
	endif
    endfor
    return a:count_open ? n_open : n_close
endfunc "}}}

"}}}
func! s:Alien5() "{{{
    " <!-- -->
    return -1
endfunc "}}}

func! HtmlIndent() "{{{
    let s:curline = tolower(getline(v:lnum))
    let indentunit = s:ShiftWidth()

    let s:newstate = {}
    let s:newstate.lnum = v:lnum

    " does the line start with a closing tag?
    let swendtag = match(s:curline, '^\s*</') >= 0

    if prevnonblank(v:lnum-1) == b:indent.lnum && s:usestate
	" use state (continue from previous line)
    else
	" start over (know nothing)
	let b:indent = s:FreshState(v:lnum)
    endif

    if b:indent.block >= 2
	" within block
	let endtag = s:endtags[b:indent.block-2]
	let blockend = stridx(s:curline, endtag)
	if blockend >= 0
	    " block ends here
	    let s:newstate.block = 0
	    " calc indent for REST OF LINE (may start more blocks):
	    let s:curline = strpart(s:curline, blockend+strlen(endtag))
	    call s:CountITags()
	    if swendtag && b:indent.block != 5
		let indent = b:indent.blocktagind + s:curind * indentunit
		let s:newstate.baseindent = indent + s:nextrel * indentunit
	    else
		let indent = s:Alien{b:indent.block}()
		let s:newstate.baseindent = b:indent.blocktagind + s:nextrel * indentunit
	    endif
	    call extend(b:indent, s:newstate, "force")
	    return indent
	else
	    " block continues
	    " indent this line with alien method
	    let indent = s:Alien{b:indent.block}()
	    call extend(b:indent, s:newstate, "force")
	    return indent
	endif
    else
	" not within a block - within usual html
	" if < 2 then always 0
	let s:newstate.block = b:indent.block
	call s:CountITags()
	if swendtag
	    let indent = b:indent.baseindent + s:curind * indentunit
	    let s:newstate.baseindent = indent + s:nextrel * indentunit
	else
	    let indent = b:indent.baseindent
	    let s:newstate.baseindent = indent + (s:curind + s:nextrel) * indentunit
	endif
	call extend(b:indent, s:newstate, "force")
	return indent
    endif

endfunc "}}}

" check user settings (first time), clear cpo, Modeline: {{{1

" DEBUG:
com! -nargs=* IndHtmlLocal <args>

call HtmlIndent_CheckUserSettings()

let &cpo = s:cpo_save
unlet s:cpo_save

" vim:set fdm=marker ts=8:
