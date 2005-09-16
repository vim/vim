" Vim completion script
" Language:	XHTML 1.0 Strict
" Maintainer:	Mikolaj Machowski ( mikmach AT wp DOT pl )
" Last Change:	2005 Sep 15

function! htmlcomplete#CompleteTags(findstart, base)
  if a:findstart
    " locate the start of the word
    let line = getline('.')
    let start = col('.') - 1
    while start >= 0 && line[start - 1] !~ '<'
      let start -= 1
    endwhile
    return start
  else
	" Set attribute groups
    let g:coreattrs = ["id", "class", "style", "title"] 
    let g:i18n = ["lang", "xml:lang", "dir"]
    let g:events = ["onclick", "ondblclick", "onmousedown", "onmouseup", "onmousemove",
    			\ "onmouseout", "onkeypress", "onkeydown", "onkeyup"]
    let g:focus = ["accesskey", "tabindex", "onfocus", "onblur"]
    let g:coregroup = g:coreattrs
    let g:coregroup = extend(g:coregroup, g:i18n)
    let g:coregroup = extend(g:coregroup, g:events)
    " find tags matching with "a:base"
    let res = []
	" If a:base contains > it means we are already outside of tag and we
	" should abandon action
	if a:base =~ '>'
		return []
	endif
	" If a:base contains white space it is attribute. 
	" It could be also value of attribute...
	" Possible situations where any prediction would be difficult:
	" 1. Events attributes
	if a:base =~ '\s'
		" Sort out style, class, and on* cases
		" Perfect solution for style would be switching for CSS completion. Is
		" it possible?
		" Also retrieving class names from current file and linked
		" stylesheets.
		if a:base =~ "\\(on[a-z]*\\|style\\|class\\)\\s*=\\s*[\"']"
			let stripbase = matchstr(a:base, ".*\\(on[a-z]*\\|style\\|class\\)\\s*=\\s*[\"']\\zs.*")
			" Now we have a:base stripped from all chars up to style/class.
			" It may fail with some strange style value combinations.
			if stripbase !~ "[\"']"
				return []
			endif
		endif
		" We have to get first word to offer
		" proper attributes.
		let tag = split(a:base)[0]
		" Get last word, it should be attr name
		let attr = matchstr(a:base, '.*\s\zs.*')
		" If attr contains =\s*[\"'] we catched value of attribute
		if attr =~ "=\s*[\"']"
			" Let do attribute specific completion
			let attrname = matchstr(attr, '.*\ze\s*=')
			let entered_value = matchstr(attr, ".*=\\s*[\"']\\zs.*")
			let values = []
			if attrname == 'media'
				let values = ["screen", "tty", "tv", "projection", "handheld", "print", "braille", "aural", "all"]
			elseif attrname == 'xml:space'
				let values = ["preserve"]
			elseif attrname == 'shape'
				if a:base =~ '^a\>'
					let values = ["rect"]
				else
					let values = ["rect", "circle", "poly", "default"]
				endif
			elseif attrname == 'valuetype'
				let values = ["data", "ref", "object"]
			elseif attrname == 'method'
				let values = ["get", "post"]
			elseif attrname == 'dir'
				let values = ["ltr", "rtl"]
			elseif attrname == 'frame'
				let values = ["void", "above", "below", "hsides", "lhs", "rhs", "vsides", "box", "border"]
			elseif attrname == 'rules'
				let values = ["none", "groups", "rows", "all"]
			elseif attrname == 'align'
				let values = ["left", "center", "right", "justify", "char"]
			elseif attrname == 'valign'
				let values = ["top", "middle", "bottom", "baseline"]
			elseif attrname == 'scope'
				let values = ["row", "col", "rowgroup", "colgroup"]
			elseif attrname == 'href'
				" Now we are looking for local anchors defined by name or id
				if entered_value =~ '^#'
					let file = join(getline(1, line('$')), ' ')
					" Split it be sure there will be one id/name element in
					" item, it will be also first word [a-zA-Z0-9_-] in element
					let oneelement = split(file, "\\(meta \\)\\@<!\\(name\\|id\\)\\s*=\\s*[\"']")
					for i in oneelement
						let values += ['#'.matchstr(i, "^[a-zA-Z][a-zA-Z0-9%_-]*")]
					endfor
				endif
			elseif attrname == 'type'
				if a:base =~ '^input'
					let values = ["input-text", "password", "checkbox", "radio", "submit", "reset", "input-file", "hidden", "input-image", "input-button"]
				elseif a:base =~ '^button'
					let values = ["button", "submit", "reset"]
				endif
			else
				return []
			endif

			if len(values) == 0
				return []
			endif

			" We need special version of sbase
			let attrbase = matchstr(a:base, ".*[\"']")

			for m in values
				if m =~ entered_value
					call add(res, attrbase . m . '" ')
				endif
			endfor
		endif
		" Shorten a:base to not include last word
		let sbase = matchstr(a:base, '.*\ze\s.*')
		if tag =~ '^\(abbr\|acronym\|b\|bdo\|big\|caption\|cite\|code\|dd\|dfn\|div\|dl\|dt\|em\|fieldset\|h\d\|kbd\|li\|noscript\|ol\|p\|samp\|small\|span\|strong\|sub\|sup\|tt\|ul\|var\)$'
			let attrs = g:coregroup
		elseif tag == 'a'
			let tagspec = ["charset", "type", "name", "href", "hreflang", "rel", "rev", "shape", "coords"]
			let attrs = extend(tagspec, g:coregroup)
			let attrs = extend(attrs, g:focus)
		elseif tag == 'area'
			let attrs = g:coregroup
		elseif tag == 'base'
			let attrs = ["href", "id"]
		elseif tag == 'blockquote'
			let attrs = g:coregroup
			let attrs = extend(attrs, ["cite"])
		elseif tag == 'body'
			let attrs = g:coregroup
			let attrs = extend(attrs, ["onload", "onunload"])
		elseif tag == 'br'
			let attrs = g:coreattrs
		elseif tag == 'button'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["name", "value", "type"])
		elseif tag == '^\(col\|colgroup\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["span", "width", "align", "char", "charoff", "valign"])
		elseif tag =~ '^\(del\|ins\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["cite", "datetime"])
		elseif tag == 'form'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["action", "method", "enctype", "onsubmit", "onreset", "accept", "accept-charset"])
		elseif tag == 'head'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "profile"])
		elseif tag == 'html'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "xmlns"])
		elseif tag == 'img'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["src", "alt", "longdesc", "height", "width", "usemap", "ismap"])
		elseif tag == 'input'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["type", "name", "value", "checked", "disabled", "readonly", "size", "maxlength", "src", "alt", "usemap", "onselect", "onchange", "accept"])
		elseif tag == 'label'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["for", "accesskey", "onfocus", "onblur"])
		elseif tag == 'legend'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["accesskey"])
		elseif tag == 'link'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["charset", "href", "hreflang", "type", "rel", "rev", "media"])
		elseif tag == 'map'
			let attrs = g:i18n
			let attrs = extend(attrs, g:events)
			let attrs = extend(attrs, ["id", "class", "style", "title", "name"])
		elseif tag == 'meta'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "http-equiv", "content", "scheme", "name"])
		elseif tag == 'title'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id"])
		elseif tag == 'object'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["declare", "classid", "codebase", "data", "type", "codetype", "archive", "standby", "height", "width", "usemap", "name", "tabindex"])
		elseif tag == 'optgroup'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["disbled", "label"])
		elseif tag == 'option'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["disbled", "selected", "value", "label"])
		elseif tag == 'param'
			let attrs = ["id", "name", "value", "valuetype", "type"]
		elseif tag == 'pre'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["xml:space"])
		elseif tag == 'q'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["cite"])
		elseif tag == 'script'
			let attrs = ["id", "charset", "type", "src", "defer", "xml:space"]
		elseif tag == 'select'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["name", "size", "multiple", "disabled", "tabindex", "onfocus", "onblur", "onchange"])
		elseif tag == 'style'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["id", "type", "media", "title", "xml:space"])
		elseif tag == 'table'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["summary", "width", "border", "frame", "rules" "cellspacing", "cellpadding"])
		elseif tag =~ '^\(thead\|tfoot\|tbody\|tr\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["align", "char", "charoff", "valign"])
		elseif tag == 'textarea'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["name", "rows", "cols", "disabled", "readonly", "onselect", "onchange"])
		elseif tag =~ '^\(th\|td\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["abbr", "headers", "scope", "rowspan", "colspan", "align", "char", "charoff", "valign"])
		endif

		for m in sort(attrs)
			if m =~ attr
				if m =~ '^\(ismap\|defer\|declare\|nohref\|checked\|disabled\|selected\|readonly\)$'
					call add(res, sbase.' '.m)
				else
					call add(res, sbase.' '.m.'="')
				endif
			endif
		endfor
		return res
	endif
	" Close tag
	let b:unaryTagsStack = "base meta link hr br param img area input col"
	if a:base =~ '^\/'
		let opentag = htmlcomplete#GetLastOpenTag("b:unaryTagsStack")
		return ["/".opentag.">"]
	endif
	" Deal with tag completion.
	let opentag = htmlcomplete#GetLastOpenTag("b:unaryTagsStack")
	" Clusters
	let special = "br span bdo map object img"
	let phrase =  "em strong dfn code q samp kbd var cite abbr acronym sub sup"
	let inlineforms = "input select textarea label button"
	let miscinline = "ins del script"
	let inline = "a ".special." ".phrase." ".inlineforms." tt i b big small"
	let misc = "noscript ".miscinline
	let block = "p h1 h2 h3 h4 h5 h6 div ul ol dl pre hr blockquote address fieldset table"

	if opentag == 'a'
		let tags = split("tt i b big small ".special." ".phrase." ".inlineforms." ".miscinline)
	elseif opentag =~ '^\(abbr\|acronym\|address\|b\|p\|h\d\|dt\|span\|bdo\|em\|strong\|dfn\|code\|samp\|kbd\|var\|cite\|q\|sub\|sup\|tt\|i\|big\|small\|label\|caption\)$'
		let tags = split(inline." ".miscinline)
	elseif opentag == 'pre'
		let tags = split("a tt i b big small br span bdo map ".phrase." ".miscinline." ".inlineforms)
	elseif opentag == 'html'
		let tags = split("head body")
	elseif opentag == 'legend'
		let tags = split(inline." ".miscinline)
	elseif opentag == 'head'
		let tags = split("title base scipt style meta link object")
	elseif opentag =~ '^\(noscript\|body\|blockquote\)$'
		let tags = split("form ".block." ".misc)
	elseif opentag =~ '^\(ul\|ol\)$'
		let tags = ["li"]
	elseif opentag == 'dl'
		let tags = split("dt dd")
	elseif opentag =~ '^\(ins\|del\|th\|td\|dd\|div\|li\)$'
		let tags = split("form ".block." ".inline." ".misc)
	elseif opentag == 'object'
		let tags = split("param form ".block." ".inline." ".misc)
	elseif opentag == 'fieldset'
		let tags = split("legend form ".block." ".inline." ".misc)
	elseif opentag == 'map'
		let tags = split("area form ".block." ".misc)
	elseif opentag == 'form'
		let tags = split(block." ".misc)
	elseif opentag == 'select'
		let tags = split("optgroup option")
	elseif opentag == 'optgroup'
		let tags = ["option"]
	elseif opentag == 'colgroup'
		let tags = ["col"]
	elseif opentag == '^\(textarea\|option\|script\|style\|title\)$'
		let tags = []
	elseif opentag == 'button'
		let tags = split("p h1 h2 h3 h4 h5 h6 div ul ol dl table")
	elseif opentag =~ '^\(thead\|tfoot\|tbody)$'
		let tags = ["tr"]
	elseif opentag == 'tr'
		let tags = split("th td")
	elseif opentag == 'table'
		let tags = split("caption col colgroup thead tfoot tbody tr")
	endif

	for m in tags
		if m =~ a:base
			call add(res, m)
		endif
	endfor

	return res

  endif
endfunction

" MM: This is greatly reduced closetag.vim used with kind permission of Steven
"     Mueller
"     Changes: strip all comments; delete error messages
" Author: Steven Mueller <diffusor@ugcs.caltech.edu>
" Last Modified: Tue May 24 13:29:48 PDT 2005 
" Version: 0.9.1

function! htmlcomplete#GetLastOpenTag(unaryTagsStack)
	let linenum=line(".")
	let lineend=col(".") - 1 " start: cursor position
	let first=1              " flag for first line searched
	let b:TagStack=""        " main stack of tags
	let startInComment=s:InComment()

	let tagpat='</\=\(\k\|[-:]\)\+\|/>'
	while (linenum>0)
		let line=getline(linenum)
		if first
			let line=strpart(line,0,lineend)
		else
			let lineend=strlen(line)
		endif
		let b:lineTagStack=""
		let mpos=0
		let b:TagCol=0
		while (mpos > -1)
			let mpos=matchend(line,tagpat)
			if mpos > -1
				let b:TagCol=b:TagCol+mpos
				let tag=matchstr(line,tagpat)

				if exists("b:closetag_disable_synID") || startInComment==s:InCommentAt(linenum, b:TagCol)
					let b:TagLine=linenum
					call s:Push(matchstr(tag,'[^<>]\+'),"b:lineTagStack")
				endif
				let lineend=lineend-mpos
				let line=strpart(line,mpos,lineend)
			endif
		endwhile
		while (!s:EmptystackP("b:lineTagStack"))
			let tag=s:Pop("b:lineTagStack")
			if match(tag, "^/") == 0		"found end tag
				call s:Push(tag,"b:TagStack")
			elseif s:EmptystackP("b:TagStack") && !s:Instack(tag, a:unaryTagsStack)	"found unclosed tag
				return tag
			else
				let endtag=s:Peekstack("b:TagStack")
				if endtag == "/".tag || endtag == "/"
				call s:Pop("b:TagStack")	"found a open/close tag pair
			elseif !s:Instack(tag, a:unaryTagsStack) "we have a mismatch error
				return ""
			endif
		endif
	endwhile
	let linenum=linenum-1 | let first=0
endwhile
return ""
endfunction

function! s:InComment()
	return synIDattr(synID(line("."), col("."), 0), "name") =~ 'Comment'
endfunction

function! s:InCommentAt(line, col)
	return synIDattr(synID(a:line, a:col, 0), "name") =~ 'Comment'
endfunction


function! s:SetKeywords()
	let g:IsKeywordBak=&iskeyword
	let &iskeyword="33-255"
endfunction

function! s:RestoreKeywords()
	let &iskeyword=g:IsKeywordBak
endfunction

function! s:Push(el, sname)
	if !s:EmptystackP(a:sname)
		exe "let ".a:sname."=a:el.' '.".a:sname
	else
		exe "let ".a:sname."=a:el"
	endif
endfunction

function! s:EmptystackP(sname)
	exe "let stack=".a:sname
	if match(stack,"^ *$") == 0
		return 1
	else
		return 0
	endif
endfunction

function! s:Instack(el, sname)
	exe "let stack=".a:sname
	call s:SetKeywords()
	let m=match(stack, "\\<".a:el."\\>")
	call s:RestoreKeywords()
	if m < 0
		return 0
	else
		return 1
	endif
endfunction

function! s:Peekstack(sname)
	call s:SetKeywords()
	exe "let stack=".a:sname
	let top=matchstr(stack, "\\<.\\{-1,}\\>")
	call s:RestoreKeywords()
	return top
endfunction

function! s:Pop(sname)
	if s:EmptystackP(a:sname)
		return ""
	endif
	exe "let stack=".a:sname
	call s:SetKeywords()
	let loc=matchend(stack,"\\<.\\{-1,}\\>")
	exe "let ".a:sname."=strpart(stack, loc+1, strlen(stack))"
	let top=strpart(stack, match(stack, "\\<"), loc)
	call s:RestoreKeywords()
	return top
endfunction

function! s:Clearstack(sname)
	exe "let ".a:sname."=''"
endfunction
